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

static const char *keywords[] = {"#include",
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
                                 "*",
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
                                 "int8_t"
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

static const char *operators[] = {
    // double chars are listed first becuase of the way the comparisons are done
    "+=", "-=", "*=", "/=", "==", "!=", "&&", "||", "&=", "|=", "^=",

    "=",  "+",  "-",  "*",  "/",  "!",  "|",  "&",  "^"};
static const char LINE_COMMENT[] = "//";
static const char START_BLOCK_COMMENT[] = "/*";
static const char END_BLOCK_COMMENT[] = "*/";

int is_operator(char *t, size_t len) {
  for (size_t i = 0; i < ARRLEN(operators); i++) {
    const char *cmp = operators[i];
    size_t cmp_len = strlen(cmp);
    if (len >= cmp_len && STRCMP(t, cmp, cmp_len))
      return cmp_len;
  }
  return 0;
}

bool is_keyword(char *t, size_t len) {
  for (size_t i = 0; i < ARRLEN(keywords); i++) {
    const char *cmp = keywords[i];
    size_t cmp_len = strlen(cmp);
    if (len == cmp_len && STRCMP(t, cmp, cmp_len)) {
      return true;
    }
  }
  return false;
}

void handle_output(Editor *e) {

  bool is_quote = false;
  bool is_block_comment = false;
  bool is_line_comment = false;

  DynBuffer *screen = dyn_buffer_new();
  // hide cursor to avoid possible tearing effect
  dyn_buffer_append(screen, "\033[?25l", 6);
  clear_screen(screen);
  // draw text
  for (size_t i = e->scrollv; i < e->lines->len && i < e->rows; i++) {
    // should be off at the end of the line
    is_line_comment = false;
    Line l = e->lines->buff[i];
    size_t len;
    if (l.len > e->cols) {
      len = e->cols;
    } else {
      len = l.len - e->scrollh;
    }
    size_t when_unhighlight = 0;
    for (size_t j = 0; j < len; j++) {
      char *curr = &e->text[l.start + j];
      if (is_block_comment) {
        if (STRLITCMP(curr, END_BLOCK_COMMENT)) {
          is_block_comment = false;
          // make sure the last quote still shows up
          when_unhighlight = 2;
        }
      } else if (!is_line_comment) {
        if (*curr == '"' || *curr == '\'') {
          if (is_quote) {
            is_quote = false;
            when_unhighlight = 1;
          } else
            is_quote = true;
        } else if (e->text[l.start + j] == '<')
          is_quote = true;
        else if (e->text[l.start + j] == '>') {
          is_quote = false;
          when_unhighlight = 1;
        } else if (!is_quote) {
					if (STRCMP(curr, LINE_COMMENT, 2))
          is_line_comment = true;
        else if (STRCMP(curr, START_BLOCK_COMMENT, 2))
          is_block_comment = true;
        else if (isdigit(*curr)) {
					if (when_unhighlight == 0) {
						dyn_buffer_append(screen, "\033[34m", 5);
            when_unhighlight = 1;
					}
        } else {
					size_t op_len = is_operator(curr, strlen(curr));
					if (op_len > 0) {
						if (when_unhighlight == 0) {
							dyn_buffer_append(screen, "\033[31m", 5);
							when_unhighlight = op_len;
						}
					} else {
						for (size_t i = 0; i < e->words->len; i++) {
							Word w = e->words->buff[i];
							if (w.start == l.start + j) {
								if (is_keyword(&e->text[w.start], w.len)) {
									if (when_unhighlight == 0) {
										 dyn_buffer_append(screen, "\033[31m", 5);
										 when_unhighlight = w.len;
									}
								}
							}
						}
					}
        }
				}
      }
      if (is_block_comment || is_line_comment)
        dyn_buffer_append(screen, "\033[0;90m", 7);
      else if (is_quote)
        dyn_buffer_append(screen, "\033[0;33m", 7);
      else if (when_unhighlight == 0) {
        dyn_buffer_append(screen, "\033[0m", 4);
      } else
        when_unhighlight -= 1;

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
