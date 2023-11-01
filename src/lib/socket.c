#include "socket.h"

int socket_listen(char *port) {
    struct sockaddr_storage incoming_addr;
    socklen_t incoming_addr_size;
    struct addrinfo hints, *addr_info;
    int sockfd, new_fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // TODO: Upgrade to IPv6 ?
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, port, &hints, &addr_info);

    sockfd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (sockfd < 0) return -1;

    bind(sockfd, addr_info->ai_addr, addr_info->ai_addrlen);
    listen(sockfd, 10); // TODO: Absract BACKLOG to separate configuration

    incoming_addr_size = sizeof(incoming_addr);
    new_fd = accept(sockfd, (struct sockaddr*) &incoming_addr, &incoming_addr_size);

    freeaddrinfo(addr_info);
    return 0;
}
