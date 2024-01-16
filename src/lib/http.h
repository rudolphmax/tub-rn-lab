#ifndef RN_PRAXIS_HTTP_H
#define RN_PRAXIS_HTTP_H

#include "filesystem/filesystem.h"
#include "../webserver.h"

#define HEADER_FIELD_MAX_COUNT 100
#define HEADER_FIELD_NAME_LENGTH 128
#define HEADER_FIELD_VALUE_LENGTH 1024
#define HEADER_URI_LENGTH 2048
#define HEADER_SPECS_LENGTH 9 // Lengths of technical specs in the req/response header (protocol & method)
#define BODY_INITIAL_SIZE 1024

typedef struct http_header_field {
    char name[HEADER_FIELD_NAME_LENGTH];
    char value[HEADER_FIELD_VALUE_LENGTH];
} http_header_field;

typedef struct http_request_header {
    char* method;
    char* URI;
    char* protocol;
    // Array of header fields
    struct http_header_field* fields;
    int num_fields;
} http_request_header;

typedef struct http_response_header {
    char* protocol;
    int status_code;
    char* status_message;
    // Array of header fields
    struct http_header_field* fields;
    int num_fields;
} http_response_header;

typedef struct http_request {
    struct http_request_header *header;
    char *body;
} http_request;

typedef struct http_response {
    struct http_response_header *header;
    char *body;
} http_response;

/**
 * Creates a new empty request.
 * @param method the request method (e.g.: GET)
 * @param URI the request URI (e.g.: /static/foo)
 * @param body the request body (e.g.: Hello World!)
 * @return the request object. NULL on error.
 */
http_request* request_create(char *method, char *URI, char *body);

/**
 * Frees the given request object.
 * @param req the request object to be freed.
 */
void http_request_free(http_request *req);

/**
 * Creates a new empty response.
 * @return the response object. NULL on error.
 */
http_response *http_response_create(int status_code, char *status_message, char *protocol, char* body);

/**
 * Adds a header field to a given response or request object.
 * @param ptr the response/request object to be modified.
 * @param name the name of the header field (e.g.: Content-Length)
 * @param value the value of the header field (e.g.: 123)
 * @return 0 on success, -1 on error.
 */
int http_add_header_field(void *http_msg, char *name, char *value);

/**
 * Determines whether a given response or request
 * has a header-field with the given name.
 * @param ptr the response/request object to be checked.
 * @param name the name of the field in search
 * @param field_index the index of the field, once found
 * @return 1 if ptr has field, 0 if not
 */
int http_has_header_field(void *ptr, char *name, int *field_index);

/**
 * Converts the given response object into a string.
 * @param res the response object to be converted.
 * @return the response string. NULL on error.
 */
char* http_response_stringify(http_response *res);

/**
 * Frees the given response object.
 * @param req the response object to be freed.
 */
void http_response_free(http_response *res);

/**
 * Handles an incoming TCP connection via HTTP.
 * @param in_fd Socket File Descriptor of the accepted connection.
 * @param ws Webserver object.
 * @param fs File System object.
 * @return 0 on success, -1 on error.
 */
int http_handle(int *in_fd, webserver *ws, file_system *fs);

#endif //RN_PRAXIS_HTTP_H
