// client_logic.c
#include "../include/client_logic.h"
#include "../include/gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>

#define MAX_MSG 512

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
char username[32] = "";
char lastResult[128] = "";
int myCards[3];
int objectCounts[8];
int gameState = 0;
int isMyTurn = 0;
int myClientId = -1;
int gClientPort = 0;

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

void* listenToServer(void *arg) {
    char buffer[MAX_MSG];
    //static char serverIP[40]; // 保存初始连接的服务器IP

    while (1) {
        memset(buffer, 0, MAX_MSG);
        int r = recv(socketClient, buffer, MAX_MSG - 1, 0);
        if (r <= 0) break;

        if (strncmp(buffer, "U OK", 4) == 0) {
            printf("用户名已接受，等待其他玩家...\n");
        } else if (strncmp(buffer, "U ERR", 5) == 0) {
            pthread_mutex_lock(&gameStateMutex);
            strcpy(lastResult, "❌ 用户名重复，请更换！");
            pthread_mutex_unlock(&gameStateMutex);
        } else if (buffer[0] == 'D') {
            sscanf(buffer + 2, "%d %d %d %d %d %d %d %d %d %d %d",
                &myCards[0], &myCards[1], &myCards[2],
                &objectCounts[0], &objectCounts[1], &objectCounts[2], &objectCounts[3],
                &objectCounts[4], &objectCounts[5], &objectCounts[6], &objectCounts[7]);
            pthread_mutex_lock(&gameStateMutex);
            gameState = GAME_STARTED;
            pthread_mutex_unlock(&gameStateMutex);
            printf("🎴 已接收到卡牌数据，游戏开始！\n");
        } else if (buffer[0] == 'D' || buffer[0] == 'M' || buffer[0] == 'V') {
            strncpy(lastResult, buffer + 2, sizeof(lastResult) - 1);
            lastResult[sizeof(lastResult) - 1] = '\0';
            printf("[Client] Message du serveur: %s\n", lastResult);
        } else if (buffer[0] == 'E') {
            strncpy(lastResult, buffer + 2, sizeof(lastResult) - 1);
            lastResult[sizeof(lastResult) - 1] = '\0';
            printf("[Client] Fin de jeu: %s\n", lastResult);
            setShowEndDialog(1);
            strncpy(lastResult, buffer + 2, sizeof(lastResult) - 1);
            lastResult[sizeof(lastResult) - 1] = '\0';
            printf("[Client] Message du serveur: %s\n", lastResult);
        } else if (buffer[0] == 'M') {
            int current;
            sscanf(buffer + 2, "%d", &current);
            
            pthread_mutex_lock(&gameStateMutex);
            isMyTurn = (current == myClientId);
            pthread_mutex_unlock(&gameStateMutex);
        } else if (buffer[0] == 'V') {
            // 验证结果类型 | 目标玩家 | 符号ID
            int resultVal, targetPlayer, symbol;
            if (sscanf(buffer + 2, "%d %d %d", &resultVal, &targetPlayer, &symbol) == 3) {
                char msg[128];
                const char *symbolName = nameobjets[symbol];
                
                if (targetPlayer == -1) {
                    // 全体玩家验证（是否有符号）
                    snprintf(msg, sizeof(msg), "全体玩家 %s 符号：%s",
                            resultVal ? "有" : "没有", symbolName);
                } else {
                    // 单个玩家验证（符号数量）
                    const char *targetName = getPlayerName(targetPlayer);
                    snprintf(msg, sizeof(msg), "玩家 %s 有 %d 个 %s",
                            targetName, resultVal, symbolName);
                }
                
                // 更新最后结果（加锁保护）
                pthread_mutex_lock(&gameStateMutex);
                strncpy(lastResult, msg, sizeof(lastResult)-1);
                pthread_mutex_unlock(&gameStateMutex);
                
                printf("[调试] 验证结果：%s\n", msg);
            } else {
                printf("[错误] 无效的V命令格式：%s\n", buffer);
            }
        } else if (buffer[0] == 'L') {
            pthread_mutex_lock(&playerDataMutex);
            playerCount = 0;
            char *token = strtok(buffer + 2, " ");
            while (token && playerCount < 4) {
                strncpy(playerNames[playerCount], token, 31);
                playerNames[playerCount][31] = '\0';
                playerCount++;
                token = strtok(NULL, " ");
            }
            pthread_mutex_unlock(&playerDataMutex);

            pthread_mutex_lock(&gameStateMutex);
            snprintf(lastResult, sizeof(lastResult), "🎮 当前已加入的玩家列表：%s", buffer + 2);
            pthread_mutex_unlock(&gameStateMutex);

        } else if (buffer[0] == 'E') {
            pthread_mutex_lock(&gameStateMutex);
            strcpy(lastResult, buffer);
            gameState = GAME_ENDED;
            pthread_mutex_unlock(&gameStateMutex);
        } else if (buffer[0] == 'I') { // 处理端口分配
            sscanf(buffer + 2, "%d", &myClientId); // 仅解析客户端ID
            // 不再执行关闭和重新连接的代码
            printf("Assigned client ID: %d\n", myClientId);
        }
    }
    return NULL;
}

void connectToServer(const char *ip, int port) {
    static int isFirstConnect = 1;
    if (isFirstConnect) {
        strcpy(serverIP, ip); // 保存服务器IP
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

    printf("✅ 已连接服务器：%s:%d\n", ip, port);

    pthread_t tid;
    pthread_create(&tid, NULL, listenToServer, NULL);
}

int getIsGameEnded() {
    return gameState == 3;
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