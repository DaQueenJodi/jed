#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct {
	size_t start;
	size_t len;
} Line;

typedef struct lines {
	size_t len;
	size_t cap;
	Line *buff;
} Lines;

typedef struct {
	size_t cx, cy;
	bool quit;
	char *text;
	Lines *lines;
	size_t scrollh, scrollv;
	size_t rows, cols;
	size_t real_cursor;
	size_t str_len;
} Editor;

Lines *lines_new(void);
void lines_free(Lines *ls);
int lines_append(Lines *ls, Line l);
size_t current_line(Editor *e);
size_t current_column(Editor *e);
void write_char(Editor *e, char c);
void delete_char(Editor *e, bool inplace);
void set_cursor(Editor *e);
void editor_save_file(Editor *e);
