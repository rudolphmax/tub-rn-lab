#include "socket.h"

int socket_accept(int *sockfd, int *in_fd) {
    struct sockaddr_storage* *in_addr;
    socklen_t in_addr_size = sizeof(in_addr);
    *in_fd = accept(*sockfd, (struct sockaddr*) &in_addr, &in_addr_size);

    if (*in_fd < 0) return -1;
    return 0;
}

int socket_listen(char *port, int *sockfd) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // TODO: Upgrade to IPv6 ?
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, port, &hints, &res);

    *sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) return -1;

    if (bind(*sockfd, res->ai_addr, res->ai_addrlen) < 0) return -1;
    // TODO: Which BACKLOG to use? As, currently, only one connection is accepted
    listen(*sockfd, 10); // TODO: Absract BACKLOG to separate configuration

    freeaddrinfo(res);
    return 0;
}

int socket_close(int *sockfd) {
    shutdown(*sockfd, SHUT_RDWR);
}
