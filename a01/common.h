#ifndef _COMMON_H_
#define _COMMON_H_
int make_socket(int type, int *serverfd);

int make_bind_socket(int port, int type, int *serverfd);
#endif
