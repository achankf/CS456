#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include "common.h"

static void usage() {
	puts("usage: client host n_port msg");
	exit(-1);
}

/** set up the server address structure */
static void make_server_info(int ip, int port, struct sockaddr_in *server_info){
	memset(server_info, 0, sizeof(struct sockaddr_in));
	server_info->sin_family = AF_INET;
	server_info->sin_port = htons(port);
	memcpy(&server_info->sin_addr, &ip, sizeof(int));
}

/** Turn hostname into its IP address */
static int map_hostname(char *hostname, int *ip){
	struct hostent* server_entity;
	
	server_entity =gethostbyname(hostname);
	if (server_entity == NULL) {
		puts("Cannot resolve hostname");
		return -1;
	}

	memcpy(ip, server_entity->h_addr, server_entity->h_length);
	return 0;
}

static int start_negotiation(int serverfd) {
	int count;
	char buf[2];

	snprintf(buf, BUF_SIZE, "%c", REQUEST);
	count = write(serverfd, buf, sizeof(buf));
	if (count <= 0) {
		puts("Cannot send the request to the server");
		return -1;
	}

	return 0;
}

static int get_udp_port(int serverfd, int *udp_port) {
	int count;
	char buf[5];
	count = read(serverfd, buf, sizeof(buf) -1);
	if (count <= 0) {
		puts("Cannot read from the server");
		return -1;
	}
	*udp_port = ntohl(*(int*)buf);
	return 0;
}

static int send_and_recieve_msg(int ip, int port, char *msg){
	struct sockaddr_in server_info;
	int udp_socketfd;
	char buf[BUF_SIZE] = {0};

	make_server_info(ip, port, &server_info);
	make_bind_socket(0, SOCK_DGRAM, &udp_socketfd);

	/* send message and recieve results */
	sendto(udp_socketfd, msg, strlen(msg), 0, (struct sockaddr*) &server_info, sizeof(struct sockaddr_in));
	recvfrom(udp_socketfd, buf, BUF_SIZE, 0, NULL, NULL);

	/* print the result */
	puts(buf);

	return 0;
}


static int make_tcp_connection(int ip, int port, int *serverfd) {

	int temp_serverfd;
	struct sockaddr_in server_info = {0};

	if (make_socket(SOCK_STREAM, &temp_serverfd) < 0) {
		return -1;
	}

	make_server_info(ip, port, &server_info);

	if (connect(temp_serverfd, (struct sockaddr *)&server_info, sizeof(server_info)) < 0) {
		puts("Cannot connect to the server");
		close(temp_serverfd);
		return -1;
	}

	*serverfd = temp_serverfd;
	return 0;
}

int main(int argc, char **argv) {
	int serverfd, ip, udp_port;

	if (argc != 4) {
		usage();
	}

	/* create tcp connection */
	if (map_hostname(argv[1], &ip) < 0
	 || make_tcp_connection(ip, atoi(argv[2]), &serverfd) < 0) {
		return -1;
	}

	/* negotiate the udp port */
	if (start_negotiation(serverfd) < 0
		|| get_udp_port(serverfd, &udp_port) < 0) {
		close(serverfd);
		return -1;
	}

	/* we're done with the tcp negotiation */
	close(serverfd);

	return send_and_recieve_msg(ip, udp_port, argv[3]);
}
