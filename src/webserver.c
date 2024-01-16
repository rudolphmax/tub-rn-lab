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

void handle_connection(int *in_fd, enum connection_protocol protocol, webserver *ws, file_system *fs) {
    if (protocol == TCP) {
        http_handle_connection(in_fd, ws, fs);
    } else if (protocol == UDP) {
        udp_handle_connection(in_fd, ws);
    }
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

        if (!(sock->revents & POLLIN)) continue;

        // Handle TCP server-sockets
        if (sock_config->is_server_socket == 1 && sock_config->protocol == TCP) {
            int in_fd = socket_accept(&(sock->fd));
            if (in_fd < 0) {
                perror("Socket failed to accept.");
                continue;
            }

            // appending the client socket & disabling TCP server socket
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
        handle_connection(&(sock->fd), sock_config->protocol, ws, fs);
        if (sock_config->is_server_socket == 1) return 0;

        // Finding corresponding server-socket and re-enabling it
        for (int j = 0; j < ws->num_open_sockets; j++) {
            if (ws->open_sockets_config[j].is_server_socket != 1) continue;

            if (ws->open_sockets_config[j].protocol == sock_config->protocol) {
                ws->open_sockets[j].events = POLLIN;
                break;
            }
        }

        // Disabling the client socket
        // TODO: when to remove client sockets?
        /*sock->events = 0;
        sock->fd = -1;
        sock_config->is_server_socket = 0;
        sock_config->protocol = 0;*/
    }

    return 0;
}

void webserver_free(webserver *ws) {
    free(ws->HOST);
    free(ws->PORT);
    free(ws->open_sockets);

    if (ws->node != NULL) dht_node_free(ws->node);

    free(ws);
}

int main(int argc, char **argv) {
    if (argc != EXPECTED_NUMBER_OF_PARAMS + 1 && argc != EXPECTED_NUMBER_OF_PARAMS + 2) {
        perror("Wrong number of args. Usage: ./webserver {ip} {port} {optional: dht_node_id}");
        exit(EXIT_FAILURE);
    }

    // TODO: Refactor this into a function
    // initializing underlying filesystem
    file_system *fs = fs_create(500); // TODO: Which size to choose?
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

    if (argc == 4) { // expecting dht-node-id
        ws->node = dht_node_init(argv[3]);
    }

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

    int quit = 0;
    while(!quit) {
        if (webserver_tick(ws, fs) != 0) quit = 1;
    }

    for (int i = 0; i < ws->num_open_sockets; i++) {
        int *sockfd = &(ws->open_sockets[i]).fd;
        socket_shutdown(NULL, sockfd);
    }

    webserver_free(ws);
    fs_free(fs);

    return 0;
}
