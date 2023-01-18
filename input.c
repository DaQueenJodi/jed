#include "input.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef struct {
	char chars[4];
} Key;
typedef struct {
	Key key;
	void (*func)(Editor *e);
} Binding;



static Key read_input(void) {
	Key k;
	// wait until we something
	while (READ(k.chars, 4) == 0);
	if (k.chars[0] == '\r') k.chars[0] = '\n';
	return k;
}
static size_t curr_line;
static void move_up(Editor *e) {
	if (curr_line > 0) {
		Line l = e->lines->buff[curr_line];
		size_t col = e->real_cursor - l.start;
		if (col > e->lines->buff[curr_line - 1].len) col = e->lines->buff[curr_line - 1].len;
		e->real_cursor = e->lines->buff[curr_line - 1].start + col;
		LOG("moved from line %zu to line %zu\n", curr_line, curr_line - 1);
	} else {
	  LOG("not moving; beginning of buffer\n");
	}
}

static void move_down(Editor *e) {
	// if trying to go past last line, make a new line
	if (curr_line == e->lines->len - 1 && e->lines->buff[curr_line].len > 0) {
		LOG("adding newline at the end\n");
		// move cursor to the end
		e->real_cursor = e->str_len;
		write_char(e, '\n');
	}
	if (curr_line < e->lines->len - 1) {
		Line l = e->lines->buff[curr_line];
	  size_t col = e->real_cursor - l.start;
		if (col > e->lines->buff[curr_line + 1].len) col = e->lines->buff[curr_line + 1].len;
	  e->real_cursor = e->lines->buff[curr_line + 1].start + col;
		LOG("moved from line %zu to line %zu\n", curr_line, curr_line + 1);
	} else LOG("not moving; end of buffer\n");
	
}

static void move_left(Editor *e) {
	
	if (e->real_cursor > 0) e->real_cursor -= 1;
}

static void move_right(Editor *e) {
	if (e->real_cursor == e->str_len - 1 && e->lines->buff[curr_line].len > 0) {
		LOG("adding newline at the end\n");
		// move cursor to the end
		e->real_cursor = e->str_len;
		write_char(e, '\n');
	}
	if (e->real_cursor < e->str_len - 1) {
		e->real_cursor += 1;
		LOG("moving right\n");
	} else LOG("not moving right; end of buffer\n");
}

void quit(Editor *e) {
	e->quit = true;
}

#include "config.h"

#define STRCMP(str1, str2, count) memcmp((str1), (str2), (count)) == 0
void handle_input(Editor *e) {
	curr_line = current_line(e);
	Key k = read_input();
	bool found = false;
	for (size_t i = 0; i < ARRLEN(keys); i++) {
		if (STRCMP(keys[i].key.chars, k.chars, 1)) {
			keys[i].func(e);
			found = true;
		}
	}
	if (!found) {
		// backspace
		if (k.chars[0] == 127) {
			delete_char(e, false);
			move_left(e);
	  } else {
		write_char(e, k.chars[0]);
		move_right(e);
 	 }
	
	}

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
