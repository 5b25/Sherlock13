// main_client.c
#include "../include/client_logic.h"
#include "../include/gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    const char *serverIP = "127.0.0.1";
    int serverPort = 32000;

    // Mode 1:./client <serverIP> <serverPort> <clientPort> <username>
    if (argc == 5) {
        serverIP = argv[1];
        serverPort = atoi(argv[2]);
        gClientPort = atoi(argv[3]);
        setUsername(argv[4]);

        connectToServer(serverIP, serverPort);

        char ip[40];
        getLocalIP(ip, sizeof(ip));
        char msg[256];
        snprintf(msg, sizeof(msg), "C %s %d %s", ip, gClientPort, getUsername());
        sendMessageToServer("", 0, msg);
    }

    // Mode 2:./client <serverIP> <serverPort> <username>
    else if (argc == 4) {
        serverIP = argv[1];
        serverPort = atoi(argv[2]);
        gClientPort = 0;
        setUsername(argv[3]);

        connectToServer(serverIP, serverPort);
    }

    // Mode 3:./client <clientPort> <username>
    else if (argc == 3) {
        gClientPort = atoi(argv[1]);
        setUsername(argv[2]);

        connectToServer(serverIP, serverPort);

        char ip[40];
        getLocalIP(ip, sizeof(ip));
        char msg[256];
        snprintf(msg, sizeof(msg), "C %s %d %s", ip, gClientPort, getUsername());
        sendMessageToServer("", 0, msg);
    }

    // Mode 4:./client <username>
    else if (argc == 2) {
        gClientPort = 0;
        setUsername(argv[1]);

        connectToServer(serverIP, serverPort);
        
        char msg[128];
        snprintf(msg, sizeof(msg), "C %s", getUsername());
        sendMessageToServer("", 0, msg);
    }

    // Mode 5: No parameters → GUI input
    else {
        connectToServer(serverIP, serverPort);
    }

    run_gui();

    return 0;
}