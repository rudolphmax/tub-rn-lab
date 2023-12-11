#ifndef RN_PRAXIS_MESSAGE_H
#define RN_PRAXIS_MESSAGE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "./utils.h"

#define HEADER_FIELD_MAX_COUNT 100
#define HEADER_FIELD_NAME_LENGTH 128
#define HEADER_FIELD_VALUE_LENGTH 1024
#define HEADER_URI_LENGTH 2048
#define HEADER_SPECS_LENGTH 9 // Lengths of technical specs in the req/response header (protocol & method)
#define BODY_INITIAL_SIZE 1024

/*
enum header_method {
    POST,
    PUT,
    GET,
    DELETE,
};
*/

typedef struct header_field {
    char name[HEADER_FIELD_NAME_LENGTH];
    char value[HEADER_FIELD_VALUE_LENGTH];
} header_field;

typedef struct request_header {
    char* method;
    char* URI;
    char* protocol;
    // Array of header fields
    struct header_field* fields;
    int num_fields;
} request_header;

typedef struct response_header {
    char* protocol;
    int status_code;
    char* status_message;
    // Array of header fields
    struct header_field* fields;
    int num_fields;
} response_header;

typedef struct request {
    struct request_header *header;
    char *body;
} request;

typedef struct response {
    struct response_header *header;
    char *body;
} response;

/**
 * Creates a new empty request.
 * @param method the request method (e.g.: GET)
 * @param URI the request URI (e.g.: /static/foo)
 * @param body the request body (e.g.: Hello World!)
 * @return the request object. NULL on error.
 */
request* request_create(char *method, char *URI, char *body);

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
 * Adds a header field to a given response or request object.
 * @param http_msg the response/request object to be modified.
 * @param name the name of the header field (e.g.: Content-Length)
 * @param value the value of the header field (e.g.: 123)
 * @return 0 on success, -1 on error.
 */
int add_header_field(void *http_msg, char *name, char *value);

/**
 * Determines whether a given response or request
 * has a header-field with the given name.
 * @param ptr the response/request object to be checked.
 * @param name the name of the field in search
 * @param field_index the index of the field, once found
 * @return 1 if ptr has field, 0 if not
 */
int has_header_field(void *ptr, char *name, int *field_index);

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
