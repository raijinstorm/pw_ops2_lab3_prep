CC = gcc
CFLAGS = -Wall -Wextra -std=c17

all: server client

server: server.c l7_common.h
	$(CC) $(CFLAGS) -o server server.c

client: client.c l7_common.h
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client
