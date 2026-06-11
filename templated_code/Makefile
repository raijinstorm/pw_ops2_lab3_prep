override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wvla -Wno-unused-parameter -Wno-unused-const-variable -g -O0 -fsanitize=address,undefined,leak

ifdef CI
override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wvla -Werror -Wno-unused-parameter -Wno-unused-const-variable
endif

TARGETS=server_text client_text server_binary_fixed client_binary_fixed server_binary_framed client_binary_framed

.PHONY: all clean

all: ${TARGETS}

%: %.c common.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f ${TARGETS}
