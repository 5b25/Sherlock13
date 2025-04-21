
#ifndef SERVER_LOGIC_H
#define SERVER_LOGIC_H

#include "common.h"

extern Client tcpClients[MAX_CLIENTS];
extern int nbClients;
extern int deck[13];
extern int tableCartes[4][8];
extern int joueurCourant;
extern int fsmServer;
extern int nbPlayers;

void melangerDeck();
void createTable();
void printDeck();
void printClients();
int findClientByName(char *name);
void sendMessageToClient(char *clientip,int clientport,char *mess);
void broadcastMessage(char *mess);
int getNextAvailablePort();
void start_server_listener(int port);

#endif
