#ifndef RN_PRAXIS_WEBSERVER_H
#define RN_PRAXIS_WEBSERVER_H

#include "lib/filesystem/filesystem.h"

#define HOSTNAME_MAX_LENGTH 16 // Max. hostname length INCLUDING \0
#define EXPECTED_NUMBER_OF_PARAMS 2
#define MAX_NUM_OPEN_SOCKETS 10
#define MAX_DATA_SIZE 1024
#define RECEIVE_ATTEMPTS 1 // The amount of times the server should retry receiving from a socket if an error occurs

typedef struct webserver {
    char* HOST;
    char* PORT;
    // Array of file descriptors (int) of currently open sockets. Length: MAX_NUM_OPEN_SOCKETS
    int* open_sockets;
    int num_open_sockets;
} webserver;

enum connection_protocol {
    HTTP,
    UDP
};

typedef struct th_args {
    int* in_fd;
    webserver* ws;
    file_system* fs;
    enum connection_protocol protocol;
} th_args;

/**
 * Initializes a new webserver-object from a given hostname and port.
 * @param hostname the server hostname (IPv4)
 * @param port_str the servers port (per TCP/IP spec, a valid uint16) in string-form
 * @return A webserver object on success, NULL on error.
 */
webserver* webserver_init(char* hostname, char* port_str);

/**
 * Executes one lifetime-tick of the given webserver
 * @param ws the webserver to tickle.
 * @return 0 on success, -1 on error
 */
int webserver_tick(webserver *ws, file_system *fs);

/**
 * Frees the given webserver-object
 * @param ws The webserver to be freed.
 */
void webserver_free(webserver *ws);

#endif //RN_PRAXIS_WEBSERVER_H
