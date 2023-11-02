#include "socket.h"

int socket_accept(int *sockfd, int *in_fd) {
    struct sockaddr_storage* *in_addr;
    socklen_t in_addr_size = sizeof(in_addr);
    *in_fd = accept(*sockfd, (struct sockaddr*) &in_addr, &in_addr_size);

    if (*in_fd < 0) return -1;
    return 0;
}

int socket_listen(webserver *ws) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // TODO: Upgrade to IPv6 ?
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ws->HOST, ws->PORT, &hints, &res);

    if (ws->num_open_sockets >= MAX_NUM_OPEN_SOCKETS) {
        perror("Maximum number of open open_sockets reached.");
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) return -1;

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) return -1;
    // TODO: Which BACKLOG to use? As, currently, only one connection is accepted
    listen(sockfd, 10); // TODO: Absract BACKLOG to separate configuration

    ws->open_sockets[ws->num_open_sockets] = &sockfd;
    ws->num_open_sockets++;
    freeaddrinfo(res);
    return 0;
}

int socket_shutdown(int *sockfd) {
    shutdown(*sockfd, SHUT_RDWR);
}

int socket_is_listening(int *sockfd) {
    int func;
    int len = sizeof(func);

    if (getsockopt(*sockfd, SOL_SOCKET, SO_ACCEPTCONN, &func, &len) < 0) return -1;

    return func;
}
