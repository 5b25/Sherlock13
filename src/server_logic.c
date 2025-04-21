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

int nextAvailablePort = DEFAULT_PORT + 1; // 初始端口号

const char *nomcartes[13] = {
    "Sebastian Moran", "irene Adler", "inspector Lestrade",
    "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
    "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
    "Mrs. Hudson", "Mary Morstan", "James Moriarty"};

const char *nameobjets[8] = {
    "Pipe","Ampoule","Poing","Insigne","Cahier","Collier","Oeil","Crâne"};

// 线程参数结构体
struct thread_args {
    int sockfd;
    int server_port;
};

void melangerDeck() {
    for (int i = 0; i < 13; i++) {
        deck[i] = i;
    }
    for (int i = 12; i > 0; i--) {
        int j = arc4random_uniform(i + 1);  // 使用 arc4random
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void createTable() {
    // 初始化所有玩家的符号计数为0
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            tableCartes[i][j] = 0;
        }
    }

    // 遍历每个玩家的3张卡牌，分配符号
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
}

void processPlayerAction(char *buffer) {
    if (strncmp(buffer, "O", 1) == 0) {
        int id, obj;
        sscanf(buffer, "O %d %d", &id, &obj);
        int found = 0;
        for (int p = 0; p < nbClients; p++) {
            if (tableCartes[p][obj] > 0) {
                found = 1;
                break;
            }
        }
        char vmsg[32];
        sprintf(vmsg, "V %d -1 %d", found, obj);
        broadcastMessage(vmsg);
    } else if (strncmp(buffer, "S", 1) == 0) {
        int id, cible, obj;
        sscanf(buffer, "S %d %d %d", &id, &cible, &obj);
        int c1 = deck[cible * 3 + 0];
        int c2 = deck[cible * 3 + 1];
        int c3 = deck[cible * 3 + 2];
        int count = tableCartes[c1][obj] + tableCartes[c2][obj] + tableCartes[c3][obj];
        char vmsg[32];
        sprintf(vmsg, "V %d %d %d", count, cible, obj);
        broadcastMessage(vmsg);
    } else if (strncmp(buffer, "G", 1) == 0) {
        int id, card;
        sscanf(buffer, "G %d %d", &id, &card);
        if (card == crimeCard) {
            char emsg[32];
            sprintf(emsg, "E %d WIN", id);
            broadcastMessage(emsg);
        } else {
            char emsg[32];
            sprintf(emsg, "E %d LOSE", id);
            broadcastMessage(emsg);
        }
    }

    joueurCourant = (joueurCourant + 1) % nbClients;
    char mmsg[32];
    sprintf(mmsg, "M %d", joueurCourant);
    broadcastMessage(mmsg);
}

void *handle_client(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;
    int sockfd = args->sockfd;
    //int server_port = args->server_port;
    free(arg);

    char buffer[MAX_MSG];
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // 获取对方地址
    getpeername(sockfd, (struct sockaddr *)&addr, &addrlen);

    int r = recv(sockfd, buffer, MAX_MSG - 1, 0);
    if (r <= 0) {
        printf("[Server] Client disconnected. Cleaning up...\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    buffer[r] = '\0'; // 添加字符串终止符
    printf("客户端消息: %s\n", buffer);

    if (strncmp(buffer, "C ", 2) == 0) {
        char clientIP[40] = {0}, clientPortStr[10] = {0}, clientName[40] = {0};
        int num_fields = sscanf(buffer + 2, "%39s %9s %39s", clientIP, clientPortStr, clientName);

        if (num_fields == 3) { // 自定义模式：C <IP> <端口> <姓名>
            tcpClients[nbClients].port = atoi(clientPortStr);
            strcpy(tcpClients[nbClients].ipAddress, clientIP);
            strcpy(tcpClients[nbClients].name, clientName);
        } else if (num_fields == 1) { // 默认模式：C <姓名>
            strcpy(clientName, buffer + 2);
            // 分配新端口
            tcpClients[nbClients].port = nextAvailablePort++;
            // 从套接字获取真实IP
            strcpy(tcpClients[nbClients].ipAddress, inet_ntoa(addr.sin_addr));
            strcpy(tcpClients[nbClients].name, clientName);
        } else {
            printf("Invalid C message format: %s\n", buffer);
            close(sockfd);
            return NULL;
        }

        // 发送分配的端口和客户端ID（ "I" 消息）
        char idmsg[64];
        snprintf(idmsg, sizeof(idmsg), "I %d %d", nbClients, tcpClients[nbClients].port);
        send(sockfd, idmsg, strlen(idmsg), 0);

        // 记录客户端Socket并递增计数
        clientSockets[nbClients] = sockfd;
        nbClients++;

        // 广播玩家列表
        char listmsg[256] = "L";
        for (int i = 0; i < nbClients; i++) {
            strcat(listmsg, " ");
            strcat(listmsg, tcpClients[i].name);
        }
        broadcastMessage(listmsg);

        // 满4人开始游戏
        if (nbClients == nbPlayers) startGame();
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

    printf("主服务器监听中，端口 %d...\n", port);

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
