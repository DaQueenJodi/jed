#pragma once
#include "jed.h"
typedef struct {
	char *text;
	size_t len;
} DynBuffer;


void handle_output(Editor *e);
int set_screen_size(Editor *e);

DynBuffer *dyn_buffer_new(void);
int dyn_buffer_append(DynBuffer *db, char *str, size_t len);
void dyn_buffer_free(DynBuffer *db);

