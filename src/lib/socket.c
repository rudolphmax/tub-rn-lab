#include "socket.h"

int socket_accept(int *sockfd) {
    struct sockaddr_storage in_addr;
    socklen_t in_addr_size = sizeof(in_addr);
    debug_print("Accepting connection...");

    return accept(*sockfd, (struct sockaddr*) &in_addr, &in_addr_size);
}

int socket_listen(webserver *ws) {
    if (ws->num_open_sockets >= MAX_NUM_OPEN_SOCKETS) {
        perror("Maximum number of open open_sockets reached.");
        return -1;
    }

    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ws->HOST, ws->PORT, &hints, &res) != 0) return -1;

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) return -1;

    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) return -1;
    listen(sockfd, BACKLOG_COUNT);

    ws->open_sockets[ws->num_open_sockets] = sockfd;
    ws->num_open_sockets++;
    freeaddrinfo(res);
    return 0;
}

int socket_send(int* sockfd, char* message) {
    unsigned long len = strlen(message);
    debug_printv("Sending message:", message);

    long bytes_sent = 0;
    while (bytes_sent < len) {
        bytes_sent += send(*sockfd, message + bytes_sent, len - bytes_sent, 0);
        if (bytes_sent < 0) return -1;
    }

    return 0;
}

int socket_receive_all(int *in_fd, char *buf, size_t bufsize) {
    int bytes_received = 0;
    memset(buf, 0, bufsize);

    long content_length = 0;
    long body_size = 0;

    // Receiving until buffer ends with CLRF (except last byte which is \0)
    while (body_size < content_length+1) {
        if (bytes_received >= bufsize - 1) {
            perror("Buffer full before entire package read.");
            return -1;
        }

        debug_print("Receiving data...");
        // Receiving data from stream once
        int n_bytes = recv(
                        *in_fd,
                        // buffer[0 - bytes_received-1] is full of data,
                        // buffer[bytes_received] is where we want to continue to write (the next first byte)
                        buf + bytes_received,
                        // the size of the space from buffer[bytes_received] to buffer[bufsize-1]
                        (bufsize-1) - bytes_received,
                        0
                    );
        printf("Received %d bytes.\n", n_bytes);

        if (n_bytes == -1 || n_bytes == 0) return -1;

        bytes_received += n_bytes;

        // Catch existing Content-Length header and expect reading body
        char *content_length_header_line = strstr(buf, "\r\nContent-Length: ");
        if (content_length_header_line != NULL) { // Request has content
            char *content_length_str = content_length_header_line + 18;
            char *ptr;
            content_length = strtol(content_length_str, &ptr, 10);
        }

        char *empty_line = strstr(buf, "\r\n\r\n");
        if (empty_line != NULL) {
            if (content_length <= 0) break;
        } else continue; // TODO: Check

        body_size += bytes_received - ((empty_line+4) - buf);
    }

    // Making sure buffer ends in \0 for safety
    buf[bufsize-1] = '\0';
    debug_printv("Full Message: \n------ \n", buf);
    debug_print("\n-----\n");

    return 0;
}

int socket_shutdown(webserver *ws, int *sockfd) {
    if (shutdown(*sockfd, SHUT_RDWR) < 0) return -1;

    // remove socket from open_socket list
    for (int i = 0; i < MAX_NUM_OPEN_SOCKETS; i++) {
        if (ws->open_sockets[i] == *sockfd) {
            ws->open_sockets[i] = 0;
            debug_print("Shutting down socket...");
        }
    }

    return 0;
}
