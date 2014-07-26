#include <stdlib.h>
#include "common.h"
#include "routing.h"
#include <unistd.h>
#include <assert.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <cstdio>

void usage(char *name) {
	assert(name != NULL);
	printf("%s <router_id> <nse_host> <nse_port> <router_port>\n", name);
}

int main(int argc, char **argv) {
	int rid, nse_ip, nse_port, router_port, router_fd;
	struct sockaddr_in nse_info;

	if (argc != 5
		|| (rid = atoi(argv[1])) == 0
		|| map_hostname(argv[2], &nse_ip) < 0
		|| (nse_port = atoi(argv[3])) == 0
		|| (router_port = atoi(argv[4])) == 0) {
		usage(argv[0]);
		return -1;
	}

	assert((unsigned int) rid < 0xffff
		&& (unsigned int) nse_port < 0xffff
		&& (unsigned int) router_port < 0xffff);

	if (make_bind_udp_socket(router_port, &router_fd) < 0) {
		printf("%s. Errmsg: %s\n", "Cannot create and bind UDP socket", strerror(errno));
		return -1;
	}

	make_server_info(nse_ip, nse_port, &nse_info);

	if (make_routing_table(rid, router_fd, &nse_info) < 0) {
		printf("%s. Errmsg: %s\n", "Cannot make the routing table", strerror(errno));
		close(router_fd);
		return -1;
	}

	/* must not reach here */
	assert(false);
	return close(router_fd);
}
