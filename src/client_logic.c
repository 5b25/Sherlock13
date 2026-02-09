// client_logic.c
#include "../include/client_logic.h"
#include "../include/gui.h"
#include "../include/common.h" // Includes Protocol definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>

// External helper from common.c
extern void send_packet(int sockfd, uint8_t type, const void *payload, uint32_t payload_len);
extern int recv_all(int sockfd, void *buffer, size_t length);

pthread_mutex_t gameStateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t playerDataMutex = PTHREAD_MUTEX_INITIALIZER;

enum {
    GAME_NOT_CONNECTED = 0,
    GAME_WAITING = 1,
    GAME_STARTED = 2,
    GAME_ENDED = 3
};

const char *nameobjets[8] = {
    "Pipe", "Ampoule", "Poing", "Insigne", 
    "Cahier", "Collier", "Oeil", "Crâne"
};

char playerNames[4][32];
int playerCount = 0;

char serverIP[256] = "127.0.0.1";

int socketClient = -1;
int myClientId = -1;
char username[32] = "";
char lastResult[128] = "";
int myCards[3]= {-1, -1, -1};;
int objectCounts[8]= {0};
int gameState = 0;  // 0 = Waiting, 1 = Started, 2 = Ended
int isMyTurn = 0;
int gClientPort = 0;
static int currentTurnPlayerId = -1;

// Object matrices and player states for GUI display
int objectTable[4][8] = {{0}};
int playerAlive[4] = {1, 1, 1, 1};

volatile int synchro = 0;

void setUsername(const char *name) {
    strncpy(username, name, sizeof(username));
}

const char* getUsername() {
    return username;
}

const char* getLastResult() {
    static char safeResult[128];
    pthread_mutex_lock(&gameStateMutex);
    strncpy(safeResult, lastResult, sizeof(safeResult)-1);
    pthread_mutex_unlock(&gameStateMutex);
    return safeResult;
}

int* getMyCards() {
    return myCards;
}

int* getObjectCounts() {
    return objectCounts;
}

int isTurn() {
    int turnStatus;
    pthread_mutex_lock(&gameStateMutex);
    turnStatus = isMyTurn;
    pthread_mutex_unlock(&gameStateMutex);
    return turnStatus;
}

int isGameEnded() {
    return gameState == GAME_ENDED;
}

void getLocalIP(char *ip, size_t len) {
    char hostname[256];
    struct hostent *host;

    gethostname(hostname, sizeof(hostname));
    host = gethostbyname(hostname);
    if (host == NULL) {
        strcpy(ip, "127.0.0.1");
        return;
    }
    snprintf(ip, len, "%s", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
}

void sendMessageToServer(char *ip, int port, char *mess) {
    if (socketClient < 0) return;
    send(socketClient, mess, strlen(mess), 0);
}

// New Binary Action Senders
void sendConnect(const char* name, int port) {
    Payload_Connect pkg;
    strncpy(pkg.name, name, 39);
    pkg.port = port;
    getLocalIP(pkg.ip, 40);
    send_packet(socketClient, MSG_CONNECT, &pkg, sizeof(pkg));
}

void sendActionO(int objId) {
    Payload_Action_O pkg = { .asking_player_id = myClientId, .object_id = objId };
    send_packet(socketClient, MSG_ACTION_O, &pkg, sizeof(pkg));
}

void sendActionS(int targetId, int objId) {
    Payload_Action_S pkg = { .asking_player_id = myClientId, .target_player_id = targetId, .object_id = objId };
    send_packet(socketClient, MSG_ACTION_S, &pkg, sizeof(pkg));
}

void sendActionG(int cardId) {
    Payload_Action_G pkg = { .asking_player_id = myClientId, .guessed_card_id = cardId };
    send_packet(socketClient, MSG_ACTION_G, &pkg, sizeof(pkg));
}

void* listenToServer(void *arg) {
    while (1) {
        PacketHeader header;
        if (recv_all(socketClient, &header, sizeof(PacketHeader)) < 0) {
            printf("Disconnected from server.\n");
            break;
        }

        uint32_t len = ntohl(header.length);
        void *buffer = NULL;
        
        // Receive Payload (if exists)
        if (len > 0) {
            buffer = malloc(len);
            recv_all(socketClient, buffer, len);
        }

        // Lock State for processing
        pthread_mutex_lock(&gameStateMutex);
        switch (header.type) {
            case MSG_ID_ASSIGN: {
                Payload_ID_Assign *p = (Payload_ID_Assign*)buffer;
                myClientId = p->playerId;
                printf("[Client] Assigned ID: %d\n", myClientId);
                break;
            }
            case MSG_PLAYER_LIST: {
                Payload_Player_List *p = (Payload_Player_List*)buffer;
                pthread_mutex_lock(&playerDataMutex);
                if (p->id >= 0 && p->id < 4) {
                    strncpy(playerNames[p->id], p->name, 32);
                    // Update player count based on the highest ID received + 1
                    if (p->id >= playerCount) {
                        playerCount = p->id + 1;
                    }
                }
                pthread_mutex_unlock(&playerDataMutex);
                if (p->id == myClientId) {
                    printf("[Client] Lobby Update: Player %d is %s\n", p->id, p->name);
                }
                break;
            }
            case MSG_DISTRIBUTE: {
                Payload_Distribute *p = (Payload_Distribute*)buffer;
                memcpy(myCards, p->Cards, sizeof(myCards));
                // memcpy(objectCounts, p->objCounts, sizeof(objectCounts)); // If server sends counts
                gameState = GAME_STARTED; // STARTED
                snprintf(lastResult, 128, "Game Started!");

                pthread_mutex_lock(&playerDataMutex);
                playerCount = 4; 
                pthread_mutex_unlock(&playerDataMutex);
                break;
            }
            case MSG_TURN: {
                Payload_Turn *p = (Payload_Turn*)buffer;
                isMyTurn = (p->player_id == myClientId);

                currentTurnPlayerId = p->player_id;
                snprintf(lastResult, 128, "Player %d's Turn", p->player_id);
                break;
            }
            case MSG_VERIFY: {
                Payload_Verify *p = (Payload_Verify*)buffer;
                if (p->target_player_id == -1) {
                    snprintf(lastResult, 128, "Global Check: Object %s %s found", 
                             nameobjets[p->object_id], p->result_val ? "IS" : "NOT");
                } else {
                    snprintf(lastResult, 128, "Player %d has %d of %s", 
                             p->target_player_id, p->result_val, nameobjets[p->object_id]);
                }
                break;
            }
            case MSG_GAME_OVER: {
                Payload_Game_Over *p = (Payload_Game_Over*)buffer;
                if (p->is_winner) {
                    snprintf(lastResult, 128, "Player %d WINS!", p->player_id);
                    setShowEndDialog(1);
                    gameState = GAME_ENDED;
                } else {
                    snprintf(lastResult, 128, "Player %d Eliminated.", p->player_id);
                }
                break;
            }
        }
        pthread_mutex_unlock(&gameStateMutex);
        if (buffer) free(buffer);

        usleep(1000);
    }
    return NULL;
}

void connectToServer(const char *ip, int port) {
    static int isFirstConnect = 1;
    if (isFirstConnect) {
        strcpy(serverIP, ip); // Save Server IP
        isFirstConnect = 0;
    }

    struct sockaddr_in addr;
    socketClient = socket(AF_INET, SOCK_STREAM, 0);
    if (socketClient < 0) {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(socketClient, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("✅ Connected to server: %s:%d\n", ip, port);

    pthread_t tid;
    pthread_create(&tid, NULL, listenToServer, NULL);
}

int getIsGameEnded() {
    return gameState == GAME_ENDED;
}

const char* getPlayerName(int index) {
    const char *name = "";
    pthread_mutex_lock(&playerDataMutex);
    if (index >= 0 && index < 4) {
        name = playerNames[index];
    }
    pthread_mutex_unlock(&playerDataMutex);
    return name;
}

int getPlayerCount() {
    int count;
    pthread_mutex_lock(&playerDataMutex);
    count = playerCount;
    pthread_mutex_unlock(&playerDataMutex);
    return count;
}

int getIsGameStarted() {
    int status;
    pthread_mutex_lock(&gameStateMutex);
    status = (gameState == GAME_STARTED);
    pthread_mutex_unlock(&gameStateMutex);
    return status;
}

int isUsernameSet() {
    return username[0] != '\0';
}

int getClientPort() {
    return gClientPort;
}

int getTableValue(int playerId, int objectId) {
    if (playerId >= 0 && playerId < 4 && objectId >= 0 && objectId < 8) {
        return objectTable[playerId][objectId];
    }
    return 0;
}

int isPlayerAlive(int playerId) {
    if (playerId >= 0 && playerId < 4) {
        return playerAlive[playerId];
    }
    return 0;
}

int getCurrentPlayer() {
    return isTurn() ? myClientId : -1;
}

void updateCurrentTurn(int id) {
    pthread_mutex_lock(&gameStateMutex);
    currentTurnPlayerId = id;
    pthread_mutex_unlock(&gameStateMutex);
}

int getCurrentTurnPlayer() {
    int id;
    pthread_mutex_lock(&gameStateMutex);
    id = currentTurnPlayerId;
    pthread_mutex_unlock(&gameStateMutex);
    return id;
}

int getIsConnected() {
    int connected;
    pthread_mutex_lock(&gameStateMutex);
    connected = (myClientId != -1); // If the ID is not -1, it means the connection is established.
    pthread_mutex_unlock(&gameStateMutex);
    return connected;
}

int getMyClientId() {
    return myClientId;
}