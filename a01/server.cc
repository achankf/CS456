#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

#include <string> /* std::string */
#include <algorithm> /* std::reverse */

/* UDP socket has been opened... send the reversed message. Then close the socket and continue getting more requests from TCP  */
static int get_and_reply_msg(int udp_socketfd) {
	char buf[BUF_SIZE] = {0};
	struct sockaddr_in client_info;
	socklen_t info_len = sizeof(struct sockaddr_in);

	recvfrom(udp_socketfd, buf, BUF_SIZE, 0, (struct sockaddr*) &client_info, &info_len);

	/* do the reverse */
	std::string msg(buf);
	std::reverse(msg.begin(), msg.end());

	sendto(udp_socketfd, msg.c_str(), msg.size(), 0, (struct sockaddr*) &client_info, info_len);
	return 0;
}

/* Complete the UDP port negotiation by openning a socket & sending back the port number */
static int reply_udp_port(int clientfd) {
	int count, port, udp_socketfd;
	char port_buf[BUF_SIZE] = {0};

	make_udp_socket(&udp_socketfd, &port);
	port = htonl(port);
	memcpy(port_buf, &port, sizeof(int));

	count = write(clientfd, port_buf, sizeof(port_buf));
	if (count <= 0) {
		puts("Cannot write the udp port to the client");
		close(udp_socketfd);
		return -1;
	}

	printf("Reply with port: %d\n", ntohl(*(int*)port_buf));
	get_and_reply_msg(udp_socketfd);

	puts("Closing udp socket");
	close(udp_socketfd);

	return 0;
}

/* Handle requests (in our case, it would be the number 13) */
static int handle_requests(int serverfd) {
	int clientfd, count, request_num;
	char buf[BUF_SIZE];

	clientfd = accept(serverfd, NULL, NULL);

	if (clientfd < 0) {
		return -1;
	}

	puts("Client connected");

	memset(buf, 0, BUF_SIZE);
	/* read one character (i.e. 13) */
	count = read(clientfd, buf, 1);
	if (count <= 0) {
		puts("Cannot read client request");
		close (clientfd);
		return -1;
	}

	/* get and process request number */
	request_num = buf[0];
	printf("GOT request: %d\n", request_num);

	if (request_num != REQUEST) {
		printf("Got invalid request num: %d\n", request_num);
		close (clientfd);
		return -1;
	}

	if (reply_udp_port(clientfd) < 0) {
		close (clientfd);
		return -1;
	}

	puts("Disconnecting client");
	close (clientfd);
	return 0;
}

int main(int argc, char **argv) {
	int serverfd, port;

	/* set a random seed */
	srand (time(NULL));
	port = 2000 + rand() % 20000;

	/* make and bind to a socket */
	if (make_bind_socket(port, SOCK_STREAM, &serverfd) < 0) {
		return -1;
	}

	/* listen to the socket */
	if (listen(serverfd, 100) < 0) {
		puts("Cannot listen to server's fd");
		close(serverfd);
		return -1;
	}

	printf("n_port=%d\n", port);

	while (true) {
		handle_requests(serverfd);
	}

	/* must not reach */
	assert(false);
	return -1;
}
