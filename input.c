#include "input.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef struct {
	char chars[4];
	size_t len;
} Key;
typedef struct {
	Key key;
	void (*func)(Editor *e);
} Binding;



static Key read_input(void) {
	Key k;
	// wait until we something
	k.len = 1;
	while (READ(k.chars, 1) <= 0);
	if (k.chars[0] == '\033') {
		ssize_t len;
		while ((len = READ(k.chars, 3)) <= 0);
		k.len += len;
	}
	if (k.chars[0] == '\r') k.chars[0] = '\n';
	LOG("k len: %zu\n", k.len);
	if (k.len != 1) LOG("k: %.*s", (int)k.len, k.chars);
	return k;
}
static void move_up(Editor *e) {
	size_t curr_line = current_line(e);
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
	size_t curr_line = current_line(e);
	// if trying to go past last line, make a new line
	
	if (curr_line == e->lines->len - 1 && e->lines->buff[curr_line].len > 0) {
		LOG("adding newline at the end\n");
		write_char(e, '\n');
	}
	
	if (curr_line < e->lines->len - 1) {
		
		Line next = e->lines->buff[curr_line + 1];
		size_t col = e->real_cursor - e->lines->buff[curr_line].start;
		col = col < next.len ? col : next.len;
		e->real_cursor = next.start + col;
	}
}

static void move_left(Editor *e) {
	if (e->real_cursor > 0) e->real_cursor -= 1;
}

static void move_right(Editor *e) {
	size_t curr_line = current_line(e);
	if (e->real_cursor == e->str_len - 1 && e->lines->buff[curr_line].len > 0) {
		LOG("adding newline at the end\n");
		write_char(e, '\n');
	}
	else if (e->real_cursor < e->str_len - 1) {
		e->real_cursor += 1;
		LOG("moving right\n");
	} else LOG("not moving right; end of buffer\n");
}

void quit(Editor *e) {
	e->quit = true;
}

#include "config.h"


void handle_input(Editor *e) {
	size_t cursor_begin = e->real_cursor;
	Key k = read_input();
	bool found = false;
	for (size_t i = 0; i < ARRLEN(keys); i++) {
		if (keys[i].key.len != k.len) continue;
		if (STRCMP(keys[i].key.chars, k.chars, k.len)) {
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
	if (e->real_cursor != cursor_begin)
		run_after_move(e);
}
