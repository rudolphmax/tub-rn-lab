#include "udp.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

udp_packet* udp_packet_create(unsigned short type, uint16_t hash, uint16_t node_id, char *node_ip, char *node_port) {
    udp_packet *pkt = calloc(1, sizeof(udp_packet));

    pkt->bytesize = 0;
    pkt->node_port = 0;
    pkt->node_ip = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));

    if (node_ip != NULL) {
        strncpy(pkt->node_ip, node_ip, MIN(HOSTNAME_MAX_LENGTH-1, strlen(node_ip)));
    }
    if (node_port != 0) {
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

/**
 * Validates the a UDP packet in string-form and fills a udp_packet object.
 * @param pkt_string request in string form as it came from the stream
 * @param pkt request object to be filled
 * @return 0 on success, -1 on error.
 */
int udp_parse_packet(char *pkt_string, udp_packet *pkt) {
    uint16_t t = 0;
    uint16_t h = 0;
    uint16_t id = 0;
    uint32_t ip = 0;
    uint16_t p = 0;

    memcpy(&t, pkt_string, 1);
    memcpy(&h, pkt_string+1, 2);
    memcpy(&id, pkt_string+3, 2);
    memcpy(&ip, pkt_string+5, 4);
    memcpy(&p, pkt_string+9, 2);

    pkt->type = ntohs(t);
    pkt->hash = ntohs(h);
    pkt->node_id = ntohs(id);
    pkt->node_port = ntohs(p);

    struct in_addr ip_addr = {ip};
    strcpy(pkt->node_ip, inet_ntoa(ip_addr));

    return 0;
}

int udp_process_packet(webserver *ws, udp_packet  *pkt_out, udp_packet *pkt_in) {
    return -1;
}

// TODO: Poor naming as UDP is not connection-based
int udp_handle_connection(int *in_fd, webserver *ws, file_system *fs) {
    char *buf = calloc(MAX_DATA_SIZE, sizeof(char)); // TODO: Update MAX_DATA_SIZE for UDP (pkt-size is fixed after all)

    // TODO: refactor this into combined function in socket (ideally)
    int n_bytes = 0;
    while (n_bytes < 11 && n_bytes >= 0) { // TODO: Hardcoded UDP-Packet length
        n_bytes = recvfrom(
                *in_fd,
                // buffer[0 - bytes_received-1] is full of data,
                // buffer[bytes_received] is where we want to continue to write (the next first byte)
                buf + n_bytes,
                // the size of the space from buffer[bytes_received] to buffer[bufsize-1]
                (MAX_DATA_SIZE-1) - n_bytes,
                0,
                NULL,
                NULL
        );
    }

    if (n_bytes <= 0) {
        if (errno == ECONNRESET || errno == EINTR || errno == ETIMEDOUT) {
            if (socket_shutdown(ws, in_fd) != 0) perror("Socket shutdown failed.");
            free(buf);
        }

        perror("Socket couldn't read package.");
        return -1;
    }

    udp_packet *pkt_in;
    pkt_in = udp_packet_create(0, 0, 0, NULL, NULL);
    if (pkt_in == NULL) perror("Error initializing packet structure.");

    udp_packet *pkt_out = udp_packet_create(0, 0, 0, NULL, NULL);

    if (udp_parse_packet(buf, pkt_in) != 0) pkt_in = NULL;

    if (udp_process_packet(ws, pkt_out, pkt_in) == 0) {
        char *res_msg = udp_packet_stringify(pkt_out);
        socket_send(ws, in_fd, res_msg, strlen(res_msg), NULL, 0);
        free(res_msg);

    } else perror("Error processing packet.");

    udp_packet_free(pkt_in);
    udp_packet_free(pkt_out);

    free(buf);
    return 0;
}
