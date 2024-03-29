#include "udp.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

udp_packet* udp_packet_create(udp_packet_type type, uint16_t hash, uint16_t node_id, char *node_ip, char *node_port) {
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
 */
dht_neighbor* dht_neighbor_from_packet(udp_packet *pkt) {
    dht_neighbor *n = calloc(1, sizeof(dht_neighbor));
    n->PORT = calloc(7, sizeof(char));
    n->IP = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));

    strcpy(n->IP, pkt->node_ip);
    snprintf(n->PORT, 6, "%d", pkt->node_port);
    n->ID = pkt->node_id;

    return n;
}

/**
 * Serializes a UDP packet into a byte-array.
 * @param pkt
 * @return
 */
char* udp_packet_serialize(udp_packet *pkt) {
    //uint16_t t = htons(pkt->type);
    uint16_t h = htons(pkt->hash);
    uint16_t id = htons(pkt->node_id);
    uint32_t ip = inet_addr(pkt->node_ip);
    uint16_t p = htons(pkt->node_port);

    pkt->bytesize = UDP_DATA_SIZE;
    char *msg = calloc(pkt->bytesize, sizeof(char));

    // memcpy(msg, &(pkt->type), 1);
    msg[0] = pkt->type;
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

    if (pkt_in->type == LOOKUP || pkt_in->type == JOIN) {
        unsigned short responsibility;
        if (ws->node == NULL) responsibility = 1;
        else if (pkt_in->type == JOIN) responsibility = dht_node_is_responsible(ws->node, pkt_in->node_id);
        else responsibility = dht_node_is_responsible(ws->node, pkt_in->hash);

        if (responsibility == 0 || (pkt_in->type == JOIN && responsibility == 2)) { // -> forward message to successor
            strcpy(pkt_out->node_ip, pkt_in->node_ip);
            pkt_out->node_port = pkt_in->node_port;
            pkt_out->node_id = pkt_in->node_id;
            pkt_out->hash = pkt_in->hash;
            pkt_out->type = pkt_in->type;

            strcpy(pkt_in->node_ip, ws->node->succ->IP);
            pkt_in->node_port = strtol(ws->node->succ->PORT, NULL, 10);
            return 0;
        }

        if (pkt_in->type == JOIN) {
            pkt_out->type = NOTIFY;
            pkt_out->hash = 0;
        } else {
            pkt_out->type = REPLY;
            pkt_out->hash = ws->node->ID;
        }

        if (responsibility == 1) {
            pkt_out->node_id = ws->node->ID;
            strcpy(pkt_out->node_ip, ws->HOST);
            pkt_out->node_port = strtol(ws->PORT, NULL, 10);

            if (pkt_in->type == JOIN) {
                free(ws->node->pred);
                ws->node->pred = dht_neighbor_from_packet(pkt_in);

                if (ws->node->succ == NULL) {
                    ws->node->succ = dht_neighbor_from_packet(pkt_in);
                }
            }

        } else if (responsibility == 2) {
            pkt_out->node_id = ws->node->succ->ID;
            strcpy(pkt_out->node_ip, ws->node->succ->IP);
            pkt_out->node_port = strtol(ws->node->succ->PORT, NULL, 10);

        } else return -1;

        return 0;

    } else if (pkt_in->type == STABILIZE)  {
        if (ws->node->pred == NULL) {
            ws->node->pred = dht_neighbor_from_packet(pkt_in);
        }

        pkt_out->type = NOTIFY;
        pkt_out->hash = 0;
        pkt_out->node_id = ws->node->pred->ID;
        strcpy(pkt_out->node_ip, ws->node->pred->IP);
        pkt_out->node_port = strtol(ws->node->pred->PORT, NULL, 10);
        return 0;

    } else if (pkt_in->type == NOTIFY) {
        if (pkt_in->node_id != ws->node->ID || pkt_in->node_port != strtol(ws->PORT, NULL, 10) || strcmp(pkt_in->node_ip, ws->HOST) != 0) {
            free(ws->node->succ);
            ws->node->succ = dht_neighbor_from_packet(pkt_in);
        }

    } else if (pkt_in->type == REPLY) {
        pkt_out->node_id = pkt_in->node_id;
        strcpy(pkt_out->node_ip, pkt_in->node_ip);
        pkt_out->node_port = pkt_in->node_port;

        // writing responsible node to lookup-cache 
        int i = dht_lookup_cache_find_empty(ws->node);
        if (i != -1) {
            ws->node->lookup_cache->nodes[i] = dht_neighbor_from_packet(pkt_out);
        }
    }

    return 1; // don't answer received replies / notfies
}

int udp_handle(short events, int *in_fd, webserver *ws) {
    char *buf = calloc(UDP_DATA_SIZE+1, sizeof(char));

    udp_packet *pkt_in = udp_packet_create(0, 0, 0, NULL, NULL);
    if (pkt_in == NULL) perror("Error initializing packet structure.");

    udp_packet *pkt_out = udp_packet_create(0, 0, 0, NULL, NULL);
    if (pkt_out == NULL) perror("Error initializing packet structure.");

    if (ws->node->status == JOINING) { // This node wants to join an existing DHT
        pkt_out->type = JOIN;
        pkt_out->hash = 0;
        pkt_out->node_id = ws->node->ID;
        strcpy(pkt_out->node_ip, ws->HOST);
        pkt_out->node_port = strtol(ws->PORT, NULL, 10);
        
        strcpy(pkt_in->node_ip, ws->node->succ->IP);
        pkt_in->node_port = strtol(ws->node->succ->PORT, NULL, 10);

        ws->node->status = OK;

    } else if (ws->node->status == STABILIZING) { // This node has to stabilize
        if (ws->node->succ != NULL) {
            pkt_out->type = STABILIZE;
            pkt_out->hash = ws->node->ID;
            pkt_out->node_id = ws->node->ID;
            strcpy(pkt_out->node_ip, ws->HOST);
            pkt_out->node_port = strtol(ws->PORT, NULL, 10);
            
            strcpy(pkt_in->node_ip, ws->node->succ->IP);
            pkt_in->node_port = strtol(ws->node->succ->PORT, NULL, 10);
        }

        ws->node->status = OK;

    } else if (events & POLLIN) {
        // TODO: refactor this into combined function in socket (ideally)
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        memset(&addr, 0, addr_len);

        int n_bytes = 0;
        while (n_bytes < UDP_DATA_SIZE && n_bytes >= 0) {
            n_bytes = recvfrom(
                    *in_fd,
                    // buffer[0 - bytes_received-1] is full of data,
                    // buffer[bytes_received] is where we want to continue to write (the next first byte)
                    buf + n_bytes,
                    // the size of the space from buffer[bytes_received] to buffer[bufsize-1]
                    (UDP_DATA_SIZE) - n_bytes,
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

        udp_parse_packet(buf, pkt_in);
    
        if (udp_process_packet(ws, pkt_out, pkt_in) != 0) {
            udp_packet_free(pkt_in);
            udp_packet_free(pkt_out);
            free(buf);
            return -1;
        }
    }

    if (!(events & POLLOUT)) return 0;

    char *res_msg = udp_packet_serialize(pkt_out);

    char *port_str = calloc(7, sizeof(char));
    snprintf(port_str, 6, "%d", pkt_in->node_port);
    socket_send(ws, in_fd, res_msg, pkt_out->bytesize, pkt_in->node_ip, port_str);
    free(port_str);
    free(res_msg);

    udp_packet_free(pkt_in);
    udp_packet_free(pkt_out);

    free(buf);
    return 0;
}
