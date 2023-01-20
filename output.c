#include "output.h"
#include "util.h"
#include <ctype.h>
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

static void clear_screen(DynBuffer *screen) {
  // clear screen
  dyn_buffer_append(screen, "\033[2J", 4);
  // put cursor at the top
  dyn_buffer_append(screen, "\033[;f", 4);
}
//#if C_SUPPORT == 1
static const char *c_keywords[] = {"#include",
                                 "#define",
                                 "for",
                                 "if",
                                 "else",
                                 "while",
                                 "true",
                                 "false",
                                 "int",
                                 "bool",
                                 "size_t",
                                 "char",
                                 "return",
                                 "void",
                                 "#pragma",
                                 "struct",
                                 "typedef",
                                 "union",
                                 "enum",
                                 "do",
                                 "const",
                                 "inline",
                                 "uint8_t",
                                 "uint16_t",
                                 "uint32_t",
                                 "uint64_t",
                                 "int8_t",
                                 "int16_t",
                                 "int32_t",
                                 "int64_t",
                                 "short",
                                 "long",
                                 "break",
																 "continue",
                                 "case",
                                 "default",
                                 "switch",
                                 "float",
                                 "static",
                                 "unsigned",
                                 "volatile",
                                 "double",
                                 "goto",
                                 "sizeof"};

static const char *c_operators[] = {
    // double chars are listed first becuase of the way the comparisons are done
    "+=", "-=", "*=", "/=", "==", "!=", "&&", "||", "&=", "|=", "^=",

    "=",  "+",  "-",  "*",  "/",  "!",  "|",  "&",  "^"};

static int c_is_operator(char *t, size_t len) {
  for (size_t i = 0; i < ARRLEN(c_operators); i++) {
    const char *cmp = c_operators[i];
    size_t cmp_len = strlen(cmp);
    if (len >= cmp_len && STRCMP(t, cmp, cmp_len))
      return cmp_len;
  }
  return 0;
}

static int c_is_keyword(char *t, size_t len) {
  for (size_t i = 0; i < ARRLEN(c_keywords); i++) {
    const char *cmp = c_keywords[i];
    size_t cmp_len = strlen(cmp);
    if (len == cmp_len && STRCMP(t, cmp, cmp_len)) {
      return cmp_len;
    }
  }
  return 0;
}
static bool c_is_quote = false;
static bool c_is_block_comment = false;
static bool c_is_line_comment = false;
static size_t c_highlight_next = 0;
void c_handle_highlighting(DynBuffer *screen, Editor *e, Line line, size_t col) {
	// if start of line
	if (col == 0)  {
		c_is_line_comment = false;
	}
	
	char *curr = &e->text[line.start + col];
	if (c_is_block_comment) {
		if (STRCMP(curr, "*/", 2)) {
			c_is_block_comment = false;
			c_highlight_next = 2;
		}
	} else if (!c_is_line_comment) {
		if (*curr == '"') {
			if (c_is_quote) {
				c_is_quote = false;
				c_highlight_next = 1;
			} else {
				c_is_quote = true;
			}
		}
		else if (*curr == '\'') {
			if (!c_is_quote) c_highlight_next = 1;
		}
		else if (STRCMP(curr, "//", 2)) {
			c_is_line_comment = true;
		}
		else if (STRCMP(curr, "/*", 2)) {
			c_is_block_comment = true;
		}
		else if (line.len > 8 && STRCMP(&e->text[line.start], "#include", 8) && *curr == '<')
			c_is_quote = true;
		else if (line.len > 8 && STRCMP(&e->text[line.start], "#include", 8) && *curr == '>') {
		  c_is_quote = false;
			c_highlight_next = 1;
		}
			
		else {
			size_t op_len = c_is_operator(curr, line.len - col);
			if (op_len > 0) {
				dyn_buffer_append(screen, "\033[0;31m", 7);
				c_highlight_next = op_len;
			}
			else {
				for (size_t i = 0; i < e->words->len; i++) {
					Word w = e->words->buff[i];
					if (w.start == line.start + col) {
						size_t kw_len = c_is_keyword(curr, w.len);
						if (kw_len > 0) {
							dyn_buffer_append(screen, "\033[0;31m", 7);
							c_highlight_next = kw_len;
						}
					}
				}
			}
		}
		
	}
  if (c_is_block_comment || c_is_line_comment)
    dyn_buffer_append(screen, "\033[0;90m", 7);
  else if (c_is_quote)
    dyn_buffer_append(screen, "\033[0;33m", 7);
  else if (c_highlight_next == 0) {
    dyn_buffer_append(screen, "\033[0m", 4);
  } else
     c_highlight_next -= 1;	
}

//#endif //C_SUPPORT
//#if MAKE_SUPPORT == 1
static const char *make_keywords[] = {
	"include", "ifeq", "else", "endif"
};
static const char *make_operators[] = {
	"::=", ":::=",
	"?=", "!=",
	"=", "$", ":", "(", ")"
};
static int make_is_operator(char *t, size_t len) {
  for (size_t i = 0; i < ARRLEN(make_operators); i++) {
    const char *cmp = make_operators[i];
    size_t cmp_len = strlen(cmp);
    if (len >= cmp_len && STRCMP(t, cmp, cmp_len))
      return cmp_len;
  }
  return 0;
}

static int make_is_keyword(char *t, size_t len) {
  for (size_t i = 0; i < ARRLEN(make_keywords); i++) {
    const char *cmp = make_keywords[i];
    size_t cmp_len = strlen(cmp);
    if (len == cmp_len && STRCMP(t, cmp, cmp_len)) {
      return cmp_len;
    }
  }
  return 0;
}

static bool make_is_line_comment = false;
static size_t make_highlight_next = 0;
void make_handle_highlighting(DynBuffer *screen, Editor *e, Line line, size_t col) {
	if (col == 0) make_is_line_comment = false;
	if (make_is_line_comment) return;
	char *curr = &e->text[line.start + col];
	size_t op_len = make_is_operator(curr, line.len - col);
	if (op_len > 0) {
		dyn_buffer_append(screen, "\033[0;35m", 7);
		make_highlight_next = op_len;
 	} else {
		for (size_t i = 0; i < e->words->len; i++) {
			Word w = e->words->buff[i];
			if (w.start == line.start + col) {
				size_t kw_len = make_is_keyword(curr, w.len);
				if (kw_len > 0) {
					dyn_buffer_append(screen, "\033[0;31m", 7);
					make_highlight_next = kw_len;
				} else {
					if (col == 0 && w.start + w.len < e->str_len && e->text[w.start + w.len] == ':') {
						dyn_buffer_append(screen, "\033[0;36m", 7);
						make_highlight_next = w.len;
					}
				}
			}
		}
	}
	if (make_is_line_comment)
    dyn_buffer_append(screen, "\033[0;90m", 7);
  else if (make_highlight_next == 0) {
    dyn_buffer_append(screen, "\033[0m", 4);
  } else
     make_highlight_next -= 1;	
}

//#endif //MAKE_SUPPORT
void handle_output(Editor *e) {
	
  c_is_quote = false;
  c_is_block_comment = false;
  c_is_line_comment = false;
	make_is_line_comment = false;

  DynBuffer *screen = dyn_buffer_new();
  // hide cursor to avoid possible tearing effect
  dyn_buffer_append(screen, "\033[?25l", 6);
  clear_screen(screen);
  // draw text
	
  for (size_t i = e->scrollv; i < e->lines->len && i < e->rows + e->scrollv; i++) {
    Line l = e->lines->buff[i];
    size_t len;
    if (l.len > e->cols) {
      len = e->cols;
    } else {
      len = l.len - e->scrollh;
    }
  
    for (size_t j = 0; j < len; j++) {
			c_handle_highlighting(screen, e, l, j);
			//make_handle_highlighting(screen, e, l, j);
      size_t index = l.start + e->scrollh + j;
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
		LOG("len: %zu\n", screen->len);
    dyn_buffer_append(screen, "\r\n", 2);
  }
  // draw cursor
  char buff[32];
  size_t len =
      snprintf(buff, sizeof(buff), "\033[%zu;%zuf", e->cy + 1, e->cx + 1);
  dyn_buffer_append(screen, buff, len);
  // restore cursor
  dyn_buffer_append(screen, "\033[?25h", 6);
  WRITE(screen->text, screen->len);
  dyn_buffer_free(screen);
}

DynBuffer *dyn_buffer_new(void) {
  DynBuffer *db = malloc(sizeof(DynBuffer));

  db->len = 0;
  db->text = NULL;

  return db;
}
int dyn_buffer_append(DynBuffer *db, char *str, size_t len) {
  db->text = realloc(db->text, db->len + len);
  if (db->text == NULL)
    return -1;
  memcpy(&db->text[db->len], str, len);
  db->len += len;
  return 0;
}

void dyn_buffer_free(DynBuffer *db) {
  free(db->text);
  free(db);
}
