SRCS=$(shell find -name "*.c")
DEBUG=-fsanitize=address,leak,undefined -ggdb
CFLAGS=-Wextra -Wall -Wpedantic $(DEBUG)
LDFLAGS=
CC=clang
NAME=jed

all: $(NAME)
$(NAME): $(SRCS) config.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $(NAME)

run: $(NAME)
	./$(NAME)
