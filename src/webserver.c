#include "webserver.h"
#include "lib/socket.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    ws->HOST = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    ws->PORT = calloc(1, sizeof(uint16_t));
    ws->open_sockets = calloc(MAX_NUM_OPEN_SOCKETS, sizeof(int*));
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

void webserver_print(webserver *ws) {
    printf("Webserver running @ %s:%u.", ws->HOST, *ws->PORT);
    printf("%u open open_sockets: ", ws->num_open_sockets);

    for (int i = 0; i < ws->num_open_sockets; i++) {
        printf("%u", *(ws->open_sockets[i]));
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

    // TODO: How to exit?
    while(1) {
        // TODO: Multithread with fork to accept multiple simultaneous connections

        // Deciding what to do for each open socket - are they listening or not?
        for (int i = 0; i < ws->num_open_sockets; i++) {
            int *sockfd = ws->open_sockets[i];

            int in_fd = socket_accept(sockfd);
            if (in_fd < 0) {
                if (errno != EINVAL) {
                    perror("Socket failed to accept.");
                    continue;
                }
                // accept failed because socket is unwilling to listen.
                // TODO: Handle non-listening sockets
            }
        }
    }

    for (int i = 0; i < ws->num_open_sockets; i++) {
        int *sockfd = ws->open_sockets[i];
        socket_shutdown(NULL, sockfd);
    }

    webserver_free(ws);

    return 0;
}
