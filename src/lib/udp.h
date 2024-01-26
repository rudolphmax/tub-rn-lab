#ifndef RN_PRAXIS_UDP_H
#define RN_PRAXIS_UDP_H

#include <stdlib.h>
#include "../webserver.h"
#include "socket.h"

#define UDP_DATA_SIZE 11

typedef enum udp_packet_type {
    LOOKUP,
    REPLY,
    STABILIZE,
    NOTIFY,
    JOIN
} udp_packet_type;

typedef struct udp_packet {
    udp_packet_type type;
    uint16_t hash;
    uint16_t node_id;
    char* node_ip;
    uint16_t node_port;
    unsigned int bytesize;
} udp_packet;

/**
 * Creates a new UDP packet and fills it with the given values (may be NULL / 0 etc.)
 * @param type The packet's type.
 * @param hash The communicated hash. The first 16 Bits of a SHA256 hash.
 * @param node_id The ID of the communicated node (the first 16 Bits of a SHA256 hash).
 * @param node_ip The IP of the communicated node.
 * @param node_port The Port of the communicated node.
 * @return a UDP packet object, NULL on error.
 */
udp_packet *udp_packet_create(udp_packet_type type, uint16_t hash, uint16_t node_id, char *node_ip, char *node_port);

/**
 * Frees the given UDP packet.
 * @param pkt the packet to be freed.
 */
void udp_packet_free(udp_packet *pkt);

/**
 * Sends a given UDP packet to a specific node (/client defined by IP and Port).
 * @param ws This webserver object.
 * @param sockfd The socket to send to (bound DGRAM socket).
 * @param packet The UDP packet to send.
 * @param dest_node The node to send the packet to.
 * @return 0 on success, -1 on error.
 */
int udp_send_to_node(webserver *ws, int *sockfd, udp_packet *packet, dht_neighbor *dest_node);

/**
 * Handles an incoming UDP connection.
 * @param in_fd Socket File Descriptor of the accepted connection.
 * @param ws Webserver object.
 * @return 0 on success, -1 on error.
 */
int udp_handle(int *in_fd, webserver *ws);

#endif //RN_PRAXIS_UDP_H
