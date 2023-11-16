#include "webserver.h"
#include "lib/socket.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    ws->HOST = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    ws->PORT = calloc(1, sizeof(uint16_t));
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
    int port_str_len = strlen(port_str)+1; // +1 for \0
    memcpy(ws->PORT, port_str, port_str_len * sizeof(char));

    return ws;
}

int headersValid(char* buf, char* URL_copy) {
    int endline_index = strstr(buf, "\r\n") - buf;
    char* header_line = calloc(endline_index, sizeof(char));
    header_line = strncpy(header_line, buf, endline_index);

    char *delimiter = " ";
    char *ptr = strtok(header_line, delimiter);
    int header_field_num = 0;

    // debug_print("Header line:");
    while(ptr != NULL) {
        // printf("%s ", ptr);
        // naechsten Abschnitt erstellen
        ptr = strtok(NULL, delimiter);

        if (header_field_num == 1) {
            URL_copy = calloc(strlen(ptr) + 1, sizeof(char));
            URL_copy = strncpy(URL_copy, ptr, strlen(ptr) + 1);


        }
        header_field_num++;
    }

    // printf("\n");
    free(header_line);

    if (header_field_num != 3) {
        return 0;
    }
    return 1;
}

int webserver_tick(webserver *ws) {
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
            // TODO: Handle non-listening sockets
            continue;
        }

        int receive_attempts_left = RECEIVE_ATTEMPTS;
        int connection_is_alive = 1;
        while (connection_is_alive) {
            char *buf = calloc(MAX_DATA_SIZE, sizeof(char));

            if (socket_receive_all(&in_fd, buf, MAX_DATA_SIZE) == 0) {
                receive_attempts_left = RECEIVE_ATTEMPTS;

                char* URL_copy;
                if (headersValid(buf, URL_copy) == 1) {
                    if (strncmp(buf, "GET", 3) == 0) {

                        size_t URL_len = strlen(URL_copy);
                        if (strcmp(&URL_copy[URL_len - 3], "foo") == 0) {
                            socket_send(&in_fd, "Foo\r\n\r\n");

                        } else if (strcmp(&URL_copy[URL_len - 3], "bar") == 0){
                            socket_send(&in_fd, "Bar\r\n\r\n");

                        } else if (strcmp(&URL_copy[URL_len - 3], "baz") == 0) {
                            socket_send(&in_fd, "Baz\r\n\r\n");

                        } else {
                            socket_send(&in_fd, "HTTP/1.1 404\r\n\r\n");
                        }
                        /*
                        //printf("%s", URL_copy);

                        char *token = strtok(URL_copy, "/");
                        //printf("%s",token);
                        //Task 2.6 but doesnt work yet
                        while (token != NULL) {
                            token = strtok(NULL, "/");
                        }

                        if (strcmp(token, "foo") == 0) {
                            socket_send(&in_fd, "Foo\r\n\r\n");
                        }
                        if (strcmp(token, "bar") == 0) {
                            socket_send(&in_fd, "Bar\r\n\r\n");
                        }
                        if (strcmp(token, "baz") == 0) {
                            socket_send(&in_fd, "Baz\r\n\r\n");
                        }
                        */
                    } else {
                        socket_send(&in_fd, "HTTP/1.1 501\r\n\r\n");
                    }
                } else {
                    socket_send(&in_fd, "HTTP/1.1 400\r\n\r\n");
                }

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
        if (webserver_tick(ws) != 0) quit = 1;
    }

    for (int i = 0; i < ws->num_open_sockets; i++) {
        int *sockfd = &(ws->open_sockets[i]);
        socket_shutdown(NULL, sockfd);
    }

    webserver_free(ws);

    return 0;
}
