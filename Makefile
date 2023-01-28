CC=gcc
CFLAGS= -Wall -pedantic --std=gnu99 -g

.PHONY: all clean
.DEFAULT_GOAL := all

all: server #client steg

server: server.o
	$(CC) $(CFLAGS) server.o -o server

clean:
	rm -f client server steg *.o
