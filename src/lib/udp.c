#include "udp.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>

udp_packet* udp_packet_create(unsigned short type, uint16_t hash, uint16_t node_id, char *node_ip, char *node_port) {
    udp_packet *pkt = calloc(1, sizeof(udp_packet));

    pkt->bytesize = 0;
    pkt->node_port = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    pkt->node_ip = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));

    if (node_ip != NULL) {
        strncpy(pkt->node_ip, node_ip, MIN(HOSTNAME_MAX_LENGTH-1, strlen(node_ip)));
    }
    if (node_port != NULL) {
        pkt->node_port = strtol(node_port, NULL, 10);
    }

    pkt->type = type;
    pkt->hash = hash;
    pkt->node_id = node_id;

    return pkt;
}

char* udp_packet_stringify(udp_packet *pkt) {
    uint16_t t = htons(pkt->type);
    uint16_t h = htons(pkt->hash);
    uint16_t id = htons(pkt->node_id);
    uint32_t ip = inet_addr(pkt->node_ip);
    uint16_t p = htons(pkt->node_port);

    pkt->bytesize = 11;
    char *msg = calloc(pkt->bytesize, sizeof(char));

    memcpy(msg, &t, 1);
    memcpy(msg + 1, &h, 2);
    memcpy(msg + 3, &id, 2);
    memcpy(msg + 5, &ip, 4);
    memcpy(msg + 9, &p, 2);

    return msg;
}

void udp_packet_free(udp_packet *pkt) {
    free(pkt->node_ip);
    free(pkt);
}

int udp_send_to_node(webserver *ws, int *sockfd, udp_packet *packet, dht_neighbor *dest_node) {
    char *msg = udp_packet_stringify(packet);
    if (socket_send(ws, sockfd, msg, packet->bytesize, dest_node->IP, dest_node->PORT) < 0) return -1;

    return 0;
}
