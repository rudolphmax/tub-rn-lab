#include "dht.h"
#include "utils.h"
#include "../webserver.h"

dht_neighbor* dht_neighbor_init(char *neighbor_id, char* neighbor_ip, char* neighbor_port) {
    if (neighbor_port == NULL || neighbor_ip == NULL) {
        perror("Invalid DHT IP or Port.");
        return NULL;
    }

    if (neighbor_id != NULL && str_is_uint16(neighbor_id) < 0) {
        perror("Invalid DHT Node ID.");
        return NULL;
    }

    dht_neighbor *neighbor = calloc(1, sizeof(dht_neighbor));
    if (neighbor_id != NULL) neighbor->ID = strtol(neighbor_id, NULL, 10);
    neighbor->PORT = neighbor_port;
    neighbor->IP = neighbor_ip;

    return neighbor;
}

dht_node* dht_node_init(char *dht_node_id, char *dht_anchor_ip, char *dht_anchor_port) {
    if (str_is_uint16(dht_node_id) < 0) {
        perror("Invalid DHT Node ID.");
        return NULL;
    }

    dht_node *node = calloc(1, sizeof(dht_node));
    node->ID = strtol(dht_node_id, NULL, 10);
    node->status = OK;

    node->pred = NULL;
    node->succ = NULL;

    if (dht_anchor_ip != NULL && dht_anchor_port != NULL) {
        node->status = JOINING;
        node->succ = dht_neighbor_init(
            NULL,
            dht_anchor_ip,
            dht_anchor_port
        );
    } else {
        if (getenv("PRED_ID") != NULL && getenv("PRED_IP") != NULL && getenv("PRED_PORT") != NULL) {
            node->pred = dht_neighbor_init(
                getenv("PRED_ID"),
                getenv("PRED_IP"),
                getenv("PRED_PORT")
            );
        }

        if (getenv("SUCC_ID") != NULL && getenv("SUCC_IP") != NULL && getenv("SUCC_PORT") != NULL) {
            node->succ = dht_neighbor_init(
                getenv("SUCC_ID"),
                getenv("SUCC_IP"),
                getenv("SUCC_PORT")
            );
        }
    }

    node->lookup_cache = calloc(1, sizeof(dht_lookup_cache));
    for (int i = 0; i < LOOKUP_CACHE_SIZE; i++) node->lookup_cache->hashes[i] = -1;

    return node;
}

void dht_node_free(dht_node *node) {
    if (node->pred != NULL) free(node->pred);
    if (node->succ != NULL)free(node->succ);
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

short dht_lookup_cache_add_hash(dht_node *node, uint16_t hash) {
    for (int i = 0; i < LOOKUP_CACHE_SIZE; i++) {
        if (node->lookup_cache->nodes[i] != NULL) continue;

        node->lookup_cache->hashes[i] = hash;
        return 0;
    }

    // replacing the oldest cache entry
    node->lookup_cache->hashes[0] = hash;
    free(node->lookup_cache->nodes[0]->IP);
    free(node->lookup_cache->nodes[0]->PORT);
    free(node->lookup_cache->nodes[0]);

    return -1;
}

int dht_lookup_cache_find_empty(dht_node *node) {
    for (int i = 0; i < LOOKUP_CACHE_SIZE; i++) {
        if (node->lookup_cache->hashes[i] != -1 && node->lookup_cache->nodes[i] == NULL) return i;
    }

    return -1;
}

dht_neighbor* dht_lookup_cache_find_node(dht_node *node, uint16_t hash) {
    for (int i = LOOKUP_CACHE_SIZE-1; i >= 0; i--) {
        if (node->lookup_cache->hashes[i] == hash) return node->lookup_cache->nodes[i];
    }

    return NULL;
}
