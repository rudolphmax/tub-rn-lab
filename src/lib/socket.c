#include <string.h>
#include <netdb.h>
#include "utils.h"
#include "socket.h"

int socket_accept(int *sockfd) {
    struct sockaddr_storage in_addr;
    socklen_t in_addr_size = sizeof(in_addr);
    debug_print("Accepting connection...");

    return accept(*sockfd, (struct sockaddr*) &in_addr, &in_addr_size);
}

int socket_open(webserver *ws, int socktype) {
    if (ws->num_open_sockets >= MAX_NUM_OPEN_SOCKETS) {
        perror("Maximum number of open open_sockets reached.");
        return -1;
    }

    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ws->HOST, ws->PORT, &hints, &res) != 0) return -1;

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) return -1;

    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) return -1;

    if (socktype == SOCK_STREAM) {
        listen(sockfd, BACKLOG_COUNT);
    }

    ws->open_sockets[ws->num_open_sockets] = sockfd;
    ws->num_open_sockets++;
    freeaddrinfo(res);
    return 0;
}

int socket_send(webserver *ws, int *sockfd, char *message) {
    unsigned long len = strlen(message);
    debug_printv("Sending message:", message);

    unsigned long bytes_sent = 0;
    while (bytes_sent < len) {
        int ret = sendto(*sockfd, message + bytes_sent, len - bytes_sent, 0, ws->HOST, strlen(ws->HOST));
        if (ret < 0) return -1;

        bytes_sent += ret;
    }

    return 0;
}

int socket_receive_all(int *in_fd, char *buf, size_t bufsize) {
    unsigned int bytes_received = 0;
    unsigned int content_length = 0;
    memset(buf, 0, bufsize);

    long body_size = -1;
    char *empty_line = NULL;

    // Receiving until buffer ends with CLRF (except last byte which is \0)
    while (empty_line == NULL || body_size < content_length) {
        if (bytes_received >= bufsize - 1) {
            perror("Buffer full before entire package read.");
            return -1;
        }

        debug_print("Receiving data...");
        // Receiving data from stream once
        int n_bytes = recvfrom(
                        *in_fd,
                        // buffer[0 - bytes_received-1] is full of data,
                        // buffer[bytes_received] is where we want to continue to write (the next first byte)
                        buf + bytes_received,
                        // the size of the space from buffer[bytes_received] to buffer[bufsize-1]
                        (bufsize-1) - bytes_received,
                        0,
                        NULL,
                        NULL
                    );
        // printf("Received %d bytes.\n", n_bytes);

        if (n_bytes == -1 || n_bytes == 0) return -1;

        bytes_received += n_bytes;

        // Catch existing Content-Length header and expect reading body
        char *content_length_header_line = strstr(buf, "\r\nContent-Length: ");
        if (content_length_header_line != NULL && content_length == 0) { // Request has content and content_length haven't been set yet
            char *content_length_str = content_length_header_line + 18;

            content_length = strtol(content_length_str, NULL, 10);
            debug_printv("Found Content-Length Header: %d", content_length_str);
            body_size = 0; // because of initial -2
        }

        empty_line = strstr(buf, "\r\n\r\n");
        if (empty_line != NULL) {
            if (content_length == 0) {
                break; // Found empy line and no content length header -> end of message
            } else {
                body_size += bytes_received - ((empty_line+4) - buf);
            }
        }
    }

    // Making sure buffer ends in \0 for safety
    buf[bufsize-1] = '\0';
    debug_printv("Full Message: \n------ \n", buf);
    debug_print("\n-----\n");

    return bytes_received;
}

int socket_shutdown(webserver *ws, int *sockfd) {
    if (shutdown(*sockfd, SHUT_RDWR) < 0) return -1;

    if (ws == NULL) return 0;

    // remove socket from open_socket list
    for (int i = 0; i < MAX_NUM_OPEN_SOCKETS; i++) {
        if (ws->open_sockets[i] == *sockfd) {
            ws->open_sockets[i] = 0;
            debug_print("Shutting down socket...");
        }
    }

    return 0;
}
