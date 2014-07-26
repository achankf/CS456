#include "routing.h"
#include "common.h"
#include <netinet/in.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <time.h>

/* structures from the hints */
struct pkt_HELLO {
	uint32_t rid;
	uint32_t link_id;
};

struct pkt_LSPDU {
	uint32_t sender;
	uint32_t rid;
	uint32_t link_id;
	uint32_t cost;
	uint32_t via;
};

struct pkt_INIT {
	uint32_t rid;
};

struct link_cost {
	uint32_t link_id;
	uint32_t cost;
};

struct circuit_DB {
	uint32_t nbr_link;
	struct link_cost linkcost[NBR_ROUTER];
};

/* forward declarations from static methods */
static int send_init(uint32_t rid, int router_fd, struct sockaddr_in *nse_info);
static int send_hello(uint32_t rid, int router_fd, uint32_t link_id, struct sockaddr_in *nse_info);

/* Real implementation */
static int send_init(uint32_t rid, int router_fd, struct sockaddr_in *nse_info) {
	struct pkt_INIT buf = {rid};
	assert(nse_info != NULL);
	return sendto(router_fd, &buf, sizeof(buf), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
}

static int send_hello(uint32_t rid, int router_fd, uint32_t link_id, struct sockaddr_in *nse_info) {
	struct pkt_HELLO buf = {rid, link_id};
	assert(nse_info != NULL);
	return sendto(router_fd, &buf, sizeof(buf), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
}

static void flood_lspdu(int router_fd, struct sockaddr_in *nse_info, uint32_t rid, uint32_t target_link, struct circuit_DB topology[NBR_ROUTER]){
	struct pkt_LSPDU lspdu;
	int i, j, k;

	/* reply with LSPDU's */
	lspdu.sender = rid;
	for (k = 0; k < topology[rid].nbr_link; k++) {
		lspdu.via = topology[rid].linkcost[k].link_id;

		/* only reply to the router that shares the same link as from the hello pkt */
		if (target_link != lspdu.via) {
			continue;
		}

		for (i = 1; i <= NBR_ROUTER; i++) {
			lspdu.rid = i;
			for (j = 0; j < topology[i].nbr_link; j++) {
				lspdu.link_id = topology[i].linkcost[j].link_id;
				lspdu.cost = topology[i].linkcost[j].cost;
				sendto(router_fd, &lspdu, sizeof(lspdu), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
			}
		}
	}
}

static bool update_topology(struct circuit_DB topology[NBR_ROUTER], struct pkt_LSPDU *lspdu) {
	struct circuit_DB *cur_node = &topology[lspdu->rid];
	int i;

	int j;
	assert(lspdu != NULL);
	assert(lspdu->link_id != 0);

	DEBUG("Neighbours in rid: %d\n", lspdu->rid);
	for (j = 0; j < cur_node->nbr_link; j++) {
		DEBUG("\tindex %d, link id %d, cost %d\n", j, cur_node->linkcost[j].link_id, cur_node->linkcost[j].cost);
	}

	/* if there is a record in the existing topology, ignore the lspdu  */
	for (i = 0; i < cur_node->nbr_link; i++) {
		DEBUG("got: %d, expect: %d\n", cur_node->linkcost[i].link_id, lspdu->link_id);
		if (cur_node->linkcost[i].link_id == lspdu->link_id) {
			DEBUG("Duplicate entry found at index %d: link id %d, old cost %d -> new cost %d\n", i, lspdu->link_id, cur_node->linkcost[i].cost, lspdu->cost);
			if (cur_node->linkcost[i].cost != lspdu->cost) {
				ERROR("ERROR rid: %d\n", lspdu->rid);
				for (j = 0; j < cur_node->nbr_link; j++) {
					ERROR("\tindex %d, link id %d, cost %d\n", j, cur_node->linkcost[j].link_id, cur_node->linkcost[j].cost);
				}
				ERROR("%s\n", "Cost not equal");
			}
			assert(cur_node->linkcost[i].cost == lspdu->cost);
			return false;
		}
	}

	DEBUG("adding (link_id, cost) = (%d, %d) for router %d, nbr_link: %d\n", lspdu->link_id, lspdu->cost, lspdu->rid, cur_node->nbr_link);
	if (cur_node->nbr_link > NBR_ROUTER -1) {
		ERROR("%s\n", "Exceed max router");
	}
	assert(cur_node->nbr_link <= NBR_ROUTER);

	cur_node->linkcost[cur_node->nbr_link].link_id = lspdu->link_id;
	cur_node->linkcost[cur_node->nbr_link].cost = lspdu->cost;
	cur_node->nbr_link++;
	return true;
}

bool sent[100][100] = {{0}};

#if 0
static void forward_lspdu(uint32_t rid, int router_fd, struct sockaddr_in *nse_info, struct pkt_LSPDU *lspdu, struct circuit_DB topology[NBR_ROUTER]) {
	int i;
	uint32_t neighbour_link;
	struct pkt_LSPDU copy = *lspdu;

	copy.sender = rid;

	/* for each neighbour */
	for (i = 0; i < topology[rid].nbr_link; i++) {
		neighbour_link = topology[rid].linkcost[i].link_id;

		/* skip sending to routers that are directly connected to the link or to the sender's link */
		if (sent[lspdu->link_id][neighbour_link]) {
			continue;
		}
		sent[lspdu->link_id][neighbour_link] = true;

		copy.via = neighbour_link;

		sendto(router_fd, &copy, sizeof(copy), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
	}
}
#endif

static void create_adj_matrix(struct circuit_DB topology[NBR_ROUTER], bool adj_mat[NBR_ROUTER+1][NBR_ROUTER+1], uint32_t cost_mat[NBR_ROUTER+1][NBR_ROUTER+1]) {
	int i, j;
	int link[L2R_SIZE] = {0};
	int temp_link, target_router;

	for (i = 0; i <= NBR_ROUTER; i++) {
		for (j = 0; j <= NBR_ROUTER; j++) {
			adj_mat[i][j] = 0;
			cost_mat[i][j] = 0;
		}
	}
	
	for (i = 1; i <= NBR_ROUTER; i++) {
		for (j = 0; j < topology[i].nbr_link; j++) {
			temp_link = topology[i].linkcost[j].link_id;
			target_router = link[temp_link];
			if (target_router != 0) {
				adj_mat[i][target_router] = adj_mat[target_router][i] = true;
				cost_mat[i][target_router] = cost_mat[target_router][i] = topology[i].linkcost[j].cost;
			} else {
				link[temp_link] = i;
			}
		}
	}

#if 0
	puts("ADJ LIST");
	for (i = 1; i <= NBR_ROUTER; i++) {
		printf("R%d -> ", i);
		for (j = 0; j < NBR_ROUTER; j++) {
			if (adj_mat[i][j]) {
				printf("%d, ", j);
			}
		}
		puts("");
	}
#endif
}

static void dijkstra(struct circuit_DB topology[NBR_ROUTER], uint32_t rid, uint32_t dest_rid) {
	uint32_t i, j, k;
	uint32_t min_dist;
	uint32_t cur_router;

	uint32_t queue[NBR_ROUTER];
	uint32_t q_head = 0;

	uint32_t dist[NBR_ROUTER + 1];
	uint32_t prev[NBR_ROUTER + 1] = {0};

	uint32_t alt, cur_link, target_router, target_link_idx, prev_router;

	bool in_queue;

	bool adj_mat[NBR_ROUTER + 1][NBR_ROUTER + 1];
	uint32_t cost_mat[NBR_ROUTER + 1][NBR_ROUTER + 1];

	create_adj_matrix(topology, adj_mat, cost_mat);
return;

	if (rid == dest_rid) {
		DEBUG("Dijkstra: (LOCAL) src:%d, dst:%d, cost:0\n", rid, rid);
		return;
	}

	/* set up book-keeping arrays */
	for (i = 1; i <= NBR_ROUTER; i++) {
		dist[i] = IFNTY;
		queue[i-1] = i;
	}

	for (i = 0; i < topology[rid].nbr_link; i++) {
	}
	q_head = NBR_ROUTER;
	dist[rid] = 0;

	while (q_head > 0) {
		// brute-force way of doing extract-min
		cur_router = queue[0];
		min_dist = dist[cur_router];
		DEBUG("q_head:%d\n", q_head);
		for (i = 0; i < q_head; i++) {
			if (dist[queue[i]] < min_dist) {
				cur_router = queue[i];
				min_dist = dist[cur_router];
				DEBUG("dist[cur_router]:%d\n", dist[cur_router]);
			}
		}

		j = 0;
		for (i = 0; i < q_head; i++) {
			queue[j] = queue[i];
			if (queue[i] != cur_router) {
				j++;
			}
		}
		q_head--;
		DEBUG("Dijkstra: cur_router:%d, min_dist:%d\n", cur_router, min_dist);

		printf("Q: ");
		for (i = 0; i < q_head; i++) {
			printf("%d, ", queue[i]);
		}
		puts("");

		// for each neighbour
		for (i = 0; i < topology[cur_router].nbr_link; i++) {
			cur_link = topology[cur_router].linkcost[i].link_id;

			target_router = 0;

			for (j = 1; j <= NBR_ROUTER; j++) {
				in_queue = false;
				for (k = 0; k < q_head; k++) {
					if (queue[k] == j) {
						in_queue = true;
					}
				}

				if (!in_queue) {
					DEBUG("Dijkstra: continued, j:%d, cur_counter:%d\n", j, cur_router);
					continue;
				}

				ERROR("Founding target link %d\n", cur_link);
				for (k = 0; k < topology[j].nbr_link; k++) {
					if (cur_link == topology[j].linkcost[k].link_id) {
						target_router = j;
						target_link_idx = k;
					}
				}
			}

			if (target_router == 0) {
				DEBUG("%s\n", "Dijkstra: Next router not found");
				continue;
			}

			DEBUG("Dijkstra: FOUND next: target router:%d, cur_link:%d, searched:%d\n", target_router, cur_link, topology[j].linkcost[target_link_idx].link_id);

			alt = dist[cur_router] + topology[target_router].linkcost[target_link_idx].cost;
			if (alt < dist[target_router]) {
				dist[target_router] = alt;
				prev[target_router] = cur_router;
			}
		}
	}

	cur_router = dest_rid;
	prev_router = dest_rid;
	printf("Path in reverse: %d", dest_rid);
	while (true) {
		cur_router = prev[cur_router];
		printf(", %d", cur_router);
		if (cur_router == 0) {
			break;
		}
		prev_router = cur_router;
	}
	puts("");
	DEBUG("Dijkstra: src:%d, dst:%d, next:%d, cost:%d\n", rid, dest_rid, prev_router, dist[prev_router]);
}

static int create_topology(uint32_t rid, int router_fd, struct sockaddr_in *nse_info, struct circuit_DB *my_db) {
	int length;
	char buf[BUF_SIZE] = {0};
	struct pkt_HELLO *hello;
	struct pkt_LSPDU *lspdu;

	/* element [0] is unused */
	struct circuit_DB topology[NBR_ROUTER + 1];
	memset(topology, 0, sizeof(topology));

	assert(my_db != NULL);
	topology[rid] = *my_db;

	assert(&topology[rid] != NULL);

	while (true) {
		fflush(stdout);
		length = recvfrom(router_fd, &buf, sizeof(buf), 0, NULL, NULL);
		if (length == sizeof(struct pkt_HELLO)) {

			hello = (struct pkt_HELLO *) buf;
			assert(hello->rid > 0 && hello->rid <= NBR_ROUTER);

			DEBUG("R%d say HELLO to R%d via link %d\n", hello->rid, rid, hello->link_id);
			flood_lspdu(router_fd, nse_info, rid, hello->link_id, topology);

		} else if (length == sizeof(struct pkt_LSPDU)) {
			lspdu = (struct pkt_LSPDU *) buf;

			if (rid == lspdu->rid) {
				continue;
			}
			if (update_topology(topology, lspdu)) {
				for (int i = 0; i < topology[rid].nbr_link; i++) {
					flood_lspdu(router_fd, nse_info, rid, topology[rid].linkcost[i].link_id, topology);
				}
			}

			for (int i = 1; i <= NBR_ROUTER; i++) {
				dijkstra(topology, rid, i);
			}

			DEBUG("R%d receives an LSPDU: sender R%d, router_id %d, link_id %d, cost %d, via %d\n", rid, lspdu->sender, lspdu->rid, lspdu->link_id, lspdu->cost, lspdu->via);
		}
		fflush(stdout);
	}

	/* Must not return */
	assert(false);
	return -1;
}

int make_routing_table(uint32_t rid, int router_fd, struct sockaddr_in *nse_info) {
	int i;
	size_t length;
	struct circuit_DB my_db;

	assert(nse_info != NULL);

	if (send_init(rid, router_fd, nse_info) < 0) {
		return -1;
	}

	DEBUG("%s\n", "Receiving circuit DB from NSE");
	length = recvfrom(router_fd, &my_db, sizeof(my_db), 0, NULL, NULL);
	if (length == -1 || length != sizeof(my_db)) {
		return -1;
	}

	/* Seems like NSE simulator didn't convert the integers to netlong's.... */
	DEBUG("nbr_link:%d\n", my_db.nbr_link);
	for (i = 0; i < my_db.nbr_link; i++) {
		DEBUG("\tlink_id:%d, cost:%d\n", my_db.linkcost[i].link_id, my_db.linkcost[i].cost);
	}

	DEBUG("%s\n", "Saying HELLO to neighbours");
	for (i = 0; i < my_db.nbr_link; i++) {
		send_hello(rid, router_fd, my_db.linkcost[i].link_id, nse_info);
	}

	create_topology(rid, router_fd, nse_info, &my_db);

	assert(false);
	/* Should not return */
	return -1;
}
