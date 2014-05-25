#include "common.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

int make_socket(int type, int *serverfd) {
	int retval = 0;
	int temp_serverfd;

	retval = temp_serverfd = socket(AF_INET, type, 0);
	if (temp_serverfd < 0) {
		puts("Cannot open socket");
		return retval;
	}

	/* OKAY */
	*serverfd = temp_serverfd;
	return retval;
}

int make_bind_socket(int port, int type, int *serverfd) {
	int retval = 0;
	int temp_serverfd;
	struct sockaddr_in server_info = {0};

	retval = make_socket(type, &temp_serverfd);
	if (retval < 0) {
		return retval;
	}

	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	server_info.sin_port = htons(port);

	retval = bind(temp_serverfd, (struct sockaddr*) &server_info, sizeof(server_info));
	if (retval < 0) {
		puts("Cannot do binding");
		close(temp_serverfd);
		return retval;
	}

	/* OKAY */
	*serverfd = temp_serverfd;
	return retval;
}

int make_udp_socket(int *fd, int *port){
	int retval = 0;
	int num_try = 0;
	while(true) {
		*port = 2000 + rand() % 20000;
		retval = make_bind_socket(*port, SOCK_DGRAM, fd);
		if (retval >= 0) {
			break;
		}
		if (num_try++ > 10) {
			puts("Exceed max tries for binding the UDP socket to a port");
			return -1;
		}
	}
	return retval;
}
