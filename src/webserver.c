#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "lib/utils.h"
#include "lib/http.h"
#include "lib/socket.h"
#include "lib/filesystem/operations.h"
#include "webserver.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    unsigned int port_str_len = strlen(port_str)+1; // +1 for \0

    ws->HOST = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    ws->PORT = calloc(port_str_len, sizeof(char));
    ws->open_sockets = calloc(MAX_NUM_OPEN_SOCKETS, sizeof(int));
    ws->num_open_sockets = 0;

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

    return ws;
}

void handle_connection(int *in_fd, enum connection_protocol protocol, webserver *ws, file_system *fs) {
    int receive_attempts_left = RECEIVE_ATTEMPTS;
    int connection_is_alive = 1;

    while (connection_is_alive) {
        int retval = -1;

        if (protocol == HTTP) {
            retval = http_handle_connection(in_fd, ws, fs);
        } else if (protocol == UDP) {
            // retval = udp_handle_connection(in_fd, ws, fs);
            retval = -1;
        }

        if (retval < 0) receive_attempts_left--;
        if (receive_attempts_left == 0) connection_is_alive = 0;
    }
}

int webserver_tick(webserver *ws, file_system *fs) {
    // TODO: Multithread with pthread or use poll/select to accept multiple simultaneous connections

    // Deciding what to do for each open socket - are they listening or not?
    for (int i = 0; i < ws->num_open_sockets; i++) {
        int *sockfd = &(ws->open_sockets[i]);

        enum connection_protocol protocol = HTTP;

        int in_fd = socket_accept(sockfd);
        if (in_fd < 0) {
            if (errno != EINVAL && errno != EOPNOTSUPP) {
                perror("Socket failed to accept.");
                continue;
            }

            // Dealing with a non-connective protocol - assumed to be UDP here.
            in_fd = *sockfd;
            protocol = UDP;
        }

        handle_connection(&in_fd, protocol, ws, fs);
    }

    return 0;
}

void webserver_free(webserver *ws) {
    free(ws->HOST);
    free(ws->PORT);
    free(ws->open_sockets);
    free(ws);
}

int main(int argc, char **argv) {
    if (argc != EXPECTED_NUMBER_OF_PARAMS + 1) {
        perror("Wrong number of args. Usage: ./webserver {ip} {port}");
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

    // opening UDP Socket
    /*
    if (socket_open(ws, SOCK_DGRAM) < 0) {
        perror("UDP Socket Creation failed.");
        exit(EXIT_FAILURE);
    }
    */

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
        int *sockfd = &(ws->open_sockets[i]);
        socket_shutdown(NULL, sockfd);
    }

    webserver_free(ws);
    fs_free(fs);

    return 0;
}
