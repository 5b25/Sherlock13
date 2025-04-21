// server_logic.c
#include "../include/server_logic.h"
#include "../include/common.h"
#include <stdio.h>
#include <bsd/stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <string.h>

Client tcpClients[MAX_CLIENTS];
int clientSockets[MAX_CLIENTS];
int nbClients = 0;
int nbPlayers = 4;
int fsmServer = 0;
int deck[13];
int tableCartes[4][8];
int joueurCourant = 0;
int crimeCard = -1;

// Mark player status
int playerAlive[4] = {1, 1, 1, 1}; // 1=active, 0=out

int nextAvailablePort = DEFAULT_PORT + 1; // Initial port number

const char *nomcartes[13] = {
    "Sebastian Moran", "irene Adler", "inspector Lestrade",
    "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
    "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
    "Mrs. Hudson", "Mary Morstan", "James Moriarty"};

const char *nameobjets[8] = {
    "Pipe","Ampoule","Poing","Insigne","Cahier","Collier","Oeil","Crâne"};

// Thread parameter structure
struct thread_args {
    int sockfd;
    int server_port;
};

void melangerDeck() {
    for (int i = 0; i < 13; i++) {
        deck[i] = i;
    }
    for (int i = 12; i > 0; i--) {
        int j = arc4random_uniform(i + 1);  // Use arc4random
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void createTable() {
    // Initialize all players' symbol counts to 0
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            tableCartes[i][j] = 0;
        }
    }

    // Iterate through each player's 3 cards, assigning symbols
    for (int player = 0; player < 4; player++) {
        int cards[3] = {deck[player*3], deck[player*3+1], deck[player*3+2]};
        for (int c = 0; c < 3; c++) {
            int card = cards[c];
            switch (card) {
                case 0: // Sebastian Moran
                    tableCartes[player][7]++; // Crâne
                    tableCartes[player][2]++; // Poing
                    break;
                case 1: // Irene Adler
                    tableCartes[player][7]++; // Crâne
                    tableCartes[player][1]++; // Ampoule
                    tableCartes[player][5]++; // Collier
                    break;
                case 2: // Inspector Lestrade
                    tableCartes[player][3]++; // Insigne
                    tableCartes[player][6]++; // Oeil
                    tableCartes[player][4]++; // Cahier
                    break;
                case 3: // Inspector Gregson
                    tableCartes[player][3]++; // Insigne
                    tableCartes[player][2]++; // Poing
                    tableCartes[player][4]++; // Cahier
                    break;
                case 4: // Inspector Baynes
                    tableCartes[player][3]++; // Insigne
                    tableCartes[player][1]++; // Ampoule
                    break;
                case 5: // Inspector Bradstreet
                    tableCartes[player][3]++; // Insigne
                    tableCartes[player][2]++; // Poing
                    break;
                case 6: // Inspector Hopkins
                    tableCartes[player][3]++; // Insigne
                    tableCartes[player][0]++; // Pipe
                    tableCartes[player][6]++; // Oeil
                    break;
                case 7: // Sherlock Holmes
                    tableCartes[player][0]++; // Pipe
                    tableCartes[player][1]++; // Ampoule
                    tableCartes[player][2]++; // Poing
                    break;
                case 8: // John Watson
                    tableCartes[player][0]++; // Pipe
                    tableCartes[player][6]++; // Oeil
                    tableCartes[player][2]++; // Poing
                    break;
                case 9: // Mycroft Holmes
                    tableCartes[player][0]++; // Pipe
                    tableCartes[player][1]++; // Ampoule
                    tableCartes[player][4]++; // Cahier
                    break;
                case 10: // Mrs. Hudson
                    tableCartes[player][0]++; // Pipe
                    tableCartes[player][5]++; // Collier
                    break;
                case 11: // Mary Morstan
                    tableCartes[player][4]++; // Cahier
                    tableCartes[player][5]++; // Collier
                    break;
                case 12: // James Moriarty
                    tableCartes[player][7]++; // Crâne
                    tableCartes[player][1]++; // Ampoule
                    break;
            }
        }
    }
}

void printDeck() {
    for (int i = 0; i < 13; i++)
        printf("%d: %s\n", i, nomcartes[deck[i]]);
}

void printClients() {
    for (int i = 0; i < nbClients; i++) {
        printf("%d: %s %d %s\n", i, tcpClients[i].ipAddress,
               tcpClients[i].port, tcpClients[i].name);
    }
}

void startGame() {
    melangerDeck();
    createTable();
    crimeCard = deck[12];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        playerAlive[i] = 1;
    }

    for (int i = 0; i < nbClients; i++) {
        char msg[256];
        int c1 = deck[i * 3 + 0];
        int c2 = deck[i * 3 + 1];
        int c3 = deck[i * 3 + 2];

        sprintf(msg, "D %d %d %d", c1, c2, c3);
        for (int j = 0; j < 8; j++) {
            int count = tableCartes[c1][j] + tableCartes[c2][j] + tableCartes[c3][j];
            char tmp[10];
            sprintf(tmp, " %d", count);
            strcat(msg, tmp);
        }
        send(clientSockets[i], msg, strlen(msg), 0);
    }

    joueurCourant = 0;
    char mmsg[32];
    sprintf(mmsg, "M %d", joueurCourant);
    broadcastMessage(mmsg);

    // Broadcasts each player's object information for GUI table display
    for (int pid = 0; pid < nbClients; pid++) {
        for (int oid = 0; oid < 8; oid++) {
            char tmsg[32];
            sprintf(tmsg, "T %d %d %d", pid, oid, tableCartes[pid][oid]);
            broadcastMessage(tmsg);
            usleep(5000);  // Avoid client processing too quickly
        }
    }
}

void processPlayerAction(char *buffer) {
    if (strncmp(buffer, "O ", 2) == 0) {
        int obj;
        if (sscanf(buffer, "O %d", &obj) == 1) {
            // 检查是否有存活玩家持有该物品
            int found = 0;
            for (int p = 0; p < nbClients; p++) {
                if (playerAlive[p] && tableCartes[p][obj] > 0) {
                    found = 1;
                    break;
                }
            }
            // 广播结果 V found -1 obj
            char vmsg[32];
            snprintf(vmsg, sizeof(vmsg), "V %d -1 %d", found, obj);
            broadcastMessage(vmsg);
            
            // 切换到下一存活玩家
            advanceToNextPlayer();
        }
    } else if (strncmp(buffer, "S ", 2) == 0) {
        int idJoueur, target_id, obj;
        if (sscanf(buffer, "S %d %d %d", &idJoueur, &target_id, &obj) == 3) {
            // 输入有效性检查
            if (target_id < 0 || target_id >= nbClients || obj < 0 || obj >= 8) {
                sendError(idJoueur, "INVALID_S_CMD");
                return;
            }
    
            // 检查目标玩家是否存活
            if (!playerAlive[target_id]) {
                sendError(idJoueur, "TARGET_DEAD");
                return;
            }
    
            // 统计符号数量并广播
            int count = tableCartes[target_id][obj];
            char vmsg[32];
            snprintf(vmsg, sizeof(vmsg), "V %d %d %d", count, target_id, obj);
            broadcastMessage(vmsg);
    
            // 切换到下一玩家
            advanceToNextPlayer();
        }
    } else if (strncmp(buffer, "G ", 2) == 0) {
        int idJoueur, card;
        if (sscanf(buffer, "G %d %d", &idJoueur, &card) == 2) {
            if (card == crimeCard) {
                char emsg[32];
                sprintf(emsg, "E %d WIN", idJoueur);
                broadcastMessage(emsg);
                fsmServer = GAME_ENDED;
            } else {
                char emsg[32];
                sprintf(emsg, "E %d LOSE", idJoueur);
                broadcastMessage(emsg);
                playerAlive[idJoueur] = 0; // 标记为淘汰
                advanceToNextPlayer(); // 切换到下一存活玩家
            }
        }
    }
}

// 切换到下一个存活玩家
void advanceToNextPlayer() {
    int aliveCount = 0;
    for (int i = 0; i < nbClients; i++) {
        if (playerAlive[i]) aliveCount++;
    }
    if (aliveCount == 0) {
        broadcastMessage("E -1 DRAW");
        return;
    }

    int attempts = 0;
    do {
        joueurCourant = (joueurCourant + 1) % nbClients;
        attempts++;
    } while (attempts < nbClients && !playerAlive[joueurCourant]);

    if (playerAlive[joueurCourant]) {
        char mmsg[32];
        sprintf(mmsg, "M %d", joueurCourant);
        broadcastMessage(mmsg);
    } else {
        broadcastMessage("E -1 DRAW"); // 所有玩家淘汰
    }
}

void *handle_client(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;
    int sockfd = args->sockfd;
    //int server_port = args->server_port;
    free(arg);

    char buffer[MAX_MSG];
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // Get player name
    getpeername(sockfd, (struct sockaddr *)&addr, &addrlen);

    int r = recv(sockfd, buffer, MAX_MSG - 1, 0);
    if (r <= 0) {
        printf("[Server] Client disconnected. Cleaning up...\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    buffer[r] = '\0'; // Add a string terminator
    printf("Client message:%s\n", buffer);

    if (strncmp(buffer, "C ", 2) == 0) {
        char clientIP[40] = {0}, clientPortStr[10] = {0}, clientName[40] = {0};
        int num_fields = sscanf(buffer + 2, "%39s %9s %39s", clientIP, clientPortStr, clientName);

        if (num_fields == 3) { // Custom Mode: C <IP> <Port> <Name>
            tcpClients[nbClients].port = atoi(clientPortStr);
            strcpy(tcpClients[nbClients].ipAddress, clientIP);
            strcpy(tcpClients[nbClients].name, clientName);
        } else if (num_fields == 1) { // Custom Mode: C <name>
            strcpy(clientName, buffer + 2);
            // Assigning a new port
            tcpClients[nbClients].port = nextAvailablePort++;
            // Get real IP from socket
            strcpy(tcpClients[nbClients].ipAddress, inet_ntoa(addr.sin_addr));
            strcpy(tcpClients[nbClients].name, clientName);
        } else {
            printf("Invalid C message format: %s\n", buffer);
            close(sockfd);
            return NULL;
        }

        // Send the assigned port and client ID ("I" message)
        char idmsg[64];
        snprintf(idmsg, sizeof(idmsg), "I %d %d", nbClients, tcpClients[nbClients].port);
        send(sockfd, idmsg, strlen(idmsg), 0);

        // Record the client socket and increment the count
        clientSockets[nbClients] = sockfd;
        nbClients++;

        // Broadcast Player List
        char listmsg[256] = "L";
        for (int i = 0; i < nbClients; i++) {
            strcat(listmsg, " ");
            strcat(listmsg, tcpClients[i].name);
        }
        broadcastMessage(listmsg);

        // Start the game with 4 players
        if (nbClients == nbPlayers) { 
            startGame(); 
        } else if (nbClients >= MAX_CLIENTS) {
            fprintf(stderr, "Max clients reached. Rejecting connection.\n");
            close(sockfd);
            pthread_exit(NULL);
        }
    } else {
        processPlayerAction(buffer);
    }

    while (1) {
        memset(buffer, 0, MAX_MSG);
        r = recv(sockfd, buffer, MAX_MSG - 1, 0);
        if (r <= 0) {
            printf("[Server] Client %d disconnected\n", sockfd);
            close(sockfd);
            break;
        }
        buffer[r] = '\0';
        processPlayerAction(buffer);
    }

    pthread_exit(NULL);
}

void start_server_listener(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(1);
    }

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    pthread_t thread_id;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server is listening on port %d...\n", port);

    while (1) {
        struct thread_args *args = malloc(sizeof(struct thread_args));
        args->sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (args->sockfd < 0) {
            perror("accept");
            free(args);
            continue;
        }
        args->server_port = port;

        if (pthread_create(&thread_id, NULL, handle_client, args) != 0) {
            perror("pthread_create");
            close(args->sockfd);
            free(args);
            continue;
        }
        pthread_detach(thread_id);
    }
}

void sendMessageToClient(char *clientip, int clientport, char *mess) {
    for (int i = 0; i < nbClients; i++) {
        if (strcmp(tcpClients[i].ipAddress, clientip) == 0 && 
            tcpClients[i].port == clientport) {
            if (send(clientSockets[i], mess, strlen(mess), MSG_NOSIGNAL) == -1) {
                perror("send");
                close(clientSockets[i]);
            }
            break;
        }
    }
}

void broadcastMessage(char *mess) {
    for (int i = 0; i < nbClients; i++) {
        if (send(clientSockets[i], mess, strlen(mess), MSG_NOSIGNAL) == -1) {
            perror("send");
            close(clientSockets[i]);
        }
    }
}

int getNextAvailablePort() {
    static int basePort = 32001;
    return basePort++;
}

void sendError(int clientId, const char* errorType) {
    if (clientId < 0 || clientId >= nbClients) return;
    char errmsg[64];
    snprintf(errmsg, sizeof(errmsg), "E %s", errorType);
    send(clientSockets[clientId], errmsg, strlen(errmsg), 0);
}
