# jed
# See LICENSE file for license details.

include config.mk

SRC = input.c jed.c output.c
BIN = jed

all: ${BIN}

config.h:
	cp config.def.h $@

${BIN}: ${SRCS} config.h
	${CC} ${CFLAGS} ${LDFLAGS} ${SRC} -o ${BIN}

run: ${BIN}
	./${BIN}

clean:
	rm -r ${BIN}

install: ${BIN}
	cp jed ${PREFIX}/bin
