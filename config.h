#define C_SUPPORT 1
#define MAKE_SUPPORT 1
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

void delete_after(Editor *e) {
	Line l = e->lines->buff[current_line(e)];
	size_t col = e->real_cursor - l.start;
	for (size_t i = 0; i < l.len - col; i++) {
		delete_char_inplace(e);
		run_after_move(e);
	}
}

void electric_return(Editor *e) {
	Line l = e->lines->buff[current_line(e)];
	write_char(e, '\n');
	move_right(e);
	
	size_t counter;
	char *chars = malloc(l.len);
	for (counter = 0; counter < l.len; counter++) {
		char c = e->text[l.start + counter];
		if (!isblank(c))
			break;
		chars[counter] = c;
	}
	for (size_t i = 0 ; i < counter; i++) {
		write_char(e, chars[i]);
		move_right(e);
	}
	run_after_move(e);
	free(chars);
}


void save_the_file(Editor *e) {
  editor_save_file(e);
}

static const char quit_key = CTRL('q');


#define KEY(k) { { (k) }, 1}
const Binding keys[] = {
	{KEY(CTRL('q')), quit},
	{KEY(CTRL('b')), move_left},
	{KEY(CTRL('n')), move_down},
	{KEY(CTRL('p')), move_up},
	{KEY(CTRL('f')), move_right},
	{KEY(CTRL('e')), move_end},
	{KEY(CTRL('a')), move_start},
	{KEY(CTRL('d')), delete_char_inplace},
	{KEY(CTRL('k')), delete_after},
	
  {KEY(CTRL('s')), save_the_file},
	// Electric keys
	// automatically indent to outer level
	{KEY('\n'), electric_return},

};
