#include "dht.h"
#include "utils.h"
#include "../webserver.h"

dht_neighbor* dht_neighbor_init(char *neighbor_id, char* neighbor_ip, char* neighbor_port) {
    if (neighbor_port == NULL || neighbor_ip == NULL) {
        perror("Invalid DHT IP or Port.");
        return NULL;
    }

    if (str_is_uint16(neighbor_id) < 0) {
        perror("Invalid DHT Node ID.");
        return NULL;
    }

    dht_neighbor *neighbor = calloc(1, sizeof(dht_neighbor));
    neighbor->ID = strtol(neighbor_id, NULL, 10);
    neighbor->PORT = neighbor_port;
    neighbor->IP = neighbor_ip;

    return neighbor;
}

dht_node* dht_node_init(char *dht_node_id) {
    if (str_is_uint16(dht_node_id) < 0) {
        perror("Invalid DHT Node ID.");
        return NULL;
    }

    dht_node *node = calloc(1, sizeof(dht_node));
    node->ID = strtol(dht_node_id, NULL, 10);

    node->pred = dht_neighbor_init(
        getenv("PRED_ID"),
        getenv("PRED_IP"),
        getenv("PRED_PORT")
        );
    node->succ = dht_neighbor_init(
            getenv("SUCC_ID"),
            getenv("SUCC_IP"),
            getenv("SUCC_PORT")
        );

    if (node->pred == NULL || node->succ == NULL) {
        free(node);
        return NULL;
    }

    return node;
}

void webserver_dht_node_free(dht_node *node) {
    free(node->pred->IP); // TODO: These might be invalid as the pointers come directly from the env vars -> we don't even allocate them...
    free(node->pred->PORT);
    free(node->pred);
    free(node->succ->IP);
    free(node->succ->PORT);
    free(node->succ);
    free(node);
}

unsigned short dht_node_is_responsible(dht_node *node, uint16_t hash) {
    if (node->pred->ID > node->ID) {
        if (node->ID >= hash || node->pred->ID < hash) return 1;
    } else if (node->ID >= hash && node->pred->ID < hash) return 1;

    if (node->ID > node->succ->ID) {
        if (hash <= node->succ->ID || hash > node->ID) return 2;
    } else if (node->succ->ID >= hash && node->ID < hash) return 2;

    return 0;
}
