#ifndef __struct_h_
#define __struct_h_

#include <stddef.h>
#include <stdint.h>

#define NBR_ROUTER 5
#define BUF_SIZE 100
#define L2R_SIZE 100
#define IFNTY 10000

/* forward declarations */
struct sockaddr_in;
struct pkt_HELLO;
struct pkt_LSPDU;
struct pkt_INIT;
struct link_cost;
struct circuit_DB;

struct node;

int make_routing_table(uint32_t rid, int router_fd, struct sockaddr_in *nse_info);
#endif
