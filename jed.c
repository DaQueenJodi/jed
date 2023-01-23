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

struct termios orig_termios;

void enter_raw_mode(void) {
	struct termios raw;
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
		DIE("tcgetattr");
	}
	
	raw = orig_termios;
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
	int len = ftell(f);
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


static const char WORD_DELIMINATORS[] = {' ', '\n', 9, '(', ')', '[', ']', ';', ':'};
Words *gen_words(char *text, size_t len) {
	Words *ws = words_new();
	Word w;
	
	for (size_t counter = 0; counter < len; counter++) {
		char c = text[counter];
		bool found = false;
		for (size_t i = 0; i < ARRLEN(WORD_DELIMINATORS); i++) {
			if (c == WORD_DELIMINATORS[i]) {
				found = true;
				break;
			}
		}
		if (found) continue;
		w.start = counter;
		for (; counter < len; counter++) {
			char c = text[counter];
			size_t found = false;
			for (size_t i = 0; i < ARRLEN(WORD_DELIMINATORS); i++) {
				if (c == WORD_DELIMINATORS[i]) {
					w.len = counter - w.start;
					words_append(ws, w);
					found = true;
					break;
				}
			}
			if (found) break;
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
	enter_raw_mode();
	atexit(exit_raw_mode);

	bool test = false;
	char *path = NULL;

	if (argc > 1) {
		char *str = argv[1];
		if (STRCMP(str, "test", 4)) test = true;
		else path = str;
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
	
	
	if (path) {
		e.text = read_file(path);
	} else {
		e.text = strdup("\n");
	}
	e.str_len = strlen(e.text);
	e.words = gen_words(e.text, e.str_len);
	e.lines = gen_lines(e.text, e.str_len);

	while (!e.quit) {
		if (set_screen_size(&e) < 0)
			DIE("get_screen_size");
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

inline size_t current_line(Editor *e) {
	return e->cy + e->scrollv;
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
	if (e->lines == NULL) DIE("failed to allocate lines");
	e->words = gen_words(e->text, e->str_len);
	if (e->words == NULL) DIE("failed to allocate words");
}

void set_cursor_y(Editor *e) {
	size_t lcount = 0;
	for (size_t counter = 0; counter < e->real_cursor; counter++) {
		char c = e->text[counter];
		if (c == '\n') {
			lcount += 1;
		}
	}
	e->cy = lcount;
}
void set_cursor_x(Editor *e) {
	size_t line = current_line(e);
	size_t start = e->lines->buff[line].start;
	size_t pos = e->real_cursor - start;
	
	LOG("line_start: %zu\n", e->lines->buff[line].start);
	LOG("curr_line: %zu\n", line);
	LOG("lines_len %zu\n", e->lines->len);
	LOG("pos: %zu\n", pos);
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
	write_file("welp.c", e->text, e->str_len);
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

	set_cursor_y(e);
	size_t line_num = current_line(e);

	int diff = line_num + 1 - e->rows - e->scrollv;
	if (diff < 0) diff = 0;
	e->scrollv = diff;
	if (diff > 0) LOG("new  scrollv: %zu\n", e->scrollv);
	e->cy -= diff;
	
	set_cursor_x(e);
	// has to be a seperate call since the original is no longer valid
	Line l = e->lines->buff[current_line(e)];
	diff = e->real_cursor - l.start - e->cols;
	if (diff < 0) diff = 0;
	e->scrollh = diff;
	if (diff > 0) LOG("new scrollh: %zu\n", e->scrollh);
	e->cx -= diff;
	

	LOG("real_cursor: %zu\n", e->real_cursor);
	LOG("cx: %zu, cy: %zu\n", e->cx, e->cy);
}

void editor_enter_minibuffer(Editor *e) {
	
}
