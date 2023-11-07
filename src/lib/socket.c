#include "socket.h"

int socket_accept(int *sockfd) {
    struct sockaddr_storage in_addr;
    socklen_t in_addr_size = sizeof(in_addr);

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

    ws->open_sockets[ws->num_open_sockets] = &sockfd;
    ws->num_open_sockets++;
    freeaddrinfo(res);
    return 0;
}

int socket_send(int* sockfd, char* message) {
    unsigned long len = strlen(message);
    long bytes_sent = send(*sockfd, message, len, 0);

    printf("Sent %lu characters of: '%s'", len, message);
    if (bytes_sent < 0 ) return -1;
    return 0;
}

int socket_receive(int *in_fd, char* buf, size_t bufsize) {
    int n_bytes = recv(*in_fd, buf, bufsize, 0);
    return n_bytes;
}

int socket_receive_all(int *in_fd, char *buf, size_t bufsize) {
    int bytes_received = 1; // 1 for adding/keeping a \0 at the end
    // setting
    memset(buf, 0, bufsize);

    // TODO: Receive until empty line (basically "\r\n\r\n", one CRLF from the previous and one from the empty-line)
    // Receiving until buffer ends with CLRF (except last byte which is \0)
    while (buf[bufsize - 2] != '\r' && buf[bufsize - 1] != '\n') {
        if (bytes_received >= bufsize - 1) {
            perror("Buffer full before entire package read.");
            return -1;
        }

        // Receiving one line of data from stream
        int t = socket_receive(
                in_fd,
                    // buffer[0 - bytes_received-1] is full of data,
                    // buffer[bytes_received] is where we want to continue to write (the next first byte)
                    buf + bytes_received, // TODO: this pointer arithmetic isn't working yet.
                    // the size of the space from buffer[bytes_received] to buffer[bufsize]
                    bufsize - bytes_received
                );

        if (t == -1) return -1;
        else bytes_received += t;
    }

    // Making sure buffer ends in \0 for safety
    buf[bufsize] = '\0';
    return 0;
}

int socket_shutdown(webserver *ws, int *sockfd) {
    if (shutdown(*sockfd, SHUT_RDWR) < 0) return -1;

    // remove socket from open_socket list
    for (int i = 0; i < MAX_NUM_OPEN_SOCKETS; i++) {
        if (*(ws->open_sockets[i]) == *sockfd) {
            ws->open_sockets[i] = NULL;
        }
    }

    return 0;
}
