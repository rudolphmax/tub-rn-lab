#ifndef RN_PRAXIS_WEBSERVER_H
#define RN_PRAXIS_WEBSERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/utils.h"

#define HOSTNAME_MAX_LENGTH 16 // Max. hostname length INCLUDING \0
#define EXPECTED_NUMBER_OF_PARAMS 2
#define MAX_NUM_OPEN_SOCKETS 1
#define MAX_DATA_SIZE 1024

struct {
    char* HOST;
    char* PORT;
    // Pointer to array of pointers to file descriptors of currently open sockets. Length: MAX_NUM_OPEN_SOCKETS
    int** open_sockets;
    int num_open_sockets;
} typedef webserver;

/*
 * Initializes a new webserver-object from a given hostname and port.
 */
webserver* webserver_init(char* hostname, char* port_str);

/*
 * Prints relevant information on the webserver-object to the console.
 */
void webserver_print(webserver *ws);

/*
 * Frees the given webserver-object
 */
void webserver_free(webserver *ws);

#endif //RN_PRAXIS_WEBSERVER_H
