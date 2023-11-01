
#include "webserver.h"
#include "lib/utils.h"
#include "lib/socket.h"

webserver* webserver_init(char* hostname, char* port_str) {
    webserver *ws = calloc(1, sizeof(webserver));
    if (!ws) return NULL;

    ws->HOST = calloc(HOSTNAME_MAX_LENGTH, sizeof(char));
    ws->PORT = calloc(1, sizeof(uint16_t));

    memcpy(ws->HOST, hostname, HOSTNAME_MAX_LENGTH * sizeof(char)); // TODO: Unsafe ? as hostname might be shorter than HOSTNAME_MAX_LENGTH
    memcpy(ws->PORT, port_str, sizeof(port_str)); // TODO: ^ same here ?
    //if (str_to_uint16(port_str, ws->PORT) < 0) return NULL;

    return ws;
}

void webserver_print(webserver *ws) {
    printf("Webserver running @ %s:%u.", ws->HOST, *ws->PORT);
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

    if (socket_listen(ws->PORT) < 0) {
        perror("Socket Creation failed.");
        exit(EXIT_FAILURE);
    }

    webserver_print(ws);

    return 0;
}
