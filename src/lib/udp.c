#include "udp.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>

udp_packet* udp_packet_create(unsigned short type, uint16_t hash, uint16_t node_id, char *node_ip, char *node_port) {
    udp_packet *pkt = calloc(1, sizeof(udp_packet));

    pkt->node_port = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    pkt->node_ip = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));

    if (node_ip != NULL) {
        strncpy(pkt->node_ip, node_ip, MIN(HOSTNAME_MAX_LENGTH-1, strlen(node_ip)));
    }
    if (node_port != NULL) {
        strncpy(pkt->node_port, node_port, MIN(HOSTNAME_MAX_LENGTH-1, strlen(node_port)));
    }

    pkt->type = type;
    pkt->hash = hash;
    pkt->node_id = node_id;

    return pkt;
}

// TODO: This is not converting to byte-message correctly yet.
char* udp_packet_stringify(udp_packet *pkt) {
    uint32_t node_ip_nbo = inet_addr(pkt->node_ip);
    uint16_t *p = (uint16_t*) pkt->node_port;
    uint16_t node_port_nbo = htons(*p);

    unsigned int pkt_bytesize = 11;
    char *str = calloc(pkt_bytesize+1, sizeof(char));

    snprintf(str, pkt_bytesize, "%c%c%c%c%c", htons(pkt->type), htons(pkt->hash), htons(pkt->node_id), node_ip_nbo, node_port_nbo);

    str[pkt_bytesize] = '\0';
    return str;
}

void udp_packet_free(udp_packet *pkt) {
    free(pkt->node_ip);
    free(pkt->node_port);
    free(pkt);
}

int udp_send_to_node(webserver *ws, int *sockfd, udp_packet *packet, dht_neighbor *dest_node) {
    char *msg = udp_packet_stringify(packet);
    if (socket_send(ws, sockfd, msg, dest_node->IP, dest_node->PORT) < 0) return -1;

    return 0;
}
