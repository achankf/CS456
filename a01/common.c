#include "common.h"
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>

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

	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	server_info.sin_port = htons(port);

	retval = temp_serverfd = socket(AF_INET, type, 0);
	if (temp_serverfd < 0) {
		puts("Cannot open socket");
		return retval;
	}

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
