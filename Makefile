CC = gcc
CFLAGS = -Wall -g

.PHONY: all clean

all: Client Server

Client: Client.c
	$(CC) $(CFLAGS) -o Client Client.c

Server: Server.c
	$(CC) $(CFLAGS) -o Server Server.c

clean:
	rm -f Client Server
