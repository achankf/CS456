CFLAGS = -pedantic -ggdb -pedantic-errors -Wall -Werror

all : client server

client : client.c common.o
	gcc $(CFLAGS) $< common.o -o $@

server: server.c common.o
	gcc $(CFLAGS) $< common.o -o $@

common.o : common.c
	gcc $(CFLAGS) $< -c
