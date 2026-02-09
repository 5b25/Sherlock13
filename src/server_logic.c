// server_logic.c
#include "../include/server_logic.h"
#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h> // Linux Epoll
#include <errno.h>
#include <fcntl.h>

#define MAX_EVENTS 64
#define THREAD_POOL_SIZE 4

// Global Game State
Client tcpClients[MAX_CLIENTS];
int clientSockets[MAX_CLIENTS];
int nbClients = 0;
int nbPlayers = 4;
int deck[13];
int tableCartes[4][8];
int joueurCourant = 0;
int crimeCard = -1;
int playerAlive[4] = {1, 1, 1, 1};
int gameStarted = 0;

// Synchronization
pthread_mutex_t gameMutex = PTHREAD_MUTEX_INITIALIZER;

// Thread Pool Task Queue
typedef struct Task {
    int clientSock;
    PacketHeader header;
    void *payload;
    struct Task *next;
} Task;

typedef struct {
    Task *front, *rear;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int stop;
} TaskQueue;

TaskQueue taskQueue;
pthread_t threadPool[THREAD_POOL_SIZE];

/* --- Helper Prototypes --- */
void send_packet(int sockfd, uint8_t type, const void *payload, uint32_t payload_len);
int recv_all(int sockfd, void *buffer, size_t length);
void handle_logic(int clientSock, uint8_t type, void *data, uint32_t len);

/* --- Game Logic Helpers --- */
void melangerDeck() {
    for (int i = 0; i < 13; i++) deck[i] = i;
    for (int i = 12; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void createTable() {
    // Initialize all players' symbol counts to 0
    memset(tableCartes, 0, sizeof(tableCartes));

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

/* --- Thread Pool Implementation --- */

void init_queue(TaskQueue *q) {
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->stop = 0;
}

void enqueue_task(TaskQueue *q, int sock, PacketHeader h, void *p) {
    Task *t = malloc(sizeof(Task));
    t->clientSock = sock;
    t->header = h;
    t->payload = p;
    t->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->rear) q->rear->next = t;
    else q->front = t;
    q->rear = t;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

Task dequeue_task(TaskQueue *q) {
    Task t;
    pthread_mutex_lock(&q->lock);
    while (q->front == NULL && !q->stop) {
        pthread_cond_wait(&q->cond, &q->lock);
    }
    if (q->stop) {
        pthread_mutex_unlock(&q->lock);
        exit(0);
    }
    Task *temp = q->front;
    t = *temp;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    pthread_mutex_unlock(&q->lock);
    return t;
}

void *worker_thread(void *arg) {
    while (1) {
        Task task = dequeue_task(&taskQueue);
        // Process Business Logic protected by Mutex inside handle_logic if needed
        handle_logic(task.clientSock, task.header.type, task.payload, ntohl(task.header.length));
        if (task.payload) free(task.payload);
    }
    return NULL;
}

/* --- Core Network Logic (Epoll Main Loop) --- */

void set_nonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0) return;
    fcntl(sock, F_SETFL, opts | O_NONBLOCK);
}

void start_server_listener(int port) {
    int listenSock, epollFd;
    struct sockaddr_in addr;
    struct epoll_event ev, events[MAX_EVENTS];

    // 1. Init Thread Pool
    init_queue(&taskQueue);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&threadPool[i], NULL, worker_thread, NULL);
    }

    // 2. Init Socket
    listenSock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    bind(listenSock, (struct sockaddr *)&addr, sizeof(addr));
    listen(listenSock, 10);
    set_nonblocking(listenSock);

    // 3. Init Epoll
    epollFd = epoll_create1(0);
    ev.events = EPOLLIN | EPOLLET; // Edge Triggered
    ev.data.fd = listenSock;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, listenSock, &ev);

    printf("[Server] Listening on port %d using Epoll + ThreadPool...\n", port);

    while (1) {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listenSock) {
                // Handle New Connection
                struct sockaddr_in cli_addr;
                socklen_t len = sizeof(cli_addr);
                int connSock = accept(listenSock, (struct sockaddr *)&cli_addr, &len);
                if (connSock >= 0) {
                    // Remove the non-blocking setting from the client socket to avoid improper use of non-blocking I/O.
                    // set_nonblocking(connSock);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = connSock;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, connSock, &ev);
                    printf("[Server] New connection: Socket %d\n", connSock);
                }
            } else {
                // Handle Data from Client
                int fd = events[i].data.fd;
                PacketHeader header;
                
                // Read Header first
                if (recv_all(fd, &header, sizeof(PacketHeader)) < 0) {
                    // Disconnected
                    close(fd);
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
                    // Remove client from list logic...
                    continue;
                }

                uint32_t len = ntohl(header.length);
                void *payload = NULL;
                if (len > 0) {
                    payload = malloc(len);
                    if (recv_all(fd, payload, len) < 0) {
                        free(payload);
                        close(fd);
                        continue;
                    }
                }

                // Push to Thread Pool
                enqueue_task(&taskQueue, fd, header, payload);
            }
        }
    }
}

/* --- Business Logic (Executed by Worker Threads) --- */

void broadcast_packet(uint8_t type, const void *payload, uint32_t len) {
    for (int i = 0; i < nbClients; i++) {
        send_packet(clientSockets[i], type, payload, len);
    }
}

void advance_turn() {
    int attempts = 0;
    do {
        joueurCourant = (joueurCourant + 1) % nbClients;
        attempts++;
    } while (!playerAlive[joueurCourant] && attempts <= nbClients);

    Payload_Turn turnPkg = { .player_id = joueurCourant };
    broadcast_packet(MSG_TURN, &turnPkg, sizeof(turnPkg));
}

void handle_logic(int clientSock, uint8_t type, void *data, uint32_t len) {
    pthread_mutex_lock(&gameMutex);

    // Get Client Index
    int clientId = -1;
    for(int i=0; i<nbClients; i++) if(clientSockets[i] == clientSock) clientId = i;
    (void)clientId;

    switch (type) {
        case MSG_CONNECT: {
            // 0. Validate Connection
            // Ignore if the room is full
            if (nbClients >= MAX_CLIENTS) {
                printf("[Server] Connection rejected: Server full.\n");
                break; 
            }
            
            // Prevent repeated logins to the same Socket
            int isAlreadyRegistered = 0;
            for (int i = 0; i < nbClients; i++) {
                if (clientSockets[i] == clientSock) {
                    isAlreadyRegistered = 1;
                    break;
                }
            }
            if (isAlreadyRegistered) {
                printf("[Server] Ignored duplicate MSG_CONNECT from Socket %d\n", clientSock);
                break; // End without assigning a new ID
            }

            Payload_Connect *pkg = (Payload_Connect*)data;

            // Filter invalid requests with empty names
            if (strlen(pkg->name) == 0) {
                 printf("[Server] Ignored connection with empty name.\n");
                 break;
            }
            
            // 1. Register new client
            clientSockets[nbClients] = clientSock;
            strncpy(tcpClients[nbClients].name, pkg->name, 32);
            tcpClients[nbClients].port = pkg->port;
            strcpy(tcpClients[nbClients].ipAddress, pkg->ip);
            
            // 2. End ID Assignment
            Payload_ID_Assign idPkg = { .playerId = nbClients, .port = 0 };
            send_packet(clientSock, MSG_ID_ASSIGN, &idPkg, sizeof(idPkg));
            
            // 3. Broadcast Player List
            for (int i = 0; i < nbClients; i++) {
                Payload_Player_List listPkg;
                listPkg.id = i;
                strncpy(listPkg.name, tcpClients[i].name, 32);
                send_packet(clientSock, MSG_PLAYER_LIST, &listPkg, sizeof(listPkg));
            }

            nbClients++;

            // 4. Broadcast the new player to all clients (including self)
            Payload_Player_List newPlayerPkg;
            newPlayerPkg.id = nbClients;
            strncpy(newPlayerPkg.name, tcpClients[nbClients].name, 32);
            broadcast_packet(MSG_PLAYER_LIST, &newPlayerPkg, sizeof(newPlayerPkg));

            // 5. Check if game should start
            if (nbClients == nbPlayers && !gameStarted) {
                printf("[Server] 4 Players connected. Starting game...\n");
                gameStarted = 1;
                melangerDeck();
                createTable();
                crimeCard = deck[12];
                
                // Distribute Cards
                for(int i=0; i<nbPlayers; i++) {
                    Payload_Distribute distPkg;
                    distPkg.Cards[0] = deck[i*3];
                    distPkg.Cards[1] = deck[i*3+1];
                    distPkg.Cards[2] = deck[i*3+2];
                    
                    // Calc initial visible objects 
                    int c1=distPkg.Cards[0], c2=distPkg.Cards[1], c3=distPkg.Cards[2];
                    for(int j=0; j<8; j++) {
                        distPkg.objCounts[j] = tableCartes[c1][j] + tableCartes[c2][j] + tableCartes[c3][j]; 
                        // distPkg.objCounts[j] = 0;
                    }
                    send_packet(clientSockets[i], MSG_DISTRIBUTE, &distPkg, sizeof(distPkg));
                }
                
                // Broadcast First Turn
                Payload_Turn turnPkg = { .player_id = 0 };
                broadcast_packet(MSG_TURN, &turnPkg, sizeof(turnPkg));
            }
            break;
        }
        case MSG_ACTION_O: {
            Payload_Action_O *pkg = (Payload_Action_O*)data;
            int found = 0;
            // Logic: Check if any ALIVE player has object
            for (int p=0; p<nbClients; p++) {
                if(playerAlive[p] && tableCartes[p][pkg->object_id] > 0) found = 1;
            }
            Payload_Verify res = { .result_val = found, .target_player_id = -1, .object_id = pkg->object_id };
            broadcast_packet(MSG_VERIFY, &res, sizeof(res));
            advance_turn();
            break;
        }
        case MSG_ACTION_S: {
            Payload_Action_S *pkg = (Payload_Action_S*)data;
            int count = tableCartes[pkg->target_player_id][pkg->object_id];
            Payload_Verify res = { .result_val = count, .target_player_id = pkg->target_player_id, .object_id = pkg->object_id };
            broadcast_packet(MSG_VERIFY, &res, sizeof(res));
            advance_turn();
            break;
        }
        case MSG_ACTION_G: {
            Payload_Action_G *pkg = (Payload_Action_G*)data;
            if (pkg->guessed_card_id == crimeCard) {
                Payload_Game_Over over = { .player_id = pkg->asking_player_id, .is_winner = 1 };
                broadcast_packet(MSG_GAME_OVER, &over, sizeof(over));
                gameStarted = 0;
            } else {
                Payload_Game_Over over = { .player_id = pkg->asking_player_id, .is_winner = 0 };
                broadcast_packet(MSG_GAME_OVER, &over, sizeof(over));
                playerAlive[pkg->asking_player_id] = 0;
                advance_turn();
            }
            break;
        }
    }
    pthread_mutex_unlock(&gameMutex);
}