#ifndef __common_h_
#define __common_h_

#include "stdio.h"

/* Macro method for DEBUG, ERROR */
#ifndef NDEBUG
#define DEBUG(fmt, ...)	printf("[\033[0;33mDEBUG\033[0m] " fmt, __VA_ARGS__)
#else
#define DEBUG(fmt, ...) while(0){}
#endif
#define ERROR(fmt, ...) printf("[\033[0;31mERROR\033[0m] " fmt, __VA_ARGS__)

/* forward declarations */
struct sockaddr_in;

int map_hostname(char *hostname, int *ret);

/* generic methods for creating sockets */
void make_server_info(int ip, int port, struct sockaddr_in *server_info);
int make_socket(int type, int *serverfd);
int make_bind_udp_socket(int port, int *serverfd);

#endif
