
#define COMMENT_COLOR = {112, 128, 153};
#define STRING_COLOR = {240, 230, 140};


void move_end(Editor *e) {
	size_t curr_line = current_line(e);
	Line *l = &e->lines->buff[curr_line];
	e->real_cursor = l->start + l->len;
	run_after_move(e);
}
void move_start(Editor *e) {
	size_t curr_line = current_line(e);
	e->real_cursor = e->lines->buff[curr_line].start;
	run_after_move(e);
}
void delete_char_inplace(Editor *e) {
	delete_char(e, true);
	run_after_move(e);
}
#define KEY(k, func) {{k}, func}
const Binding keys[] = {
	KEY(CTRL('q'), quit),
	KEY(CTRL('e'), move_end),
	KEY(CTRL('a'), move_start),
	KEY(CTRL('b'), move_left),
	KEY(CTRL('n'), move_down),
	KEY(CTRL('p'), move_up),
	KEY(CTRL('f'), move_right),
	KEY(CTRL('d'), delete_char_inplace),
};
