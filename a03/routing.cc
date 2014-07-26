#include "routing.h"
#include "common.h"
#include <netinet/in.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <unistd.h>

#include <vector>
#include <map>
#include <algorithm>

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

/* Real implementation */

static int send_init(uint32_t rid, int router_fd, struct sockaddr_in *nse_info, FILE *logfd) {
	struct pkt_INIT buf = {rid};
	assert(nse_info != NULL);
	fprintf(logfd, "R%d sends INIT\n", rid);
	return sendto(router_fd, &buf, sizeof(buf), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
}

static int send_hello(uint32_t rid, int router_fd, uint32_t link_id, struct sockaddr_in *nse_info, FILE *logfd) {
	struct pkt_HELLO buf = {rid, link_id};
	assert(nse_info != NULL);
	fprintf(logfd, "R%d sends HELLO via link id %d\n", rid, link_id);
	return sendto(router_fd, &buf, sizeof(buf), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
}

/* reply by sending everything back */
static void flood_lspdu(int router_fd, struct sockaddr_in *nse_info, uint32_t rid, uint32_t target_link, struct circuit_DB topology[NBR_ROUTER], FILE *logfd){
	struct pkt_LSPDU lspdu;

	/* reply with LSPDU's */
	lspdu.sender = rid;
	for (uint32_t k = 0; k < topology[rid].nbr_link; k++) {
		lspdu.via = topology[rid].linkcost[k].link_id;

		/* only reply to the router that shares the same link as from the hello pkt */
		if (target_link != lspdu.via) {
			continue;
		}

		for (uint32_t i = 1; i <= NBR_ROUTER; i++) {
			lspdu.rid = i;
			for (uint32_t j = 0; j < topology[i].nbr_link; j++) {
				lspdu.link_id = topology[i].linkcost[j].link_id;
				lspdu.cost = topology[i].linkcost[j].cost;
				fprintf(logfd, "R%d sends a LSPDU: sender R%d, router_id %d, link_id %d, cost %d, via %d\n", rid, lspdu.sender, lspdu.rid, lspdu.link_id, lspdu.cost, lspdu.via);
				sendto(router_fd, &lspdu, sizeof(lspdu), 0, (struct sockaddr*) nse_info, sizeof(struct sockaddr_in));
			}
		}
	}
}

/* detect change and update the DB if necessary */
static bool update_topology(struct circuit_DB topology[NBR_ROUTER], struct pkt_LSPDU *lspdu) {
	struct circuit_DB *cur_node = &topology[lspdu->rid];

	assert(lspdu != NULL);
	assert(lspdu->link_id != 0);

	/* if there is a record in the existing topology, ignore the lspdu  */
	for (uint32_t i = 0; i < cur_node->nbr_link; i++) {
		if (cur_node->linkcost[i].link_id == lspdu->link_id) {
			assert(cur_node->linkcost[i].cost == lspdu->cost);
			return false;
		}
	}

	assert(cur_node->nbr_link <= NBR_ROUTER);

	cur_node->linkcost[cur_node->nbr_link].link_id = lspdu->link_id;
	cur_node->linkcost[cur_node->nbr_link].cost = lspdu->cost;
	cur_node->nbr_link++;
	return true;
}

/* create an adjacency matrix from the topology db -- basically connecting the edges together */
static void create_adj_matrix(struct circuit_DB topology[NBR_ROUTER], bool adj_mat[NBR_ROUTER+1][NBR_ROUTER+1], uint32_t cost_mat[NBR_ROUTER+1][NBR_ROUTER+1]) {
	int link[L2R_SIZE] = {0};
	int temp_link, target_router;

	for (uint32_t i = 0; i <= NBR_ROUTER; i++) {
		for (uint32_t j = 0; j <= NBR_ROUTER; j++) {
			adj_mat[i][j] = 0;
			cost_mat[i][j] = 0;
		}
	}
	
	for (uint32_t i = 1; i <= NBR_ROUTER; i++) {
		for (uint32_t j = 0; j < topology[i].nbr_link; j++) {
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

#ifndef NDEBUG
	puts("ADJ LIST");
	for (uint32_t i = 1; i <= NBR_ROUTER; i++) {
		printf("R%d -> ", i);
		for (uint32_t j = 0; j <= NBR_ROUTER; j++) {
			if (adj_mat[i][j]) {
				printf("(%d,%d), ", j, cost_mat[i][j]);
			}
		}
		puts("");
	}
#endif
}

static void dijkstra(struct circuit_DB topology[NBR_ROUTER], uint32_t rid, FILE *logfd) {

	uint32_t dist[NBR_ROUTER + 1] = {0};
	uint32_t prev[NBR_ROUTER + 1] = {0};

	bool adj_mat[NBR_ROUTER + 1][NBR_ROUTER + 1];
	uint32_t cost_mat[NBR_ROUTER + 1][NBR_ROUTER + 1];

	/* convert topology db to adjacency matrix for convenience */
	create_adj_matrix(topology, adj_mat, cost_mat);

	/* initialize book-keeping arrays */
	for (uint32_t i = 1; i <= NBR_ROUTER; i++) {
		if (adj_mat[rid][i]) {
			dist[i] = cost_mat[rid][i];
			prev[i] = rid;
		} else {
			dist[i] = IFNTY;
		}
	}

	dist[rid] = 0;

	int num_valid = NBR_ROUTER;
	bool popped[NBR_ROUTER+1] = {false};

	/* begin the search */
	while (num_valid > 0) {
		uint32_t min_dist = IFNTY;
		uint32_t next_router = 0;

		/* find the next router that has the minimum distance */
		for (int i = 1; i <= NBR_ROUTER; i++) {
			if (!popped[i] && dist[i] <= min_dist) {
				next_router = i;
				min_dist = dist[i];
			}
		}
		num_valid--;

		if (next_router == 0) {
			break;
		}

		popped[next_router] = true;

		/* find the sub-path */
		for (int i = 1; i <= NBR_ROUTER; i++) {
			if (adj_mat[next_router][i] && !popped[i]) {
				uint32_t alt = dist[next_router] + cost_mat[next_router][i];
				if (alt < dist[i]) {
					dist[i] = alt;
					prev[i] = next_router;
				}
			}
		}
	}

	/* print the RIB */
	fprintf(logfd, "# RIB\n");
	for (uint32_t i = 1; i <= NBR_ROUTER; i++) {
		if (i == rid) {
			fprintf(logfd, "R%d -> R%d -> (Local), 0\n", rid, i);
		} else {

			if (dist[i] == IFNTY) {
				continue;
			}

			uint32_t prev_router = i, prevprev_router = i;
			while (true) {
				prev_router = prev[prev_router];
				if (prev_router == rid || prev_router == 0) {
					break;
				}
				prevprev_router = prev_router;
				printf("prev: %d\n", prevprev_router);
			}
			//printf("src:%d, dst:%d, path:%d, cost:%d\n", rid, i , prevprev_router, dist[i]);
			fprintf(logfd, "R%d -> R%d -> R%d, %d\n", rid, i, prevprev_router, dist[i]);
		}
	}
	fprintf(logfd, "\n");
}

static void log_topology_db(uint32_t rid, struct circuit_DB topology[NBR_ROUTER +1], FILE *logfd) {
	fprintf(logfd, "\n# Topology Database\n");
	for (uint32_t i = 1; i <= NBR_ROUTER; i++) {
		if (topology[i].nbr_link <= 0) {
			continue;
		}
		fprintf(logfd, "R%d -> R%d nbr link %d\n", rid, i, topology[i].nbr_link);
		for (uint32_t j = 0; j < topology[i].nbr_link; j++) {
			fprintf(logfd, "R%d -> R%d link %d cost %d\n", rid, i, topology[i].linkcost[j].link_id, topology[i].linkcost[j].cost);
		}
	}
	fprintf(logfd, "\n");
}

static int create_topology(uint32_t rid, int router_fd, struct sockaddr_in *nse_info, struct circuit_DB *my_db, FILE *logfd) {
	int length;
	char buf[BUF_SIZE] = {0};
	struct pkt_HELLO *hello;
	struct pkt_LSPDU *lspdu;

	/* element [0] is unused */
	struct circuit_DB topology[NBR_ROUTER + 1];
	memset(topology, 0, sizeof(topology));

	assert(my_db != NULL);
	topology[rid] = *my_db;

	/* log the initial db that is retrieved from NSE */
	log_topology_db(rid, topology, logfd);

	assert(&topology[rid] != NULL);

	/* Either receive HELLO packets at first, or receive LSPDU in the usual case */
	while (true) {
		length = recvfrom(router_fd, &buf, sizeof(buf), 0, NULL, NULL);
		if (length == sizeof(struct pkt_HELLO)) {

			hello = (struct pkt_HELLO *) buf;
			assert(hello->rid > 0 && hello->rid <= NBR_ROUTER);

			flood_lspdu(router_fd, nse_info, rid, hello->link_id, topology, logfd);

		} else if (length == sizeof(struct pkt_LSPDU)) {
			lspdu = (struct pkt_LSPDU *) buf;

			if (rid == lspdu->rid) {
				continue;
			}
			fprintf(logfd, "R%d receives an LSPDU: sender R%d, router_id %d, link_id %d, cost %d, via %d\n", rid, lspdu->sender, lspdu->rid, lspdu->link_id, lspdu->cost, lspdu->via);

			if (!update_topology(topology, lspdu)) {
				/* no change */
				continue;
			}

			log_topology_db(rid, topology, logfd);

			dijkstra(topology, rid, logfd);

			/* then flood the neighbours with the latest changes */
			for (uint32_t i = 0; i < topology[rid].nbr_link; i++) {
				flood_lspdu(router_fd, nse_info, rid, topology[rid].linkcost[i].link_id, topology, logfd);
			}
		}
	}

	/* Must not return */
	assert(false);
	return -1;
}

int make_routing_table(uint32_t rid, int router_fd, struct sockaddr_in *nse_info) {
	int length;
	struct circuit_DB my_db;
	FILE *logfd;
	char buf[256] = {0};

	assert(nse_info != NULL);

	snprintf(buf, sizeof(buf), "router%u.log", rid);
	logfd = fopen(buf, "w");
	setbuf(logfd, NULL);

	if (send_init(rid, router_fd, nse_info, logfd) < 0) {
		return -1;
	}

	length = recvfrom(router_fd, &my_db, sizeof(my_db), 0, NULL, NULL);
	if (length == -1 || length != sizeof(my_db)) {
		return -1;
	}

	for (uint32_t i = 0; i < my_db.nbr_link; i++) {
		send_hello(rid, router_fd, my_db.linkcost[i].link_id, nse_info, logfd);
	}

	create_topology(rid, router_fd, nse_info, &my_db, logfd);

	assert(false);
	/* Should not return */
	return -1;
}
