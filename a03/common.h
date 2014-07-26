#ifndef __common_h_
#define __common_h_

/* forward declarations */
struct sockaddr_in;

int map_hostname(char *hostname, int *ret);

/* generic methods for creating sockets */
void make_server_info(int ip, int port, struct sockaddr_in *server_info);
int make_socket(int type, int *serverfd);
int make_bind_udp_socket(int port, int *serverfd);

#endif
