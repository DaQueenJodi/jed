#pragma once
#include "jed.h"
typedef struct {
	char *text;
	size_t len;
} DynBuffer;


void handle_output(Editor *e);
int set_screen_size(Editor *e);
int dyn_buffer_append(DynBuffer *db, char *str, size_t len);
DynBuffer *dyn_buffer_new(void);

