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

/**
 * TODO: Doc this
 * @param pkt
 * @return
 */
char* udp_packet_serialize(udp_packet *pkt) {
    //uint16_t t = htons(pkt->type);
    uint16_t h = htons(pkt->hash);
    uint16_t id = htons(pkt->node_id);
    uint32_t ip = inet_addr(pkt->node_ip);
    uint16_t p = htons(pkt->node_port);

    pkt->bytesize = 11;
    char *msg = calloc(pkt->bytesize, sizeof(char));

    memcpy(msg, &(pkt->type), 1);
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
    char *msg = udp_packet_serialize(packet);
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
    uint8_t t = 0;
    uint16_t h = 0;
    uint16_t id = 0;
    uint32_t ip = 0;
    uint16_t p = 0;

    memcpy(&t, pkt_string, 1);
    memcpy(&h, pkt_string+1, 2);
    memcpy(&id, pkt_string+3, 2);
    memcpy(&ip, pkt_string+5, 4);
    memcpy(&p, pkt_string+9, 2);

    pkt->type = t;
    pkt->hash = ntohs(h);
    pkt->node_id = ntohs(id);
    pkt->node_port = ntohs(p);

    struct in_addr ip_addr = {ip};
    strcpy(pkt->node_ip, inet_ntoa(ip_addr));

    return 0;
}

int udp_process_packet(webserver *ws, udp_packet  *pkt_out, udp_packet *pkt_in) {
    if (pkt_in == NULL) return -1;

    unsigned short responsibility;
    if (ws->node == NULL) responsibility = 1;
    else responsibility = dht_node_is_responsible(ws->node, pkt_in->hash);

    if (pkt_in->type != 1) {
        if (responsibility == 0) { // -> forward lookup to successor
            //memcpy(pkt_out, pkt_in, sizeof(*pkt_in));
            strcpy(pkt_out->node_ip, pkt_in->node_ip);
            pkt_out->node_port = pkt_in->node_port;
            pkt_out->node_id = pkt_in->node_id;
            pkt_out->hash = pkt_in->hash;
            pkt_out->type = pkt_in->type;

            strcpy(pkt_in->node_ip, ws->node->succ->IP);
            pkt_in->node_port = strtol(ws->node->succ->PORT, NULL, 10);
            return 0;
        }

        pkt_out->type = 1;
        pkt_out->hash = ws->node->ID;

        if (responsibility == 1) {
            pkt_out->node_id = ws->node->ID;
            strcpy(pkt_out->node_ip, ws->HOST);
            pkt_out->node_port = strtol(ws->PORT, NULL, 10);

        } else if (responsibility == 2) {
            pkt_out->node_id = ws->node->succ->ID;
            strcpy(pkt_out->node_ip, ws->node->succ->IP);
            pkt_out->node_port = strtol(ws->node->succ->PORT, NULL, 10);

        } else return -1;

        return 0;
    }

    pkt_out->node_id = pkt_in->node_id;
    strcpy(pkt_out->node_ip, pkt_in->node_ip);
    pkt_out->node_port = pkt_in->node_port;

    // writing responsible node to lookup-cache 
    int i = dht_lookup_cache_find_empty(ws->node);
    if (i != -1) {
        dht_neighbor *n = calloc(1, sizeof(dht_neighbor));
        n->PORT = calloc(7, sizeof(char));
        n->IP = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));

        strcpy(n->IP, pkt_out->node_ip);
        snprintf(n->PORT, 6, "%d", pkt_out->node_port);
        n->ID = pkt_out->node_id;

        ws->node->lookup_cache->nodes[i] = n;
    }

    return 1; // don't answer received replies
}

// TODO: Poor naming as UDP is not connection-based
int udp_handle_connection(int *in_fd, webserver *ws) {
    char *buf = calloc(MAX_DATA_SIZE, sizeof(char)); // TODO: Update MAX_DATA_SIZE for UDP (pkt-size is fixed after all)

    // TODO: refactor this into combined function in socket (ideally)
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    memset(&addr, 0, addr_len);

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
                (struct sockaddr *) &addr,
                &addr_len
        );
    }

    if (n_bytes <= 0) {
        if (errno == ECONNRESET || errno == EINTR || errno == ETIMEDOUT) {
            if (socket_shutdown(ws, in_fd) != 0) perror("Socket shutdown failed.");
            free(buf);
        }

        return -1;
    }

    udp_packet *pkt_in;
    pkt_in = udp_packet_create(0, 0, 0, NULL, NULL);
    if (pkt_in == NULL) perror("Error initializing packet structure.");

    udp_packet *pkt_out = udp_packet_create(0, 0, 0, NULL, NULL);

    if (udp_parse_packet(buf, pkt_in) != 0) pkt_in = NULL;

    if (udp_process_packet(ws, pkt_out, pkt_in) == 0) {
        char *res_msg = udp_packet_serialize(pkt_out);

        char *port_str = calloc(7, sizeof(char));
        snprintf(port_str, 6, "%d", pkt_in->node_port);
        socket_send(ws, in_fd, res_msg, pkt_out->bytesize, pkt_in->node_ip, port_str);
        free(res_msg);
    }

    udp_packet_free(pkt_in);
    udp_packet_free(pkt_out);

    free(buf);
    return 0;
}
