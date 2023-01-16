#pragma once
#include "jed.h"

typedef enum {
	MOVE_DOWN,
	MOVE_UP,
	MOVE_LEFT,
	MOVE_RIGHT,
} SpecialKey;


void handle_input(Editor *e);
