#ifndef RN_PRAXIS_UDP_H
#define RN_PRAXIS_UDP_H

#include <stdlib.h>
#include "../webserver.h"
#include "socket.h"

typedef struct udp_packet {
    uint8_t type;
    uint16_t hash;
    uint16_t node_id;
    char* node_ip;
    uint16_t node_port;
    unsigned int bytesize;
} udp_packet;

/**
 * TODO: Doc this
 * @param type
 * @param hash
 * @param node_id
 * @param node
 * @return
 */
udp_packet *udp_packet_create(unsigned short type, uint16_t hash, uint16_t node_id, char *node_ip, char *node_port);

/**
 * TODO: Doc this
 * @param pkt
 */
void udp_packet_free(udp_packet *pkt);

/**
 * TODO: Doc this
 * @param type
 * @param hash
 * @param node_id
 * @param orig_node
 * @return
 */
int udp_send_to_node(webserver *ws, int *sockfd, udp_packet *packet, dht_neighbor *dest_node);

/**
 * Handles an incoming UDP connection.
 * @param in_fd Socket File Descriptor of the accepted connection.
 * @param ws Webserver object.
 * @return 0 on success, -1 on error.
 */
int udp_handle_connection(int *in_fd, webserver *ws);

#endif //RN_PRAXIS_UDP_H
