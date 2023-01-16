SRCS=$(shell find -name "*.c")
DEBUG=-fsanitize=address,leak,undefined -ggdb
CFLAGS=-Wextra -Wall -Wpedantic -Werror $(DEBUG)
LDFLAGS=
CC=clang
NAME=jed

all: $(NAME)
$(NAME): $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $(NAME)

run: $(NAME)
	./$(NAME)
