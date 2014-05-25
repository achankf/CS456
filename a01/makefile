CFLAGS = -pedantic -ggdb -pedantic-errors -Wall -Werror

all : client server

client : client.cc common.o
	gcc $(CFLAGS) $< common.o -o $@

server: server.cc common.o
	g++ $(CFLAGS) $< common.o -o $@

common.o : common.cc
	gcc $(CFLAGS) $< -c
