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
	/*
	raw.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	raw.c_iflag &= ~(IXON | ICRNL  | IGNBRK| PARMRK | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	*/
	cfmakeraw(&raw);

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		DIE("tcsetattr");
	}
	// set cursor to bar
	WRITE_LIT("\033]50;CursorShape=1\x7");
}

void exit_raw_mode(void) {
	// set cursor back to block
	WRITE_LIT("\033]50;CursorShape=0\x7");
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
		DIE("tcsetattr");
	}
}
int write_file(char *path, char *buffer, size_t len) {
	FILE *f = fopen(path, "w");
	if (f == NULL) return -1;
	
	if (fwrite(buffer, 1, len, f) != len) {
		return -1;
	}
	
	fclose(f);
	return 0;
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

Words *gen_words(char *text, size_t len) {
	Words *ws = words_new();
	Word w;
	w.start = 0;
	for (size_t counter = 0; counter < len; counter++) {
		char c = text[counter];
		if (c == '\n' || c == ' ') {
			size_t len = counter - w.start;
			if (len > 0) {
				w.len = len;
				if (words_append(ws, w) < 0) return NULL;
				w.start = counter + 1;
			}
		}
	}
	return ws;
}

Lines *gen_lines(char *buffer, size_t len) {
	Lines *ls = lines_new();
	Line l;
	
	l.start = 0;
	for  (size_t counter = 0; counter < len; counter++) {
		if (buffer[counter] == '\n') {
			l.len = (counter - l.start);
			if (lines_append(ls, l) < 0) return NULL;
			l.start = counter + 1;
		}
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
							 .scrollv			= 0, };
	
	if (set_screen_size(&e) < 0) {
		DIE("get_screen_size");
	}
	if (path) {
		e.text = read_file(path);
	} else {
		e.text = strdup("\n");
	}
	e.str_len = strlen(e.text);
	e.words = gen_words(e.text, e.str_len);
	e.lines = gen_lines(e.text, e.str_len);

	while (!e.quit) {
		handle_output(&e);
		handle_input(&e);
	}
	lines_free(e.lines);
	words_free(e.words);
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

size_t inline current_line(Editor *e) {
	return e->scrollv + e->cy;
}
size_t inline current_column(Editor *e) {
	return e->scrollh + e->cx;
}


void delete_char(Editor *e, bool inplace) {
	if (!inplace && e->real_cursor == 0) {
		LOG( "can't delete nothingness\n");
		return;
	}
	if (inplace && e->real_cursor + 1 >= e->str_len) {
		LOG( "can't delete nothingness\n");
		return;
	}
	size_t len = e->str_len;
	
		int offset1, offset2;
		if (inplace) {
			offset1 = +0;
			offset2 = +1;
		} else {
			offset1 = -1;
			offset2 = +0;
		}
		memmove(&e->text[e->real_cursor + offset1], &e->text[e->real_cursor + offset2], len - e->real_cursor);
	
	e->text[len - 1] = '\0';
	lines_free(e->lines);
	words_free(e->words);
	e->str_len -= 1;
	e->lines = gen_lines(e->text, e->str_len);
	e->words = gen_words(e->text, e->str_len);
}

void write_char(Editor *e, char c) {
	LOG("writing character '%c'\n", c);
	size_t len = e->str_len;
	e->text = realloc(e->text, len + 1 + 1);
	if (e->text == NULL)
		DIE("realloc");
	// only memmove if needed
	if (e->real_cursor != len + 1) {
     memmove(&e->text[e->real_cursor + 1], &e->text[e->real_cursor], len - e->real_cursor);
	}
	e->text[e->real_cursor] = c;
	e->text[len + 1] = '\0';
	lines_free(e->lines);
	words_free(e->words);
	e->str_len += 1;
	e->lines = gen_lines(e->text, e->str_len);
	e->words = gen_words(e->text, e->str_len);
}

void set_cursor(Editor *e) {
	size_t lcount = 0;
	for (size_t counter = 0; counter < e->real_cursor; counter++) {
		char c = e->text[counter];
		if (c == '\n') {
			lcount += 1;
		}
	}
	e->cy = lcount;
	int line = current_line(e);
	size_t pos = e->real_cursor - e->lines->buff[line].start;
	e->cx = pos;
	for (size_t counter = 0; counter < pos; counter++) {
		char c = e->text[e->lines->buff[line].start + counter];
		// tab
		if (c == 9 || iscntrl(c)) {
			e->cx += 1;
		}
	}
}
void editor_save_file(Editor *e) {
	LOG("writing file\n");
	write_file("welp.bak", e->text, e->str_len);
}


Words *words_new(void) {
	Words *ws = malloc(sizeof(Words));
	ws->cap = 5;
	ws->len = 0;
	ws->buff = malloc(sizeof(Word) * ws->cap);
	return ws;
}

int words_append(Words *ws, Word w) {
	if (ws->len == ws->cap) {
		ws->cap *= 2;
		ws->buff = realloc(ws->buff, ws->cap * sizeof(Word));
		if (ws->buff == NULL) return -1;
	}
	ws->buff[ws->len++] = w;
	return 0;
}

void words_free(Words *ws) {
	free(ws->buff);
	free(ws);
}

void run_after_move(Editor *e) {
	set_cursor(e);
	size_t line_num = current_line(e);
	Line l = e->lines->buff[line_num];
	
	int diff = (e->real_cursor - l.start) - e->cols;
	if (diff < 0) diff = 0;
	e->scrollh = diff;
	if (diff > 0) LOG("new scrollh: %zu\n", e->scrollh);
	
	diff = line_num - e->rows;
	if (diff < 0) diff = 0;
	e->scrollv = diff;
	if (diff > 0) LOG("new scrollv: %zu\n", e->scrollv);

	LOG("real_cursor: %zu\n", e->real_cursor);
	LOG("cx: %zu, cy: %zu\n", e->cx, e->cy);
}
