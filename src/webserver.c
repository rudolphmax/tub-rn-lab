#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "lib/utils.h"
#include "lib/message.h"
#include "lib/socket.h"
#include "lib/filesystem/operations.h"
#include "webserver.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    unsigned int port_str_len = strlen(port_str)+1; // +1 for \0

    ws->HOST = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    ws->PORT = calloc(port_str_len, sizeof(char));
    ws->open_sockets = calloc(MAX_NUM_OPEN_SOCKETS, sizeof(int));
    ws->num_open_sockets = 0;

    if (strlen(hostname)+1 > HOSTNAME_MAX_LENGTH) {
        perror("Invalid hostname");
        return NULL;
    }
    memcpy(ws->HOST, hostname, HOSTNAME_MAX_LENGTH * sizeof(char));

    if (str_is_uint16(port_str) < 0) {
        perror("Invalid port.");
        return NULL;
    }
    memcpy(ws->PORT, port_str, port_str_len * sizeof(char));

    return ws;
}

int parse_request_headers(char* header_string, request *req) {
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

        if (0 != add_header_field(req, name, value)) {
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

int parse_request(char *req_string, request *req) {
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

        if (0 != parse_request_headers(header_string, req)) {
            free(header_string);
            return -1;
        }

        free(header_string);
    }

    int cl_field_index = -1;
    if (has_header_field(req, "Content-Length", &cl_field_index) == 1) {
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

int webserver_process_get(request *req, response *res, file_system *fs) {
    // validating request URI against filesystem
    target_node *tnode = fs_find_target(fs, req->header->URI);

    // the target (.../.../foo) exists
    if (tnode == NULL) {
        res->header->status_code = 404;
        return 0;
    }

    res->header->status_code = 200;
    strcpy(res->header->status_message, "Ok");

    inode* target_inode = &(fs->inodes[tnode->target_index]);

    if (target_inode->n_type == fil) { // target is a file not a directory
        int file_size = 0;
        uint8_t *file_contents = fs_readf(fs, req->header->URI, &file_size);

        if (file_size > 0) memcpy(res->body, file_contents, file_size);

        free(file_contents);
    }

    fs_free_target_node(tnode);
    return 0;
}

int webserver_process_put(request *req, response *res, file_system *fs) {
    if (strncmp(req->header->URI, "/dynamic", 8) != 0) {
        res->header->status_code = 403;
        strcpy(res->header->status_message, "Forbidden");
        return 0;
    }

    //The access IS permitted
    int mkfile_result = fs_mkfile(fs,req->header->URI);
    target_node *tnode = fs_find_target(fs, req->header->URI);

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

int webserver_process_delete(request *req, response *res, file_system *fs) {
    target_node *tnode = fs_find_target(fs, req->header->URI);
    
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

int webserver_process(char *buf, response *res, request *req, file_system *fs) {
    if (parse_request(buf, req) != 0) {
        res->header->status_code = 400;
        return 0;
    }

    if (strncmp(req->header->method, "GET", 3) == 0) {
        return webserver_process_get(req, res, fs);

    } else if (strncmp(req->header->method, "PUT", 3) == 0) {
        return webserver_process_put(req, res, fs);

    } else if (strncmp(req->header->method, "DELETE", 6) == 0) {
        return webserver_process_delete(req, res, fs);

    } else res->header->status_code = 501;

    return 0;
}

int connection_receive_to_buffer(int *in_fd, webserver *ws, file_system *fs) {
    char *buf = calloc(MAX_DATA_SIZE, sizeof(char));

    int bytes_received = socket_receive_all(in_fd, buf, MAX_DATA_SIZE);
    if (bytes_received < 0) {
        if (errno == ECONNRESET || errno == EINTR || errno == ETIMEDOUT) {
            if (socket_shutdown(ws, in_fd) != 0) {
                perror("Socket shutdown failed.");
            }

            free(buf);
            return -1;
        }

        perror("Socket couldn't read package.");

    } else {
        // TODO: determine type of request

        request *req;
        req = request_create(NULL, NULL, NULL);
        if (req == NULL) perror("Error initializing request structure");

        response *res = response_create(0, NULL, NULL, NULL);
        if (res == NULL) perror("Error initializing response structure");

        if (webserver_process(buf, res, req, fs) == 0) {
            char *res_msg = response_stringify(res);
            socket_send(in_fd, res_msg);
            free(res_msg);

        } else perror("Error processing request");

        response_free(res);
        request_free(req);
    }

    free(buf);
    return 0;
}

void handle_connection(int *in_fd, webserver *ws, file_system *fs) {
    int receive_attempts_left = RECEIVE_ATTEMPTS;
    int connection_is_alive = 1;

    while (connection_is_alive) {
        if (connection_receive_to_buffer(in_fd, ws, fs) < 0) {
            receive_attempts_left--;
        } else {
            receive_attempts_left = RECEIVE_ATTEMPTS;
        }

        if (receive_attempts_left == 0) connection_is_alive = 0;
    }
}

int webserver_tick(webserver *ws, file_system *fs) {
    // TODO: Multithread with fork to accept multiple simultaneous connections

    // Deciding what to do for each open socket - are they listening or not?
    for (int i = 0; i < ws->num_open_sockets; i++) {
        int *sockfd = &(ws->open_sockets[i]);

        // TODO: Which socket-fds do we actually need? Which one does what?
        int in_fd = socket_accept(sockfd);
        if (in_fd < 0) {
            if (errno != EINVAL) {
                perror("Socket failed to accept.");
                continue;
            }
            // else: accept failed because socket is unwilling to listen.
            // Handle non-listening sockets here
            // continue;
        }

        handle_connection(&in_fd, ws, fs);
    }

    return 0;
}

void webserver_free(webserver *ws) {
    free(ws->HOST);
    free(ws->PORT);
    free(ws->open_sockets);
    free(ws);
}

int main(int argc, char **argv) {
    if (argc != EXPECTED_NUMBER_OF_PARAMS + 1) {
        perror("Wrong number of args. Usage: ./webserver {ip} {port}");
        exit(EXIT_FAILURE);
    }

    // TODO: Refactor this into a function
    // initializing underlying filesystem
    file_system *fs = fs_create(500); // TODO: Which size to choose?
    fs_mkdir(fs, "/static");
    fs_mkdir(fs, "/dynamic");
    fs_mkfile(fs, "/static/foo");
    fs_writef(fs, "/static/foo", "Foo");
    fs_mkfile(fs, "/static/bar");
    fs_writef(fs, "/static/bar", "Bar");
    fs_mkfile(fs, "/static/baz");
    fs_writef(fs, "/static/baz", "Baz");

    // initializing webserver
    webserver *ws = webserver_init(argv[1], argv[2]);
    if (!ws) {
        perror("Initialization of the webserver failed.");
        exit(EXIT_FAILURE);
    }

    // opening TCP Socket
    if (socket_listen(ws, SOCK_STREAM) < 0) {
        perror("TCP Socket Creation failed.");
        exit(EXIT_FAILURE);
    }

    // opening UDP Socket
    if (socket_listen(ws, SOCK_DGRAM) < 0) {
        perror("UDP Socket Creation failed.");
        exit(EXIT_FAILURE);
    }

    int quit = 0;
    while(!quit) {
        if (webserver_tick(ws, fs) != 0) quit = 1;
    }

    for (int i = 0; i < ws->num_open_sockets; i++) {
        int *sockfd = &(ws->open_sockets[i]);
        socket_shutdown(NULL, sockfd);
    }

    webserver_free(ws);
    fs_free(fs);

    return 0;
}
