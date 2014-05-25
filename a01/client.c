#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include "common.h"

#define BUF_SIZE (1024)

void usage() {
	puts("usage: client host n_port msg");
	exit(-1);
}

int send_request(int serverfd){
	int count, retval = 0, request = 13;
	char buf[BUF_SIZE];

	snprintf(buf, BUF_SIZE, "%d", request);
	count = write(serverfd, buf, BUF_SIZE);
	if (count <= 0) {
		puts("Cannot send the request to the server");
		return -1;
	}

	return retval;
}

int get_port_and_send_msg(int serverfd){
	int retval = 0, count;
	char buf[BUF_SIZE];
	count = read(serverfd, buf, BUF_SIZE-1);
	if (count <= 0) {
		puts("Cannot read from the server");
		return -1;
	}
	puts(buf);
	return retval;
}

int resolve_and_connect(char *hostname, int port, int *serverfd){

	int temp_serverfd, retval;
	struct hostent* server_entity;
	struct sockaddr_in server_info = {0};

	server_entity =gethostbyname(hostname);
	if (server_entity == NULL) {
		puts("Cannot resolve hostname");
		return -1;
	}

	memcpy(&server_info.sin_addr, server_entity->h_addr, server_entity->h_length);
	/* printf("%d\n", server_info.sin_addr); */

	retval = make_socket(SOCK_STREAM, &temp_serverfd);
	if (retval < 0) {
		return retval;
	}

	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(port);

	retval = connect(temp_serverfd, (struct sockaddr *)&server_info, sizeof(server_info));
	if (retval < 0) {
		puts("Cannot connect to the server");
		close(temp_serverfd);
		return retval;
	}

	*serverfd = temp_serverfd;
	return retval;
}

int main(int argc, char **argv) {
	int serverfd, retval = 0;

	if (argc != 4) {
		usage();
	}


	retval = resolve_and_connect(argv[1], atoi(argv[2]), &serverfd);
	if (retval < 0){
		return retval;
	}

	retval = send_request(serverfd);
	if (retval < 0){
		goto MAIN_FINALLY;
	}

	retval = get_port_and_send_msg(serverfd);
	if (retval < 0){
		goto MAIN_FINALLY;
	}

	return retval;

MAIN_FINALLY:
	close(serverfd);
	return retval;
}
