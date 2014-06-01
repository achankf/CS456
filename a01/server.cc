#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

#include <string>
#include <algorithm> /* std::reverse */

int get_and_reply_msg(int udp_socketfd){
	int retval = 0;
	char buf[BUF_SIZE] = {0};
	struct sockaddr_in client_info;
	socklen_t info_len = sizeof(struct sockaddr_in);
	recvfrom(udp_socketfd, buf, BUF_SIZE, 0, (struct sockaddr*) &client_info, &info_len);

	std::string msg(buf);
	std::reverse(msg.begin(), msg.end());
	puts(msg.c_str());

	sendto(udp_socketfd, msg.c_str(), msg.size(), 0, (struct sockaddr*) &client_info, info_len);
	return retval;
}

int reply_udp_port(int clientfd, int *udp_socketfd) {
	int count, len, retval = 0;
	char buf[BUF_SIZE] = {0};

	int udp_port;

	make_udp_socket(udp_socketfd, &udp_port);

	len = snprintf(buf, BUF_SIZE, "%d", udp_port);
	count = write(clientfd, buf, len+1);
	if (count <= 0) {
		puts("Cannot write the udp port to the client");
		close(*udp_socketfd);
		return -1;
	}
	
	get_and_reply_msg(*udp_socketfd);

	return retval;
}

int get_requests(int serverfd, int *udp_socketfd) {
	int clientfd, retval = 0;
	char buf[BUF_SIZE];
	int count, request_num;

	retval = clientfd = accept(serverfd, NULL, NULL);

	if (clientfd < 0) {
		return retval;
	}

	puts("Client connected");

	memset(buf, 0, BUF_SIZE);
	count = read(clientfd, buf, BUF_SIZE);
	if (count <= 0) {
		puts("Cannot read client request");
		goto BAD_CLIENT_REQUEST;
	}

	request_num = buf[0];
	printf("GOT request: %d\n", request_num);

	if (request_num != REQUEST) {
		printf("Got invalid request num: %d", request_num);
		retval = -1;
		goto BAD_CLIENT_REQUEST;
	}

	retval = reply_udp_port(clientfd, udp_socketfd);
	if (retval < 0) {
		goto BAD_CLIENT_REQUEST;
	}

	return retval;

BAD_CLIENT_REQUEST:
	puts("Disconnecting client");
	close (clientfd);

	/* unreachable */
	return retval;
}

int main(int argc, char **argv) {
	int serverfd, udp_socketfd, retval = 0;
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
		goto MAIN_BAD_DEALLOC;
	}

	printf("Port opened: %d\n", port);

	while (true) {
		/* ignore retval */
		get_requests(serverfd, &udp_socketfd);
	}

	/* not reached */
	close(serverfd);
	return retval;

MAIN_BAD_DEALLOC:
	close(serverfd);
	return retval;
}
