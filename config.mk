# jed version
VERSION = 

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
CFLAGS = -Wextra -Wall -Wpedantic -fsanitize=address,leak,undefined -ggdb
LDFLAGS = 

# compiler and linker
CC = cc
