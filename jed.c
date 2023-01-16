#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include "input.h"
#include "output.h"
#include <termios.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define JODCCL_IMPLEMENTATION
#include "jodccl.h"

struct termios orig_termios;


void enter_raw_mode(void) {
	struct termios raw;
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
		DIE("tcgetattr");
	}
	raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		DIE("tcsetattr");
	}
}

void exit_raw_mode(void) {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
		DIE("tcsetattr");
	}
}


char *read_file(char *path) {
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		DIE("fopen");
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		DIE("fseek");
	}
	size_t len = ftell(f);
	if (len < 0) {
		DIE("ftell");
	}
	rewind(f);

	char *buffer = malloc(len + 1);
	if (buffer == NULL) {
		DIE("malloc");
	}

	if (fread(buffer, len, 1, f) == 0) {
		DIE("fread");
	}		
	fclose(f);

	buffer[len] = '\0';
	return buffer;
}


Lines *gen_lines(char *buffer) {
	Lines *ls = lines_new();
	Line l;
	size_t counter = 0;
	size_t len = strlen(buffer);
	l.start = 0;
	while (counter < len) {
		if (buffer[counter] == '\n') {
			l.len = (counter - l.start);
			lines_append(ls, l);
			l.start = counter + 1;
		}
		counter += 1;
	}
	return ls;
}


int main(int argc, char **argv) {
	(void)argc;
	
	enter_raw_mode();
	atexit(exit_raw_mode);

	bool test = false;
	char *path = NULL;

	jcl_add_bool(&test, "test", "t", NULL, false);
	jcl_add_str(&path, "path", "p", NULL, false);
	JCLError err = jcl_parse_args(argv);
	if (err.err != JCL_PARSE_OK) {
		jcl_print_error(err);
		return 1;
	}

	if (test) {
		while (true) {
			char c;
			while (READ(&c, 1) == 0);
			if (c == CTRL('q')) {
				return 0;
			}
			if (iscntrl(c)) {
				printf("%d\r\n", c);
			} else {
				printf("%d: '%c'\r\n", c, c);
			}
		}
	}

	Editor e = { .cx					= 0,
							 .cy					= 0,
							 .quit				= false,
							 .lines				= NULL,
	             .text				= NULL,
							 .scrollh			= 0,
							 .real_cursor = 0,
							 .scrollv			= 0 };
	
	if (get_screen_size(&e.cols, &e.rows) < 0) {
		DIE("get_screen_size");
	}
	if (path) {
		e.text = read_file(path);
	} else {
		e.text = strdup("Hello World\nAAAA\nUawau\n");
	}
	e.lines = gen_lines(e.text);
	
	while (!e.quit) {
		handle_output(&e);
		handle_input(&e);
	}
	lines_free(e.lines);
	free(e.text);
}

Lines *lines_new(void) {
	Lines *ls = malloc(sizeof(Lines));
	if (ls == NULL) {
		DIE("malloc");
	}
	ls->cap = 5;
	ls->len = 0;
	ls->buff = malloc(sizeof(Line) * ls->cap);
	return ls;
}

int lines_append(Lines *ls, Line l) {
	if (ls->len == ls->cap) {
		ls->cap *= 2;
		ls->buff = realloc(ls->buff, ls->cap * sizeof(Line));
		if (ls->buff == NULL) {
			return -1;
		}
	}
	ls->buff[ls->len++] = l;
	return 0;
}

void lines_free(Lines *ls) {
	free(ls->buff);
	free(ls);
}

int inline current_line(Editor *e) {
	return e->scrollv + e->cy;
}
int inline current_column(Editor *e) {
	return e->scrollh + e->cx;
}

void delete_char(Editor *e, bool inplace) {
	int col = current_column(e);
	if (!inplace) col -= 1;
	int line = current_line(e);
	int offset = e->lines->buff[line].start + col;
	size_t len = strlen(e->text);
	memmove(&e->text[offset], &e->text[offset + 1], len - (offset + 1));
	e->text[len - 1] = '\0';
	e->lines = gen_lines(e->text);
}
void write_char(Editor *e, char c) {
	int col = current_column(e);
	int line = current_line(e);
	int offset = e->lines->buff[line].start + col;
	size_t len = strlen(e->text);
	e->text = realloc(e->text, len + 1 + 1);
	if (e->text == NULL)
		DIE("realloc");
	memmove(&e->text[offset + 1], &e->text[offset], len - (offset + 1));
	e->text[offset] = c;
	e->lines = gen_lines(e->text);
}


void set_cursor(Editor *e) {
	size_t lcount = 0;
	size_t counter = 0;
	while (counter++ < e->real_cursor) {
		char c = e->text[counter];
		if (c == '\n' || c == '\0') {
			lcount += 1;
		}
	}
	e->cy = lcount;
	int line = current_line(e);
	e->cx = e->real_cursor - e->lines->buff[line].start;
}
