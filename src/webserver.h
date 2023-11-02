#ifndef RN_PRAXIS_WEBSERVER_H
#define RN_PRAXIS_WEBSERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HOSTNAME_MAX_LENGTH 15
#define EXPECTED_NUMBER_OF_PARAMS 2 // TODO: Good name?
#define MAX_NUM_OPEN_SOCKETS 1

struct {
    char* HOST;
    u_int16_t* PORT;
    int** open_sockets; // TODO: Well typed?
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

#endif //RN_PRAXIS_WEBSERVER_H
