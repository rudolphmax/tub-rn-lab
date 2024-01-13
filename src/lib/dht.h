#ifndef RN_PRAXIS_DHT_H
#define RN_PRAXIS_DHT_H

#include <stdint.h>

#define LOOKUP_CACHE_SIZE 10

typedef struct dht_neighbor {
    uint16_t ID;
    char* IP;
    char* PORT;
} dht_neighbor;

typedef struct dht_lookup_cache {
    int hashes[LOOKUP_CACHE_SIZE];
    dht_neighbor* nodes[LOOKUP_CACHE_SIZE];
} dht_lookup_cache;

/**
 * This structure hold all the info the webserver needs
 * to function as a node in a DHT.
 */
typedef struct dht_node {
    uint16_t ID;
    dht_neighbor* pred;
    dht_neighbor* succ;
    dht_lookup_cache* lookup_cache;
} dht_node;

/**
 * Initializes a new dht_neighbor-object from a given ID, IP and PORT.
 * @param neighbor_id the ID of the neighbor
 * @param neighbor_ip the IP of the neighbor
 * @param neighbor_port the PORT of the neighbor
 * @return A dht_neighbor object on success, NULL on error.
 */
dht_neighbor* dht_neighbor_init(char *neighbor_id, char* neighbor_ip, char* neighbor_port);

/**
 * Initializes a new dht_node-object from a given ID.
 * @param ws the webserver to initialize the dht_node for
 * @param dht_node_id the ID of the webserver in the DHT (16Bit int in string-form).
 * @return 0 on success, -1 on error
 */
dht_node* dht_node_init(char *dht_node_id);

/**
 * Frees the given dht_node-object.
 * @param node The dht_node to be freed.
 */
void dht_node_free(dht_node *node);

/**
 * Decides whether a given DHT Node is responsible for
 * the given resource (in hash-form, i.e. the first 16bit of a SHA256 hash)
 * @param node the node to check responsibility for.
 * @param hash the hash to check against node.
 * @return 1 if node is responsible, 2 if node's successor is, 0 else
 */
unsigned short dht_node_is_responsible(dht_node *node, uint16_t hash);

/**
 * TODO: Doc this
 * @param node
 * @param hash
 * @param neighbor
 * @return
 */
short dht_lookup_cache_add_hash(dht_node *node, uint16_t hash);

/**
 * TODO: Doc this
 * @param node
 * @return
 */
int dht_lookup_cache_find_empty(dht_node *node);

/**
 * TODO: Doc this
 * @param node
 * @param hash
 * @return
 */
dht_neighbor* dht_lookup_cache_find_node(dht_node *node, uint16_t hash);

#endif //RN_PRAXIS_DHT_H
