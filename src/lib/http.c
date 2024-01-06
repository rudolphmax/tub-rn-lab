#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "http.h"
#include "udp.h"
#include "filesystem/operations.h"
#include "socket.h"

http_request* request_create(char *method, char *URI, char *body) {
    http_request_header *req_header = calloc(1, sizeof(http_request_header));
    req_header->method = calloc(HEADER_SPECS_LENGTH, sizeof(char));
    if (method != NULL) {
        strncpy(req_header->method, method, MIN(strlen(method), HEADER_SPECS_LENGTH-1));
    }

    req_header->URI = calloc(HEADER_URI_LENGTH, sizeof(char));
    if (URI != NULL) {
        strncpy(req_header->URI, URI, MIN(strlen(URI), HEADER_URI_LENGTH-1));
    }

    req_header->protocol = calloc(HEADER_SPECS_LENGTH, sizeof(char));
    strcpy(req_header->protocol, "HTTP/1.1");

    req_header->num_fields = 10;
    req_header->fields = calloc(req_header->num_fields, sizeof(http_header_field));

    http_request *req = calloc(1, sizeof(http_request));
    req->header = req_header;

    if (body != NULL) {
        req->body = calloc(strlen(body) + 1, sizeof(char));
        strncpy(req->body, body, strlen(body));
    } else {
        req->body = calloc(BODY_INITIAL_SIZE, sizeof(char));
    }

    return req;
}

void http_request_free(http_request *req) {
    free(req->header->fields);
    free(req->header->protocol);
    free(req->header->method);
    free(req->header->URI);

    free(req->header);
    free(req->body);
    free(req);
}

http_response *http_response_create(int status_code, char *status_message, char *protocol, char *body) {
    http_response_header *res_header = calloc(1, sizeof(http_response_header));

    res_header->protocol = calloc(HEADER_SPECS_LENGTH, sizeof(char));
    if (protocol == NULL) {
        strcpy(res_header->protocol, "HTTP/1.1");
    } else {
        strcpy(res_header->protocol, protocol);
    }

    if (status_code != 0) {
        res_header->status_code = status_code;
    }

    res_header->status_message = calloc(HEADER_FIELD_VALUE_LENGTH, sizeof(char));
    if (status_message != NULL) {
        strcpy(res_header->status_message, status_message);
    }

    res_header->num_fields = 10;
    res_header->fields = calloc(res_header->num_fields, sizeof(http_header_field));

    http_response *res = calloc(1, sizeof(http_response));
    res->header = res_header;

    if (body != NULL) {
        res->body = calloc(strlen(body) + 1, sizeof(char));
        strncpy(res->body, body, strlen(body));
    } else {
        res->body = calloc(BODY_INITIAL_SIZE, sizeof(char));
    }

    return res;
}

int http_add_header_field(void *ptr, char *name, char *value) {
    http_request *http_msg = ptr;

    if (strlen(name) > HEADER_FIELD_NAME_LENGTH || strlen(value) > HEADER_FIELD_VALUE_LENGTH) {
        perror("Header field name or value too long.");
        return -1;
    }

    for (int i = 0; i < http_msg->header->num_fields; i++) {
        if (strlen(http_msg->header->fields[i].name) != 0) continue;

        strncpy(http_msg->header->fields[i].name, name, MIN(strlen(name), HEADER_FIELD_NAME_LENGTH-1));
        strncpy(http_msg->header->fields[i].value, value, MIN(strlen(value), HEADER_FIELD_VALUE_LENGTH-1));
        return 0;
    }

    // No empty field exists, so realloc ->fields and use a new one
    int i = http_msg->header->num_fields + 1;

    if (realloc(http_msg->header->fields, (http_msg->header->num_fields * 2) * sizeof(http_header_field)) == NULL) return -1;

    http_msg->header->num_fields *= 2; // increase num_fields if realloc was successful

    strncpy(http_msg->header->fields[i].name, name, MIN(strlen(name), HEADER_FIELD_NAME_LENGTH-1));
    strncpy(http_msg->header->fields[i].value, value, MIN(strlen(value), HEADER_FIELD_VALUE_LENGTH-1));
    return 0;
}

int http_has_header_field(void *ptr, char *name, int *field_index) {
    http_request *http_msg = ptr;

    for (int i = 0; i < http_msg->header->num_fields; i++) {
        if (strlen(http_msg->header->fields[i].name) == 0) continue;

        int cmp = strncmp(http_msg->header->fields[i].name, name, HEADER_FIELD_NAME_LENGTH);
        if (cmp != 0) continue;

        if (NULL != field_index) *field_index = i;
        return 1;
    }

    return 0;
}

/**
 * Returns the size of the given response object.
 * @param res the response object to be measured.
 * @return the size of the response object.
 */
int http_response_bytesize(http_response *res) {
    unsigned int size = 0;

    size += strlen(res->header->protocol) + 1; // +1 for space
    size += 3; // status code
    size += strlen(res->header->status_message) + 2; // +2 for \r\n

    char *body_bytesize_str = calloc(12, sizeof(char));
    if (strlen(res->body) > 0) {
        if (strlen(res->body) > 99999999999) {
            perror("Body too large.");
            return -1;
        }
    }

    snprintf(body_bytesize_str, 12, "%lu", strlen(res->body));
    http_add_header_field(res, "Content-Length", body_bytesize_str);
    free(body_bytesize_str);

    for (int i = 0; i < res->header->num_fields; i++) {
        if (strlen(res->header->fields[i].name) == 0) continue;

        size += strlen(res->header->fields[i].name) + 1; // +1 for :
        size += strlen(res->header->fields[i].value) + 2; // +2 for \r\n
    }

    size += 2; // \r\n
    size += strlen(res->body);

    // TODO: Its missing exactly this amount of bytes (Says Valgrind). Why?
    size += 3;
    size += strlen(res->body);

    return size;
}

char* http_response_stringify(http_response *res) {
    int res_size = http_response_bytesize(res);

    if (res_size == 0) { // response is empty
        res_size = 1;
    } else if (res_size < 0) {
        perror("Could not stringify response.");
        return NULL;
    }

    char *res_str = calloc(res_size, sizeof(char));
    strcat(res_str, res->header->protocol);
    strcat(res_str, " ");

    char *status_code_str = calloc(4, sizeof(char));
    snprintf(status_code_str, 4, "%d", res->header->status_code);
    strcat(res_str, status_code_str);
    free(status_code_str);

    strcat(res_str, " ");

    strcat(res_str, res->header->status_message);
    strcat(res_str, "\r\n");

    for (int i = 0; i < res->header->num_fields; i++) {
        if (strlen(res->header->fields[i].name) == 0) continue;

        strcat(res_str, res->header->fields[i].name);
        strcat(res_str, ": ");
        strcat(res_str, res->header->fields[i].value);
        strcat(res_str, "\r\n");
    }

    strcat(res_str, "\r\n");

    if (strlen(res->body) > 0) {
        strcat(res_str, res->body);
    }

    return res_str;
}

void http_response_free(http_response *res) {
    free(res->header->fields);
    free(res->header->protocol);
    free(res->header->status_message);

    free(res->header);
    free(res->body);
    free(res);
}

/**
 * Parses header-fields from a given string into a request object.
 * @param header_string string to be parsed.
 * @param req request object to be filled
 * @return 0 on success, -1 on error
 */
int http_parse_request_headers(char* header_string, http_request *req) {
    char *delim = "\r\n";
    char *ptr = strtok(header_string, delim);

    while (NULL != ptr) {
        // finding separating ': ' in the line
        char *value_start = strstr(ptr, ": ") + 2;
        if (NULL == value_start) return -1;

        // determining lengths of name and value
        unsigned int value_len = strlen(ptr) - ((value_start) - ptr);
        unsigned int name_len = strlen(ptr) - value_len - 2;

        // copying to temporaries
        char *name = calloc(HEADER_FIELD_NAME_LENGTH, sizeof(char));
        char *value = calloc(HEADER_FIELD_VALUE_LENGTH, sizeof(char));
        strncpy(name, ptr, MIN(HEADER_FIELD_NAME_LENGTH-1, name_len));
        strncpy(value, value_start, MIN(HEADER_FIELD_VALUE_LENGTH-1, value_len));

        if (0 != http_add_header_field(req, name, value)) {
            free(name);
            free(value);
            return -1;
        }
        free(name);
        free(value);

        ptr = strtok(NULL, delim);
    }

    return 0;
}

/**
 * Validates the HTTP request header and fills a request object.
 * @param req_string request in string form as it came from the stream
 * @param req request object to be filled
 * @return 0 on success, -1 on error.
 */
int http_parse_request(char *req_string, http_request *req) {
    unsigned int endline_index = strstr(req_string, "\r\n") - req_string;
    char* request_line = calloc(endline_index + 1, sizeof(char)); // + 1 for '\0'
    request_line = strncpy(request_line, req_string, endline_index);

    debug_printv("Header line:", request_line);

    char *delimiter = " ";
    char *ptr = strtok(request_line, delimiter);

    int request_line_field_num = 0;
    while(ptr != NULL) {
        // printf("%s ", ptr);

        char *cpy_dest = NULL;
        unsigned int field_max_length = HEADER_SPECS_LENGTH;
        switch (request_line_field_num) {
            case 0:
                cpy_dest = req->header->method;
                break;

            case 1:
                cpy_dest = req->header->URI;
                field_max_length = HEADER_URI_LENGTH;
                break;

            case 2:
                cpy_dest = req->header->protocol;
                break;

            default: break;
        }

        strncpy(cpy_dest, ptr, MIN(strlen(ptr), field_max_length-1));

        // Get next slice
        ptr = strtok(NULL, delimiter);
        request_line_field_num++;
    }
    free(request_line);

    if (request_line_field_num != 3) return -1;

    unsigned int emptyline = strstr(req_string, "\r\n\r\n") - req_string;

    int header_string_len = emptyline - (endline_index + 2);
    if (0 < header_string_len) {
        char *header_string = calloc(header_string_len+1, sizeof(char));
        strncpy(header_string, req_string + (endline_index + 2), header_string_len);

        if (0 != http_parse_request_headers(header_string, req)) {
            free(header_string);
            return -1;
        }

        free(header_string);
    }

    int cl_field_index = -1;
    if (http_has_header_field(req, "Content-Length", &cl_field_index) == 1) {
        unsigned int content_length = strtol(req->header->fields[cl_field_index].value, NULL, 10);

        if (content_length == 0) return 0;

        char *body = req_string + (emptyline + 4); // body starts after emptyline

        if (strlen(body) != content_length) {
            debug_print("Content-Length does not match body length!");
            return -1;
        }

        // reallocating body if it's too small
        if (content_length > strlen(req->body)-1) { // -1 because of \0
            unsigned int new_size = 2*strlen(body);
            if (realloc(req->body,new_size * sizeof(char)) == NULL) return -1;
            memset(req->body, 0, new_size);
        }

        strncpy(req->body, body, content_length);
    }

    return 0;
}

/**
 * Processes a GET request and fills a response object.
 * @return 0 on success, -1 on error.
 */
int http_process_get(http_request *req, http_response *res, struct file_system *fs) {
    // validating request URI against filesystem
    struct target_node *tnode = fs_find_target(fs, req->header->URI);

    // the target (.../.../foo) exists
    if (tnode == NULL) {
        res->header->status_code = 404;
        return 0;
    }

    res->header->status_code = 200;
    strcpy(res->header->status_message, "Ok");

    struct inode * target_inode = &(fs->inodes[tnode->target_index]);

    if (target_inode->n_type == fil) { // target is a file not a directory
        int file_size = 0;
        uint8_t *file_contents = fs_readf(fs, req->header->URI, &file_size);

        if (file_size > 0) memcpy(res->body, file_contents, file_size);

        free(file_contents);
    }

    fs_free_target_node(tnode);
    return 0;
}

/**
 * Processes a PUT request and fills a response object.
 * @return 0 on success, -1 on error.
 */
int http_process_put(http_request *req, http_response *res, struct file_system *fs) {
    if (strncmp(req->header->URI, "/dynamic", 8) != 0) {
        res->header->status_code = 403;
        strcpy(res->header->status_message, "Forbidden");
        return 0;
    }

    //The access IS permitted
    int mkfile_result = fs_mkfile(fs,req->header->URI);
    struct target_node *tnode = fs_find_target(fs, req->header->URI);

    if (mkfile_result == -1) {  // Failed to create a target
        res->header->status_code = 400;

    } else if (mkfile_result == 0) {   //Successfully created the target
        res->header->status_code = 201;
        strcpy(res->header->status_message, "Created");
        fs_writef(fs,req->header->URI,req->body);

    } else if (mkfile_result == -2 && fs->inodes[tnode->target_index].n_type == fil){ //Successfully overwrites the target with the correct type
        res->header->status_code = 204;
        strcpy(res->header->status_message, "No Content");
        fs_rm(fs, req->header->URI); //Do I remove the entire path?
        fs_mkfile(fs, req->header->URI);
        fs_writef(fs,req->header->URI,req->body);

    } else { //None of the above (probably unnecessary: Damian want's to leave it out, I want to keep it :P)
        res->header->status_code = 400;
    }

    if (tnode != NULL) fs_free_target_node(tnode);
    return 0;
}

/**
 * Processes a DELETE request and fills a response object.
 * @return 0 on success, -1 on error.
 */
int http_process_delete(http_request *req, http_response *res, struct file_system *fs) {
    struct target_node *tnode = fs_find_target(fs, req->header->URI);

    if (tnode == NULL) { // The file doesn't exist
        res->header->status_code = 404;
        strcpy(res->header->status_message, "Not Found");
        return 0;
    }

    if (strncmp(req->header->URI, "/dynamic", 8) != 0) { // The access IS NOT permitted
        res->header->status_code = 403;
        strcpy(res->header->status_message, "Forbidden");

    } else if (fs->inodes[tnode->target_index].n_type == fil) {
        if (fs_rm(fs, req->header->URI) != 0) return -1;

        res->header->status_code = 204;
        strcpy(res->header->status_message, "No Content");

    } else res->header->status_code = 400;

    fs_free_target_node(tnode);
    return 0;
}

/**
 * Fill a given response object with a redirect-response.
 * @param res The response object to be used.
 * @param status_code The redirect's status code (has to be 3xx)
 * @param location The location to redirect to (gets sent as header)
 * @return 0 on success, -1 on error
 */
int http_redirect(http_response *res, unsigned int status_code, char *location) {
    if (status_code < 300 || status_code > 399) return -1;

    switch (status_code) {
        case 303:
            strcpy(res->header->status_message, "See Other");
            break;

        default:
            return -1;
    }

    res->header->status_code = status_code;
    http_add_header_field(res, "Content-Length", "0");
    http_add_header_field(res, "Location", location);

    return 0;
}

/**
 * Processes a request from a buffer, fills request and response objects.
 * @param content_length content-length predetermined as received from stream
 * @param res the response object to be filled
 * @param req the request object to be filled
 * @param fs the filesystem to be used
 * @return 0 on success, -1 on error.
 */
int http_process_request(webserver *ws, http_response *res, http_request *req, struct file_system *fs) {
    if (req == NULL) {
        res->header->status_code = 400;
        return 0;
    }

    if (ws->node != NULL) { // Server is a node in a DHT
        unsigned short self_responsible = 0;
        unsigned short succ_responsible = 0;

        int h = hash(req->header->URI);

        if (ws->node->pred->ID > ws->node->ID) {
            if (ws->node->ID >= h || ws->node->pred->ID < h) self_responsible = 1;
        } else if (ws->node->ID >= h && ws->node->pred->ID < h) self_responsible = 1;

        if (ws->node->ID > ws->node->succ->ID) {
            if (h <= ws->node->succ->ID || h > ws->node->ID) succ_responsible = 1;
        } else if (ws->node->succ->ID >= h && ws->node->ID < h) succ_responsible = 1;

        if (succ_responsible == 1) {
            // -> redirect to successor
            unsigned int red_loc_len = 9 + strlen(ws->node->succ->IP) + strlen(ws->node->succ->PORT) + strlen(req->header->URI);
            char *red_loc = calloc(red_loc_len, sizeof(char));
            snprintf(red_loc, red_loc_len, "http://%s:%s%s", ws->node->succ->IP, ws->node->succ->PORT,
                     req->header->URI);

            http_redirect(res, 303, red_loc);
            return 0;

        } else if (self_responsible == 0) {
            // TODO: Check if the mistakes are here (may be)
            int udp_sock = -1;
            for (int i = 0; i < ws->num_open_sockets; i++) {
                if (ws->open_sockets_config[i].is_server_socket == 0 || ws->open_sockets_config[i].protocol != 1) continue;

                udp_sock = ws->open_sockets[i].fd;
            }
            if (udp_sock == -1) return -1;

            udp_packet *packet = udp_packet_create(0, h, ws->node->ID, ws->HOST, ws->PORT);
            if (udp_send_to_node(ws, &udp_sock, packet, ws->node->succ) < 0) {
                perror("Error sending to node.");
            }
            udp_packet_free(packet);

            strcpy(res->header->status_message, "Service Unavailable");
            res->header->status_code = 503;
            http_add_header_field(res, "Retry-After", "1");
            return 0;
        }
    }

    if (strncmp(req->header->method, "GET", 3) == 0) {
        return http_process_get(req, res, fs);

    } else if (strncmp(req->header->method, "PUT", 3) == 0) {
        return http_process_put(req, res, fs);

    } else if (strncmp(req->header->method, "DELETE", 6) == 0) {
        return http_process_delete(req, res, fs);

    } else res->header->status_code = 501;

    return 0;
}

int http_handle_connection(int *in_fd, webserver *ws, file_system *fs) {
    char *buf = calloc(MAX_DATA_SIZE, sizeof(char));

    int bytes_received = socket_receive_all(in_fd, buf, MAX_DATA_SIZE);
    if (bytes_received <= 0) {
        if (errno == ECONNRESET || errno == EINTR || errno == ETIMEDOUT) {
            if (socket_shutdown(ws, in_fd) != 0) {
                perror("Socket shutdown failed.");
            }

            free(buf);
        }

        perror("Socket couldn't read package.");
        return -1;

    } else {
        http_request *req;
        req = request_create(NULL, NULL, NULL);
        if (req == NULL) perror("Error initializing request structure");

        http_response *res = http_response_create(0, NULL, NULL, NULL);

        if (http_parse_request(buf, req) != 0) req = NULL;

        if (http_process_request(ws, res, req, fs) == 0) {
            char *res_msg = http_response_stringify(res);
            socket_send(ws, in_fd, res_msg, strlen(res_msg), NULL, 0);
            free(res_msg);

        } else perror("Error processing request");

        http_response_free(res);
        http_request_free(req);
    }

    free(buf);
    return 0;
}
