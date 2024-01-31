#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "lib/utils.h"
#include "lib/http.h"
#include "lib/udp.h"
#include "lib/socket.h"
#include "lib/filesystem/operations.h"
#include "webserver.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    unsigned int port_str_len = strlen(port_str)+1; // +1 for \0

    ws->HOST = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    ws->PORT = calloc(port_str_len, sizeof(char));
    ws->open_sockets = calloc(MAX_NUM_OPEN_SOCKETS, sizeof(struct pollfd));
    ws->open_sockets_config = calloc(MAX_NUM_OPEN_SOCKETS, sizeof(open_socket));
    ws->num_open_sockets = 0;

    for (int i = 0; i < MAX_NUM_OPEN_SOCKETS; i++) {
        ws->open_sockets[i].fd = -1;
        ws->open_sockets_config[i].is_server_socket = 0;
    }

    if (strlen(hostname)+1 > HOSTNAME_MAX_LENGTH) {
        perror("Invalid hostname");
        return NULL;
    }
    memcpy(ws->HOST, hostname, HOSTNAME_MAX_LENGTH * sizeof(char));

    if (str_is_uint16(port_str) < 0) {
        perror("Invalid port.");
        return NULL;
    }
    memcpy(ws->PORT, port_str, port_str_len * sizeof(char));

    ws->node = NULL;

    return ws;
}

/**
 * TODO: Doc this
 * @return 0 when the connection is still alive, -1 when the socket's events have to be reevaluated
 */
int handle_connection(short events, int *in_fd, enum connection_protocol protocol, webserver *ws, file_system *fs) {
    if (protocol == TCP) {
        if (http_handle(in_fd, ws, fs) < 0) return -1;
    } else if (protocol == UDP) {
        udp_handle(events, in_fd, ws);
        return -1;
    }

    return 0;
}

int webserver_tick(webserver *ws, file_system *fs) {
    int ready = poll(ws->open_sockets, ws->num_open_sockets, 100);
    if (ready == 0) return 0;
    else if (ready == -1) {
        perror("poll");
        return -1;
    }

    // Deciding what to do for each open socket - are they listening or not?
    for (int i = 0; i < ws->num_open_sockets; i++) {
        struct pollfd *sock = &(ws->open_sockets[i]);
        open_socket *sock_config = &(ws->open_sockets_config[i]);

        if (!(sock->revents & (POLLIN | POLLOUT))) continue;

        // Handle TCP server-sockets
        if (sock_config->is_server_socket == 1 && sock_config->protocol == TCP) {
            int in_fd = socket_accept(&(sock->fd));
            if (in_fd < 0) {
                perror("Socket failed to accept.");
                continue;
            }

            // appending the client socket
            for (int j = 0; j < MAX_NUM_OPEN_SOCKETS; j++) {
                if (ws->open_sockets[j].fd == -1) {
                    sock->events = 0;
                    ws->open_sockets[j].fd = in_fd;
                    ws->open_sockets[j].events = POLLIN;
                    ws->open_sockets_config[j].protocol = sock_config->protocol;
                    ws->num_open_sockets++;
                    break;
                }
            }

            continue;
        }

        // Handle UDP server socket & all client sockets
        if (handle_connection(sock->revents, &(sock->fd), sock_config->protocol, ws, fs) < 0) {
            // Disableing all events
            sock->events = 0;

            if (sock_config->protocol == TCP) {
                // Disabling the TCP client socket
                sock->fd = -1;
                sock_config->is_server_socket = 0;
                sock_config->protocol = 0;

                // re-enableing the TCP server socket
                // TODO: Check if this is necessary
            }
        }
    }

    return 0;
}

void webserver_free(webserver *ws) {
    free(ws->HOST);
    free(ws->PORT);
    free(ws->open_sockets);
    free(ws->open_sockets_config);

    if (ws->node != NULL) dht_node_free(ws->node);

    free(ws);
}

int main(int argc, char **argv) {
    if (argc != MIN_NUMBER_OF_PARAMS + 1 && argc != MAX_NUMBER_OF_PARAMS + 1) {
        perror("Wrong number of args. Usage: ./webserver {IP} {PORT} {Node ID} {? Anchor IP} {? Anchor PORT} ; When Anchor IP is set, Anchor PORT must be set as well.");
        exit(EXIT_FAILURE);
    }

    // initializing underlying filesystem
    file_system *fs = fs_create(50);
    fs_mkdir(fs, "/static");
    fs_mkdir(fs, "/dynamic");
    fs_mkfile(fs, "/static/foo");
    fs_writef(fs, "/static/foo", "Foo");
    fs_mkfile(fs, "/static/bar");
    fs_writef(fs, "/static/bar", "Bar");
    fs_mkfile(fs, "/static/baz");
    fs_writef(fs, "/static/baz", "Baz");

    // initializing webserver
    webserver *ws = webserver_init(argv[1], argv[2]);
    if (!ws) {
        perror("Initialization of the webserver failed.");
        exit(EXIT_FAILURE);
    }

    if (argc > 4) {
        ws->node = dht_node_init(argv[3], argv[4], argv[5]);
    } else ws->node = dht_node_init(argv[3], NULL, NULL);

    // opening UDP Socket
    if (socket_open(ws, SOCK_DGRAM) < 0) {
        perror("UDP Socket Creation failed.");
        exit(EXIT_FAILURE);
    }

    // opening TCP Socket
    if (socket_open(ws, SOCK_STREAM) < 0) {
        perror("TCP Socket Creation failed.");
        exit(EXIT_FAILURE);
    }

    int should_stabilize = 0;
    if (getenv("NO_STABILIZE") == NULL) should_stabilize = 1;

    int t = 0;
    int quit = 0;
    while(!quit) {
        t++;

        for (int i = 0; i < ws->num_open_sockets; i++) {
            struct pollfd *sock = &(ws->open_sockets[i]);
            open_socket *sock_config = &(ws->open_sockets_config[i]);

            if (sock_config->is_server_socket == 1) {
                if ((ws->node->status == JOINING | ws->node->status == STABILIZING) && sock_config->protocol == UDP) {
                    sock->events = POLLOUT;
                    break;
                } else if (ws->node->status == OK) {
                    sock->events = POLLIN | POLLOUT;
                }
            }
        }

        if (webserver_tick(ws, fs) != 0) quit = 1;

        if (should_stabilize == 1 && t % STABILIZE_INTERVAL == 0 && ws->node != NULL) {
            ws->node->status = STABILIZING;
            t = 0;
        }
    }

    for (int i = 0; i < ws->num_open_sockets; i++) {
        int *sockfd = &(ws->open_sockets[i]).fd;
        socket_shutdown(NULL, sockfd);
    }

    webserver_free(ws);
    fs_free(fs);

    return 0;
}
