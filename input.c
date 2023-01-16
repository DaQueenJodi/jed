#include "input.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>



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
static int curr_line;
static void move_up(Editor *e) {
	if (e->cy == 0 && e->scrollv > 0) e->scrollv -= 1;
			else if (e->cy > 0) e->cy -= 1;	
}

static void move_down(Editor *e) {
	if (e->cy == e->rows - 1 && curr_line < e->lines->len - 2)
		e->scrollv += 1;
	else if (e->cy < e->lines->len) e->cy += 1;
}

static void move_left(Editor *e) {
	if (e->cx == 0 && e->scrollh > 0) e->scrollh -= 1;
	else if (e->cx > 0) e->cx -= 1;
}

static void move_right(Editor *e) {
	int len = e->lines->buff[curr_line].len;
	if (e->cx == e->cols - 1 && e->cx + e->scrollh < e->lines->buff[curr_line].len) e->scrollh += 1;
	else if (e->cx < len)  e->cx += 1;
}

static void move_end(Editor *e) {

			int len = e->lines->buff[curr_line].len;
			int diff = len - e->cols;
			if (diff <= 0)
				e->cx = len;
			else {
				e->cx = len - diff;
				e->scrollh = diff;
		  }
}

static void move_beginning(Editor *e) {
			e->scrollh = 0;
			e->cx = 0;
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
int new_line = current_line(e);
	if (curr_line != new_line && new_line < e->lines->len) {
	  // snap cursor
		int len = e->lines->buff[new_line].len;
		if (e->cx > len) e->cx = len;
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
		default:
			// backspace
			if (k.key == 127) {
				delete_char(e, false);
				move_left(e);
		  } else {
			write_char(e, k.key);
			if (k.key == '\n')
				handle_movement(e, (Key){.is_special = true, .key = MOVE_DOWN});
			else
				handle_movement(e, (Key){.is_special = true, .key = MOVE_RIGHT});
			}
	  }
	}
}
