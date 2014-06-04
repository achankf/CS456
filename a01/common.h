#ifndef _COMMON_H_
#define _COMMON_H_

#define REQUEST (13)
#define BUF_SIZE (1024)

/* generic methods for creating sockets */

int make_socket(int type, int *serverfd);

/* client needs to create a UDP socket too */
int make_bind_socket(int port, int type, int *serverfd);

/* this is probably not so generic */
int make_udp_socket(int *fd, int *port);
#endif
