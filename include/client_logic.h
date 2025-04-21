// client_logic.h
#ifndef CLIENT_LOGIC_H
#define CLIENT_LOGIC_H

#include <stddef.h>

extern const char *nameobjets[8];
extern int gClientPort;             // 客户端端口
extern char username[32];           // 玩家姓名
extern char serverIP[256];          // 服务器IP

void sendMessageToServer(char *ipAddress, int portno, char *mess);
void setUsername(const char *name);
const char* getUsername();
int getCurrentPlayer();
int* getMyCards();
int* getObjectCounts();
const char* getLastResult();
int isTurn();
int isGameEnded();
int getIsGameEnded();
const char* getPlayerName(int index);
int getPlayerCount();
int getIsGameStarted();
void connectToServer(const char* ip, int port);
void getLocalIP(char *ip, size_t len);
int isUsernameSet();
int getClientPort();
int getTableValue(int playerId, int objectId);
int isPlayerAlive(int playerId);
int getCurrentPlayer();

#endif

