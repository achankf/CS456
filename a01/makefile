CFLAGS = -std=c++98 -ggdb -pedantic-errors -Wall -Werror

all : 

.phony : client server

client : client.cc common.cc
	g++ $(CFLAGS) client.cc common.cc -o $@

server: server.cc common.cc
	g++ $(CFLAGS) server.cc common.cc -o $@
