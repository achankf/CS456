#include "common.h"
#include <netdb.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

/** Turn hostname into its IP address */
int map_hostname(char *hostname, int *ret) {
  struct hostent* server_entity;

  assert(hostname != NULL);
  assert(ret != NULL);

  server_entity = gethostbyname(hostname);
  if (server_entity == NULL) {
    return -1; 
  }

  memcpy(ret, server_entity->h_addr_list[0], server_entity->h_length);
  return 0;
}

int make_socket(int type, int *serverfd) {
	int retval = 0;
	int temp_serverfd;

	assert(serverfd != NULL);

	retval = temp_serverfd = socket(AF_INET, type, 0);
	if (temp_serverfd < 0) {
		return retval;
	}

	/* OKAY */
	*serverfd = temp_serverfd;
	return retval;
}

int make_bind_udp_socket(int port, int *serverfd) {
	int temp_serverfd;
	struct sockaddr_in server_info = {0};

	assert(serverfd != NULL);

	if (make_socket(SOCK_DGRAM, &temp_serverfd) < 0) {
		return -1;
	}

	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
  server_info.sin_port = htons(port); 

	if (bind(temp_serverfd, (struct sockaddr*) &server_info, sizeof(server_info)) < 0) {
		close(temp_serverfd);
		return -1;
	}

	/* OKAY */
	*serverfd = temp_serverfd;
	return 0;
}

/** set up the server address structure */
void make_server_info(int ip, int port, struct sockaddr_in *server_info) {
	assert(server_info != NULL);
	memset(server_info, 0, sizeof(struct sockaddr_in));
	server_info->sin_family = AF_INET;
	server_info->sin_port = htons(port);
	memcpy(&server_info->sin_addr, &ip, sizeof(int));
}
