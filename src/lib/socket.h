#ifndef RN_PRAXIS_SOCKET_H
#define RN_PRAXIS_SOCKET_H

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../webserver.h"
#include "utils.h"

#define BACKLOG_COUNT 10

/*
 * Opens a listening socket on the given port @ localhost
 * and accepts a single connection.
 * The socket's file descriptor is written to sockfd
 */
int socket_listen(webserver *ws);

/*
 * Accepts connections on a given socket and fills `in_fd`
 * with the connection's file descriptor.
 */
int socket_accept(int *sockfd);


/*
 * Accepts connections on a given socket and fills `in_fd`
 * with the connection's file descriptor.
 */
int socket_send(int *sockfd, char* message);

/**
 * Performs a one-line receive on an incoming socket
 * Wrapper for sys/scoket/recv
 * @param in_fd incoming socket file descriptor
 * @param buf char buffer to write to
 * @param bufsize size of buf
 * @return 0 on success, -1 on error
 */
int socket_receive(int *in_fd, char *buf, size_t bufsize);

/**
 * Receives data from an incoming socket, until CRLF is received.
 * @param in_fd incoming socket file descriptor
 * @param buf char buffer to write data to
 * @param bufsize size of buf
 * @return 0 on success, -1 on error
 */
int socket_receive_all(int *in_fd, char *buf, size_t bufsize);

/*
 * Shuts a given socket down safely.
 */
int socket_shutdown(webserver *ws, int *sockfd);

/*
 * Determines whether a given socket (by file descriptor) is listening.
 * @returns >0 when the socket is listening, 0 when it's not & <0 on error.
 */
int socket_is_listening(int *sockfd);

#endif //RN_PRAXIS_SOCKET_H
