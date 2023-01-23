#pragma once
#include <stdlib.h>
#include <unistd.h>
#define DIE(str) do { perror(str); exit(1); } while (0)
#define READ(buff, count) read(STDIN_FILENO, buff, count)
#define WRITE(str, count) write(STDOUT_FILENO, str, count)
#define WRITE_LIT(lit) WRITE(lit, sizeof(lit) - 1)
#if DEBUG == 1
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...) do {} while (0)
#endif
#define CTRL(c) ((c) & 0x1f)
#define ARRLEN(arr) (sizeof((arr)) / sizeof((arr)[0]))
#include <string.h>
#define STRCMP(str1, str2, count) memcmp((str1), (str2), (count)) == 0
#define STRLITCMP(str1, lit) memcmp((str1), (lit), sizeof((lit)) - 1) == 0
