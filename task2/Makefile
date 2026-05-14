override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wvla -Wno-unused-parameter -g -O0 -fsanitize=address,undefined,leak

NAME=sop-hre

.PHONY: clean all

all: ${NAME}

${NAME}: ${NAME}.c l7_common.h
	$(CC) $(CFLAGS) -o ${NAME} ${NAME}.c

clean:
	rm -f ${NAME}
