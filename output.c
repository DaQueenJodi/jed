#include "input.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

int get_screen_size(int *c, int *r) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
	  return -1;
	}
	*c = ws.ws_col;
	*r = ws.ws_row;
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
	for (int i = 0; i < e->lines->len; i++) {
		if (i > e->rows) break;
		Line l = e->lines->buff[i + e->scrollv];
		//char *buff = malloc(l.len + 5);
		//size_t len = snprintf(buff, l.len + 5, "%d %.*s", i, l.len, e->text + l.start + e->scrollh);
		//WRITE(buff, len);
		WRITE(e->text + l.start + e->scrollh, l.len);
		//free(buff);
		WRITE_LIT("\r\n");
	}
	// draw cursor
	char buff[32];
	size_t len = snprintf(buff, sizeof(buff), "\033[%d;%df", e->cy + 1, e->cx + 1);
	WRITE(buff, len);
}
