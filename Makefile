include config.mk

SRCS = input.c jed.c output.c
NAME=jed
ifeq ($(DEBUG),1)
	FINAL_CFLAGS=$(CFLAGS) $(DEBUG_FLAGS)
else
	FINAL_CFLAGS=$(CFLAGS)
endif

all: $(NAME)
$(NAME): $(SRCS) config.h
	$(CC) $(FINAL_CFLAGS) $(SRCS) -o $(NAME)
clean:
	rm -f $(NAME)
run: $(NAME)
	./$(NAME)
