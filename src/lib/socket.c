#include "socket.h"

int socket_accept(int *sockfd) {
    struct sockaddr_storage in_addr;
    socklen_t in_addr_size = sizeof(in_addr);
    debug_print("Accepting connection...");

    return accept(*sockfd, (struct sockaddr*) &in_addr, &in_addr_size);
}

int socket_listen(webserver *ws) {
    if (ws->num_open_sockets >= MAX_NUM_OPEN_SOCKETS) {
        perror("Maximum number of open open_sockets reached.");
        return -1;
    }

    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // TODO: Upgrade to IPv6 ?
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ws->HOST, ws->PORT, &hints, &res) != 0) return -1;

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) return -1;

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) return -1;
    // TODO: Which BACKLOG_COUNT to use? As, currently, only one connection is accepted
    listen(sockfd, BACKLOG_COUNT);

    ws->open_sockets[ws->num_open_sockets] = sockfd;
    ws->num_open_sockets++;
    freeaddrinfo(res);
    return 0;
}

int socket_send(int* sockfd, char* message) {
    unsigned long len = strlen(message);
    long bytes_sent = send(*sockfd, message, len, 0);

    // TODO: bytes_sent < len -> send the rest as well
    if (bytes_sent < 0 ) return -1;
    return 0;
}

int socket_receive_all(int *in_fd, char *buf, size_t bufsize) {
    int bytes_received = 0;
    memset(buf, 0, bufsize);

    // Receiving until buffer ends with CLRF (except last byte which is \0)
    while (string_ends_with_empty_line(buf) != 0) {
        // TODO: This might discard the first packet if two are read consecutively
        if (bytes_received >= bufsize - 1) {
            perror("Buffer full before entire package read.");
            return -1;
        }

        debug_print("Receiving data...");
        // Receiving data from stream once
        int n_bytes = recv(
                        *in_fd,
                        // buffer[0 - bytes_received-1] is full of data,
                        // buffer[bytes_received] is where we want to continue to write (the next first byte)
                        buf + bytes_received,
                        // the size of the space from buffer[bytes_received] to buffer[bufsize-1]
                        (bufsize-1) - bytes_received,
                        0
                    );
        printf("Received %d bytes.\n", n_bytes);

        if (n_bytes == -1 || n_bytes == 0) return -1;
        else bytes_received += n_bytes;
    }

    // Making sure buffer ends in \0 for safety
    buf[bufsize-1] = '\0';
    printf("Full Message: \n------ \n%s------\n", buf);

    return 0;
}

int socket_shutdown(webserver *ws, int *sockfd) {
    if (shutdown(*sockfd, SHUT_RDWR) < 0) return -1;

    // remove socket from open_socket list
    for (int i = 0; i < MAX_NUM_OPEN_SOCKETS; i++) {
        if (ws->open_sockets[i] == *sockfd) {
            ws->open_sockets[i] = 0;
            debug_print("Shutting down socket...");
        }
    }

    return 0;
}
