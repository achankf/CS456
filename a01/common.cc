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
	int retval = 0, i = 0, num_tries = 10000;
	int temp_serverfd;
	struct sockaddr_in server_info = {0};

	retval = make_socket(type, &temp_serverfd);
	if (retval < 0) {
		return retval;
	}

	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);

	while (true) {
		server_info.sin_port = htons(port + i);
		retval = bind(temp_serverfd, (struct sockaddr*) &server_info, sizeof(server_info));

		if (retval >= 0) {
			break;
		}

		/* cannot bind and has reach trying limit */
		if (num_tries < 0) {
			puts("Cannot do binding");
			close(temp_serverfd);
			return retval;
		}

		/* otherwise continue binding with the next port value */
		i++;
	}

	/* OKAY */
	*serverfd = temp_serverfd;
	return retval;
}

int make_udp_socket(int *fd, int *port) {
	int retval = 0, i = 0, num_tries = 10000;

	*port = 2000 + rand() % 20000;

	while(true) {
		port += i;
		retval = make_bind_socket(*port, SOCK_DGRAM, fd);
		if (retval >= 0) {
			break;
		}
		if (num_tries < 0) {
			puts("Exceed max tries for binding the UDP socket to a port");
			return -1;
		}
		i++;
	}
	return retval;
}
