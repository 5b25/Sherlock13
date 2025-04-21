
#ifndef COMMON_H
#define COMMON_H

#define DEFAULT_PORT 32000
#define MAX_CLIENTS 4
#define MAX_MSG 1024

extern const char *nomcartes[13];
extern const char *nameobjets[8];

typedef struct {
    char ipAddress[40];
    int port;
    char name[40];
    int playing;
} Client;

#endif
