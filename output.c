#include "input.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

int set_screen_size(Editor *e) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
	  return -1;
	}
	e->cols = ws.ws_col;
	e->rows = ws.ws_row;
	return 0;
}


static void clear_screen(void) {
	// clear screen
	WRITE_LIT("\033[2J");
	// put cursor at the top
	WRITE_LIT("\033[;f");
}

void handle_output(Editor *e) {
	clear_screen();
	// draw text
	for (size_t i = e->scrollv; i < e->lines->len && i < e->rows; i++) {
		Line l = e->lines->buff[i];
		size_t len;
		if (l.len  > e->cols) {
			len = e->cols;
		} else {
			len = l.len - e->scrollh;
		}
		//fprintf(stderr, "writing %zu bytes from: e->text[%zu + %zu]\n", len - e->scrollh, l.start, e->scrollh);
		WRITE(&e->text[l.start + e->scrollh], len);
		//fprintf(stderr, "wrote: %.*s\n", (int)(len), &e->text[l.start + e->scrollh]);
		WRITE_LIT("\r\n");
	}
	// draw cursor
	char buff[32];
	size_t len = snprintf(buff, sizeof(buff), "\033[%zu;%zuf", e->cy + 1, e->cx + 1);
	WRITE(buff, len);
}
