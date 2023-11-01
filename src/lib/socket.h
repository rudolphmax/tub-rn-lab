//
// Created by Maximilian Rudolph on 01.11.23.
//

#ifndef RN_PRAXIS_SOCKET_H
#define RN_PRAXIS_SOCKET_H

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int socket_listen(char *port);

#endif //RN_PRAXIS_SOCKET_H
