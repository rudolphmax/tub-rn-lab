#include "webserver.h"
#include "lib/socket.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    ws->HOST = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    ws->PORT = calloc(1, sizeof(uint16_t));
    ws->open_sockets = calloc(MAX_NUM_OPEN_SOCKETS, sizeof(int*));
    ws->num_open_sockets = 0;

    memcpy(ws->HOST, hostname, HOSTNAME_MAX_LENGTH * sizeof(char)); // TODO: Unsafe ? as hostname might be shorter than HOSTNAME_MAX_LENGTH
    // TODO: Store port as uint16 (as per TCP/IP standard) and convert back to char* in socket_listen
    memcpy(ws->PORT, port_str, sizeof(port_str)); // TODO: ^ same here ?

    return ws;
}

void webserver_print(webserver *ws) {
    printf("Webserver running @ %s:%u.", ws->HOST, *ws->PORT);
    printf("%u open open_sockets: ", ws->num_open_sockets);

    for (int i; i < ws->num_open_sockets; i++) {
        printf("%u", *(ws->open_sockets[i]));
    }
    printf("\n");
}

int webserver_free(webserver *ws) {
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
        for (int i; i < ws->num_open_sockets; i++) {
            int *sockfd = ws->open_sockets[i];

            if (socket_is_listening(sockfd) > 0) { // socket is listening, so accept
                int *in_fd;
                if (socket_accept(sockfd, in_fd) < 0) {
                    perror("Socket failed to accept.");
                    continue;
                }
            } else continue;
        }

        webserver_print(ws);
    }

    for (int i; i < ws->num_open_sockets; i++) {
        int *sockfd = ws->open_sockets[i];
        socket_shutdown(sockfd);
    }

    // TODO: free webserver

    return 0;
}
