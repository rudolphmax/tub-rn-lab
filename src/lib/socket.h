#ifndef RN_PRAXIS_SOCKET_H
#define RN_PRAXIS_SOCKET_H

#include <sys/socket.h>
#include "../webserver.h"

#define BACKLOG_COUNT 10

/**
 * Opens a listening socket for the given webserver (which provides PORT & HOST).
 * The resulting socket's file descriptor is written to ws->open_sockets.
 * @param ws the webserver to open the socket for
 * @param socktype socket-type corresponding to the AI_SOCKTYPE of addrinfo
 * @return 0 on success, -1 on error
 */
int socket_open(webserver *ws, int socktype);

/**
 * Accepts connections on a given socket and fills `in_fd` with the connection's file descriptor.
 * @param sockfd accepting socket's file descriptor
 * @return incoming connection's socket-fd
 */
int socket_accept(int *sockfd);


/**
 * Accepts connections on a given socket and fills `in_fd` with the connection's file descriptor.
 * @param sockfd accepting socket's file descriptor
 * @param message the message to be sent
 * @return 0 on success, -1 on error (just like sys/send)
 */
int socket_send(webserver *ws, int *sockfd, char *message);

/**
 * Receives data from an incoming socket, until CRLF is received.
 * @param in_fd incoming socket file descriptor
 * @param buf char buffer to write data to
 * @param bufsize size of buf
 * @return number of bytes received on success, -1 on error
 */
int socket_receive_all(int *in_fd, char *buf, size_t bufsize);

/**
 * Shuts both sides of a given socket down.
 * @param ws the webserver the socket open on.
 * @param sockfd the connected socket's file descriptor
 * @return 0 on success, -1 on error
 */
int socket_shutdown(webserver *ws, int *sockfd);

#endif //RN_PRAXIS_SOCKET_H
