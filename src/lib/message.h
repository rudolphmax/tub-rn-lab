#ifndef RN_PRAXIS_MESSAGE_H
#define RN_PRAXIS_MESSAGE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HEADER_FIELD_MAX_COUNT 100
#define HEADER_FIELD_NAME_LENGTH 128
#define HEADER_FIELD_VALUE_LENGTH 1024
#define HEADER_URI_LENGTH 2048
#define BODY_INITIAL_SIZE 1024

/*
enum header_method {
    POST,
    PUT,
    GET,
    DELETE,
};
*/

struct header_field {
    char name[HEADER_FIELD_NAME_LENGTH];
    char value[HEADER_FIELD_VALUE_LENGTH];
} typedef header_field;

struct request_header {
    char* method;
    char* URI;
    char* protocol;
    // Array of header fields
    header_field* fields;
    int num_fields;
} typedef request_header;

struct response_header {
    char* protocol;
    int status_code;
    char* status_message;
    // Array of header fields
    header_field* fields;
    int num_fields;
} typedef response_header;

struct request {
    request_header *header;
    char *body;
} typedef request;

struct response {
    response_header *header;
    char *body;
} typedef response;

/**
 * Creates a new empty request.
 * @return the request object. NULL on error.
 */
request* request_create();

/**
 * Frees the given request object.
 * @param req the request object to be freed.
 */
void request_free(request *req);

/**
 * Creates a new empty response.
 * @return the response object. NULL on error.
 */
response *response_create(int status_code, char *status_message, char *protocol, char* body);

/**
 * Returns the size of the given response object.
 * @param res the response object to be measured.
 * @return the size of the response object.
 */
int response_bytesize(response *res);

/**
 * Converts the given response object into a string.
 * @param res the response object to be converted.
 * @return the response string. NULL on error.
 */
char* response_stringify(response *res);

/**
 * Frees the given response object.
 * @param req the response object to be freed.
 */
void response_free(response *res);

#endif //RN_PRAXIS_MESSAGE_H
