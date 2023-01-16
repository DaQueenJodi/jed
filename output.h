#pragma once
#include "jed.h"
typedef struct {
	char *text;
	size_t len;
	size_t cap;
} DynBuffer;


void handle_output(Editor *e);
int get_screen_size(int *c, int *r);


