override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wvla -Wno-unused-parameter -g -O0

ifdef CI
override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wvla -Werror -Wno-unused-parameter
endif

TARGETS=sop-crone sop-witch

.PHONY: all clean

all: $(TARGETS)

sop-crone: sop-crone.c l7-common.h
	$(CC) $(CFLAGS) -o sop-crone sop-crone.c

sop-witch: sop-witch.c l7-common.h
	$(CC) $(CFLAGS) -o sop-witch sop-witch.c

clean:
	rm -f $(TARGETS)
