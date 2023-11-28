#include "webserver.h"
#include "lib/socket.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    int port_str_len = strlen(port_str)+1; // +1 for \0

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

int parse_header(char* req_string, request *req) {
    int endline_index = strstr(req_string, "\r\n") - req_string;
    char* header_line = calloc(endline_index + 1, sizeof(char)); // + 1 for '\0'
    header_line = strncpy(header_line, req_string, endline_index);
    debug_printv("Header line:", header_line);

    char *delimiter = " ";
    char *ptr = strtok(header_line, delimiter);

    int header_field_num = 0;
    while(ptr != NULL) {
        // printf("%s ", ptr);

        char *cpy_dest = NULL;
        int field_max_length = HEADER_SPECS_LENGTH;
        switch (header_field_num) {
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
        }

        strncpy(cpy_dest, ptr, MIN(strlen(ptr), field_max_length-1));

        // Get next slice
        ptr = strtok(NULL, delimiter);
        header_field_num++;
    }
    free(header_line);
    if (header_field_num != 3) return -1;

    char* content_length_end_index = strstr(req_string, "Content-Length: ") + strlen("Content-Length: ");
    char* next_newline_index = strstr(content_length_end_index, "\r\n");

    int content_length = strtol(content_length_end_index, next_newline_index, 10);
    add_header_field(req, "Content-Length", content_length);
    // FIXME! TODO!

    return 0;
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
            continue;
        }

        int receive_attempts_left = RECEIVE_ATTEMPTS;
        int connection_is_alive = 1;
        while (connection_is_alive) {
            char *buf = calloc(MAX_DATA_SIZE, sizeof(char));

            if (socket_receive_all(&in_fd, buf, MAX_DATA_SIZE) == 0) {
                receive_attempts_left = RECEIVE_ATTEMPTS;

                request *req;
                req = request_create(NULL, NULL, NULL);
                if (req == NULL) {
                    perror("Error initializing request structure");
                }

                response *res = response_create(0, NULL, NULL, NULL);
                if (res == NULL) {
                    perror("Error initializing response structure");
                }

                if (parse_header(buf, req) == 0) {
                    if (strncmp(req->header->method, "GET", 3) == 0) {

                        // validating request URI against filesystem
                        target_node *tnode = fs_find_target(fs, req->header->URI);

                        // the target (.../.../foo) exists
                        if (tnode == NULL) {
                            res->header->status_code = 404;
                        } else {
                            res->header->status_code = 200;
                            strcpy(res->header->status_message, "Ok");

                            inode* target_inode = &(fs->inodes[tnode->target_index]);

                            if (target_inode->n_type == reg_file) { // target is a file not a directory
                                int file_size = 0;
                                uint8_t *file_contents = fs_readf(fs, req->header->URI, &file_size);

                                if (file_size > 0) memcpy(res->body, file_contents, file_size);
                            }
                        }

                    } else if (strncmp(req->header->method, "PUT", 3) == 0) {
                        if (strncmp(req->header->URI, "/dynamic", 8) != 0) { //The access IS NOT permitted
                            res->header->status_code = 403;
                            strcpy(res->header->status_message, "Forbidden");

                        } else {   //The access IS permitted
                            int mkfile_result = fs_mkfile(fs,req->header->URI);

                            if (mkfile_result == -1) {  //Failed to create a target
                                res->header->status_code = 400;

                            } else if (mkfile_result == 0) {   //Successfully creates a target
                                res->header->status_code = 201;
                                strcpy(res->header->status_message, "Created");
                                fs_writef(fs,req->header->URI,req->body);

                            } else if (mkfile_result == -2 && fs->inodes[fs_find_target(fs, req->header->URI)->target_index].n_type == reg_file){ //Successfully overwrites the target with the correct type
                                res->header->status_code = 204;
                                strcpy(res->header->status_message, "No Content");
                                fs_rm(fs, req->header->URI); //Do I remove the entire path?
                                fs_mkfile(fs, req->header->URI);
                                fs_writef(fs,req->header->URI,req->body);

                            } else { //None of the above (probably unnecessary: Damian want's to leave it out, I want to keep it :P)
                                res->header->status_code = 400;
                            }
                        }

                    } else if (strncmp(req->header->method, "DELETE", 6) == 0) {
                        // Only permit access to files in /dynamic
                        if (strncmp(req->header->URI, "/dynamic", 8) != 0) { // The access IS NOT permitted
                            res->header->status_code = 403;
                            strcpy(res->header->status_message, "Forbidden");

                        } else if (fs_find_target(fs, req->header->URI) == NULL) { //The access is permitted, but the file doesn´t exist
                            res->header->status_code = 404;
                            strcpy(res->header->status_message, "Not Found");

                        } else if (fs->inodes[fs_find_target(fs, req->header->URI)->target_index].n_type == reg_file){ //The access is permitted and the file to be deleted has the correct type
                            res->header->status_code = 204;
                            strcpy(res->header->status_message, "No Content");
                            fs_rm(fs, req->header->URI);

                        } else { //None of the above (Incorrect request)
                            res->header->status_code = 400;
                        }

                    } else {
                        res->header->status_code = 501;
                    }

                } else res->header->status_code = 400;

                request_free(req);

                char *res_msg = response_stringify(res);
                socket_send(&in_fd, res_msg);
                response_free(res);

            } else if (errno == ECONNRESET || errno == EINTR || errno == ETIMEDOUT) {
                connection_is_alive = 0;

                if (socket_shutdown(ws, &in_fd) != 0) {
                    perror("Socket shutdown failed.");
                    return -1;
                }
            } else {
                receive_attempts_left--;
                if (receive_attempts_left == 0) connection_is_alive = 0;

                perror("Socket couldn't read package.");
            }
        }
    }
    // TODO: Do we have to free the filesystem?
    return 0;
}

void webserver_print(webserver *ws) {
    printf("Webserver running @ %s:%u.", ws->HOST, *ws->PORT);
    printf("%u open open_sockets: ", ws->num_open_sockets);

    for (int i = 0; i < ws->num_open_sockets; i++) {
        printf("%u", ws->open_sockets[i]);
    }
    printf("\n");
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

    if (socket_listen(ws) < 0) {
        perror("Socket Creation failed.");
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
