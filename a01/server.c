#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

#define BUF_SIZE (1024)

int reply_udp_port(int clientfd) {
	int len, count, port, retval = 0;
	char buf[BUF_SIZE] = {0};

	int udp_socketfd, num_try = 0;

	while(true){
		port = 2000 + rand() % 20000;
		retval = make_bind_socket(port, SOCK_DGRAM, &udp_socketfd);
		if (retval >= 0){
			break;
		}
		if (num_try++ > 10){
			puts("Exceed max tries for binding the UDP socket to a port");
			return -1;
		}
	}

	len = snprintf(buf, BUF_SIZE, "%d", port);
	count = write(clientfd, buf, len+1);

	close(udp_socketfd);

	return retval + (len+count)*0;
}

int handle_request(int clientfd, int request_num) {
	int retval = 0;
	switch(request_num) {
	case 13:
		retval = reply_udp_port(clientfd);
		break;
	default:
		printf("Got invalid request num: %d", request_num);
		retval = -1;
	}
	return retval;
}

int get_requests(int serverfd) {
	int clientfd, retval = 0;
	char buf[BUF_SIZE];
	int count, request_num;
	while (true) {
		retval = clientfd = accept(serverfd, NULL, NULL);

		if (clientfd < 0) {
			continue;
		}

		puts("Client connected");

		memset(buf, 0, BUF_SIZE);
		count = read(clientfd, buf, BUF_SIZE);
		if (count <= 0) {
			puts("Cannot read client request");
			goto BAD_CLIENT_REQUEST;
		}

		request_num = atoi(buf);
		printf("GOT request: %d\n", request_num);

		handle_request(clientfd, request_num);

BAD_CLIENT_REQUEST:
		puts("Disconnecting client");
		close (clientfd);
	}

	/* unreachable */
	return retval;
}

int main(int argc, char **argv) {
	int serverfd, retval = 0;
	int port;

	/* set a random seed */
	srand (time(NULL));
	port = 2000 + rand() % 20000;

	retval = make_bind_socket(port, SOCK_STREAM, &serverfd);
	if (retval < 0) {
		return retval;
	}

	retval = listen(serverfd, 100);
	if (retval < 0) {
		puts("Cannot listen to server's fd");
		return retval;
	}

	printf("Port opened: %d\n", port);
	retval = get_requests(serverfd);
	/* unreachable */
	return retval;
}
