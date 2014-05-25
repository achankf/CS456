#ifndef _COMMON_H_
#define _COMMON_H_

#define REQUEST (13)
#define BUF_SIZE (1024)
int make_socket(int type, int *serverfd);

int make_bind_socket(int port, int type, int *serverfd);

int make_udp_socket(int *fd, int *port);
#endif
