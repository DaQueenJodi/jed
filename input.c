#include "input.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>


typedef struct {
	bool is_special;
	int key;
} Key;

static Key read_input(void) {
	Key k;
	k.is_special = true;
	char c;
	// wait until we something
	while (READ(&c, 1) == 0);
	switch (c) {
	case CTRL('b'):
		k.key = MOVE_LEFT;
		break;
	case CTRL('n'):
		k.key = MOVE_DOWN;
		break;
	case CTRL('p'):
		k.key = MOVE_UP;
		break;
	case CTRL('f'):
		k.key = MOVE_RIGHT;
		break;
	case '\r':
		k.is_special = false;
		k.key = '\n';
		break;
	 default:
		 k.is_special = false;
		 k.key = c;
	}
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

static void move_end(Editor *e) {
	Line *l = &e->lines->buff[curr_line];
	e->real_cursor = l->start + l->len;
}

static void move_beginning(Editor *e) {
	e->real_cursor = e->lines->buff[curr_line].start;
}

static void handle_movement(Editor *e, Key k) {
	curr_line = current_line(e);
	if (k.is_special) {
		switch ((SpecialKey)k.key) {
		case MOVE_DOWN: {
			move_down(e);
			break;
		}
		case MOVE_UP: {
			move_up(e);
			break;
		}
		case MOVE_LEFT:
			move_left(e);
			break;
		case MOVE_RIGHT: {
			move_right(e);
		  break;
		}
		}
	} else {
		switch ((char)k.key) {
		case CTRL('e'): {
			move_end(e);
			break;
		}
		case CTRL('a'):
			move_beginning(e);
			break;
		
	}
  }
	
}
void handle_input(Editor *e) {
	Key k = read_input();
	if (k.is_special) {
		switch ((SpecialKey)k.key) {
		case MOVE_DOWN:
		case MOVE_UP:
		case MOVE_LEFT:
		case MOVE_RIGHT:
			handle_movement(e, k);
			break;
		}
	} else {
		switch ((char)k.key) {
		case CTRL('q'):
			e->quit = true;
			break;
		case CTRL('e'):
		case CTRL('a'):
			handle_movement(e, k);
			break;
		case CTRL('s'):
			editor_save_file(e);
			break;
		case CTRL('d'):
			delete_char(e, true);
			break;
		default:
			// backspace
			if (k.key == 127) {
				delete_char(e, false);
				move_left(e);
		  } else {
			write_char(e, k.key);
			move_right(e);
 	  }
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
	
	/*
	size_t new_line = current_line(e);
	if (curr_line != new_line) {
	  // snap cursor
		Line l = e->lines->buff[new_line];
		if ((e->real_cursor - l.start) > l.len) e->real_cursor = l.start + l.len;
		}*/
	LOG("real_cursor: %zu\n", e->real_cursor);
	LOG("cx: %zu, cy: %zu\n", e->cx, e->cy);
}
