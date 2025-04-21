// main_serveur.c
#include "../include/server_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int port = (argc >= 2) ? atoi(argv[1]) : DEFAULT_PORT;
    printf("Starting server on port %d...\n", port);

    melangerDeck();
    createTable();
    start_server_listener(port);

    return 0;
}
