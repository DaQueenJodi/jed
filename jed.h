#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct {
	size_t start;
	int len;
} Line;

typedef struct lines {
	int len;
	int cap;
	Line *buff;
} Lines;

typedef struct {
	int cx, cy;
	bool quit;
	char *text;
	Lines *lines;
	int scrollh, scrollv;
	int rows, cols;
	int real_cursor;
} Editor;

Lines *lines_new(void);
void lines_free(Lines *ls);
int lines_append(Lines *ls, Line l);
int current_line(Editor *e);
int current_column(Editor *e);
void write_char(Editor *e, char c);
void delete_char(Editor *e, bool inplace);
void set_cursor(Editor *e);
