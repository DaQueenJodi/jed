#include "output.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>

int set_screen_size(Editor *e) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
	  return -1;
	}
	e->cols = ws.ws_col;
	e->rows = ws.ws_row;
	return 0;
}


static void clear_screen(DynBuffer *screen) {
	// clear screen
	dyn_buffer_append(screen, "\033[2J", 4);
	// put cursor at the top
	dyn_buffer_append(screen, "\033[;f", 4);
}

void handle_output(Editor *e) {
	// draw text
	DynBuffer *screen = dyn_buffer_new();
	clear_screen(screen);
	for (size_t i = e->scrollv; i < e->lines->len && i < e->rows; i++) {
		Line l = e->lines->buff[i];
		size_t len;
		if (l.len  > e->cols) {
			len = e->cols;
		} else {
			len = l.len - e->scrollh;
		}
		for (size_t j = 0; j < len; j++) {
			size_t index = l.start + e->scrollh + j;
			//if (index >= e->str_len) continue;
			char c = e->text[index];
			// tab
			if (c == 9)
				dyn_buffer_append(screen, "  ", 2);
		  else if (iscntrl(c)) {
				char buff[3];
				sprintf(buff, "^%c", c + '@');
				dyn_buffer_append(screen, buff, 2);
					
			} else {
				dyn_buffer_append(screen, &c, 1);
			}
			
		}
		dyn_buffer_append(screen, "\r\n", 2);
	}
	// draw cursor
	
	char buff[32];
	size_t len = snprintf(buff, sizeof(buff), "\033[%zu;%zuf", e->cy + 1, e->cx + 1);
	dyn_buffer_append(screen, buff, len);
	WRITE(screen->text, screen->len);
}

DynBuffer *dyn_buffer_new(void) {
	DynBuffer *db = malloc(sizeof(DynBuffer));

	db->len = 0;
	db->text = NULL;
	
	return db;
}
int dyn_buffer_append(DynBuffer *db, char *str, size_t len) {
	db->text = realloc(db->text, db->len + len);
	if (db->text == NULL) return -1;
	memcpy(&db->text[db->len], str, len);
	db->len += len;
	return 0;
}
