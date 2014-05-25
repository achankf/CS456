#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include "common.h"

void usage() {
	puts("usage: client host n_port msg");
	exit(-1);
}

void make_server_info(int ip, int port, struct sockaddr_in *server_info){
	memset(server_info, 0, sizeof(struct sockaddr_in));
	server_info->sin_family = AF_INET;
	server_info->sin_port = htons(port);
	memcpy(&server_info->sin_addr, &ip, sizeof(int));
}

int map_hostname(char *hostname, int *ip){
	struct hostent* server_entity;
	
	server_entity =gethostbyname(hostname);
	if (server_entity == NULL) {
		puts("Cannot resolve hostname");
		return -1;
	}

	memcpy(ip, server_entity->h_addr, server_entity->h_length);
	return 0;
}

int send_request(int serverfd) {
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

int get_udp_port(int serverfd, int *udp_port) {
	int retval = 0, count;
	char buf[BUF_SIZE];
	count = read(serverfd, buf, BUF_SIZE-1);
	if (count <= 0) {
		puts("Cannot read from the server");
		return -1;
	}
	*udp_port = atoi(buf);
	return retval;
}

int send_and_recieve_msg(int ip, int port, char *msg){
	int retval = 0;
	struct sockaddr_in server_info;
	int udp_socketfd;
	char buf[BUF_SIZE] = {0};

	make_server_info(ip, port, &server_info);
	make_bind_socket(0, SOCK_DGRAM, &udp_socketfd);

	sendto(udp_socketfd, msg, strlen(msg), 0, (struct sockaddr*) &server_info, sizeof(struct sockaddr_in));

	recvfrom(udp_socketfd, buf, BUF_SIZE, 0, NULL, NULL);

	puts(buf);

	return retval;
}


int connect_server(int ip, int port, int *serverfd) {

	int temp_serverfd, retval = 0;
	struct sockaddr_in server_info = {0};

	retval = make_socket(SOCK_STREAM, &temp_serverfd);
	if (retval < 0) {
		return retval;
	}

	make_server_info(ip, port, &server_info);

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
	int serverfd, retval = 0, ip, udp_port;

	if (argc != 4) {
		usage();
	}

	retval = map_hostname(argv[1], &ip);
	if (retval < 0){
		return retval;
	}

	retval = connect_server(ip, atoi(argv[2]), &serverfd);
	if (retval < 0) {
		return retval;
	}

	retval = send_request(serverfd);
	if (retval < 0) {
		goto MAIN_BAD_DEALLOC;
	}

	retval = get_udp_port(serverfd, &udp_port);
	if (retval < 0) {
		goto MAIN_BAD_DEALLOC;
	}

	close(serverfd);

	retval = send_and_recieve_msg(ip, udp_port, argv[3]);
	/* ignore return value as we are done */

	return retval;

MAIN_BAD_DEALLOC:
	close(serverfd);
	return retval;
}
