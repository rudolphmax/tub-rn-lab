#ifndef RN_PRAXIS_WEBSERVER_H
#define RN_PRAXIS_WEBSERVER_H

#include "lib/filesystem/filesystem.h"

#define HOSTNAME_MAX_LENGTH 16 // Max. hostname length INCLUDING \0
#define EXPECTED_NUMBER_OF_PARAMS 2
#define MAX_NUM_OPEN_SOCKETS 10
#define MAX_DATA_SIZE 1024
#define RECEIVE_ATTEMPTS 1 // The amount of times the server should retry receiving from a socket if an error occurs

typedef struct dht_neighbor {
    uint16_t ID;
    char* IP;
    char* PORT;
} dht_neighbor;

/**
 * This structure hold all the info the webserver needs
 * to function as a node in a DHT.
 */
typedef struct dht_node {
    uint16_t ID;
    dht_neighbor* pred;
    dht_neighbor* succ;
} dht_node;

typedef struct webserver {
    char* HOST;
    char* PORT;
    // Array of file descriptors (int) of currently open sockets. Length: MAX_NUM_OPEN_SOCKETS
    int* open_sockets;
    int num_open_sockets;
    dht_node *node;
} webserver;

enum connection_protocol {
    HTTP,
    UDP
};

/**
 * Initializes a new webserver-object from a given hostname and port.
 * @param hostname the server hostname (IPv4)
 * @param port_str the servers port (per TCP/IP spec, a valid uint16) in string-form
 * @return A webserver object on success, NULL on error.
 */
webserver* webserver_init(char* hostname, char* port_str);

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
int webserver_dht_node_init(webserver *ws, char *dht_node_id);

/**
 * Executes one lifetime-tick of the given webserver
 * @param ws the webserver to tickle.
 * @return 0 on success, -1 on error
 */
int webserver_tick(webserver *ws, file_system *fs);

/**
 * Frees the given webserver-object
 * @param ws The webserver to be freed.
 */
void webserver_free(webserver *ws);

/**
 * Frees the given dht_node-object.
 * @param node The dht_node to be freed.
 */
void webserver_dht_node_free(dht_node *node);

#endif //RN_PRAXIS_WEBSERVER_H
