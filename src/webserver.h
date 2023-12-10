#ifndef RN_PRAXIS_WEBSERVER_H
#define RN_PRAXIS_WEBSERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include "lib/utils.h"
#include "lib/message.h"
#include "lib/filesystem/filesystem.h"
#include "lib/filesystem/operations.h"

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

/**
 * Initializes a new webserver-object from a given hostname and port.
 * @param hostname the server hostname (IPv4)
 * @param port_str the servers port (per TCP/IP spec, a valid uint16) in string-form
 * @return A webserver object on success, NULL on error.
 */
webserver* webserver_init(char* hostname, char* port_str);

/**
 * Validates the HTTP request header and fills a request object.
 * @param req_string request in string form as it came from the stream
 * @param req request object to be filled
 * @param content_length content-length predetermined as received from stream
 * @return 0 on success, -1 on error.
 */
int parse_request(char* req_string, request *req, unsigned int content_length);

/**
 * Processes a GET request and fills a response object.
 * @return 0 on success, -1 on error.
 */
int webserver_process_get(request *req, response *res, file_system *fs);

/**
 * Processes a PUT request and fills a response object.
 * @return 0 on success, -1 on error.
 */
int webserver_process_put(request *req, response *res, file_system *fs);

/**
 * Processes a DELETE request and fills a response object.
 * @return 0 on success, -1 on error.
 */
int webserver_process_delete(request *req, response *res, file_system *fs);

/**
 * Processes a request from a buffer, fills request and response objects.
 * @param buf the buffer containing the request
 * @param content_length content-length predetermined as received from stream
 * @param res the response object to be filled
 * @param req the request object to be filled
 * @param fs the filesystem to be used
 * @return 0 on success, -1 on error.
 */
int webserver_process(char *buf, unsigned int content_length, response *res, request *req, file_system *fs);

/**
 * Executes one lifetime-tick of the given webserver
 * @param ws the webserver to tickle.
 * @return 0 on success, -1 on error
 */
int webserver_tick(webserver *ws, file_system *fs);

/**
 * Prints relevant information on the webserver-object to the console.
 * @param ws The webserver who's info is to be printed
 */
void webserver_print(webserver *ws);

/**
 * Frees the given webserver-object
 * @param ws The webserver to be freed.
 */
void webserver_free(webserver *ws);

#endif //RN_PRAXIS_WEBSERVER_H
