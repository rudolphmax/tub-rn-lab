#ifndef RN_PRAXIS_SOCKET_H
#define RN_PRAXIS_SOCKET_H

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * Opens a listening socket on the given port @ localhost
 * and accepts a single connection.
 * The socket's file descriptor is written to sockfd
 */
int socket_listen(char *port, int *sockfd);

int socket_accept(int *sockfd, int *in_fd);

int socket_close(int *sockfd);

#endif //RN_PRAXIS_SOCKET_H
