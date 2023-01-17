#pragma once
#include <stdlib.h>
#include <unistd.h>
#define DIE(str) do { perror(str); exit(1); } while (0)
#define READ(buff, count) read(STDIN_FILENO, buff, count)
#define WRITE(str, count) write(STDOUT_FILENO, str, count)
#define WRITE_LIT(lit) WRITE(lit, sizeof(lit) - 1)
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define CTRL(c) ((c) & 0x1f)
