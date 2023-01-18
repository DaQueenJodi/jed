#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
	size_t start;
	size_t len;
} Line;

typedef struct {
	size_t len;
	size_t cap;
	Line *buff;
} Lines;

typedef struct {
	size_t start;
	size_t len;
} Word;

typedef struct {
	size_t len;
	size_t cap;
	Word *buff;
} Words;

typedef struct {
	size_t cx, cy;
	bool quit;
	char *text;
	Lines *lines;
	size_t scrollh, scrollv;
	size_t rows, cols;
	size_t real_cursor;
	size_t str_len;
	Words *words;
} Editor;

Lines *lines_new(void);
int lines_append(Lines *ls, Line l);
void lines_free(Lines *ls);

Words *words_new(void);
int words_append(Words *ws, Word w);
void words_free(Words *ws);

size_t current_line(Editor *e);
size_t current_column(Editor *e);
void write_char(Editor *e, char c);
void delete_char(Editor *e, bool inplace);
void set_cursor(Editor *e);
void editor_save_file(Editor *e);
void run_after_move(Editor *e);
