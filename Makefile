CC=gcc
CFLAGS= -Wall -pedantic --std=gnu99 -g

.PHONY: all clean
.DEFAULT_GOAL := all

all: server client #steg

server: server.o general.o
	$(CC) $(CFLAGS) server.o general.o -o stegserver -lcrypto

client: client.o general.o
	$(CC) $(CFLAGS) client.o general.o -o stegclient

clean:
	rm -f client server steg *.o
