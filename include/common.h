// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h> // Required for uint32_t, int32_t

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

/* TLV Protocol Definition */

// 1. Message Types - 1 byte
typedef enum {
    MSG_CONNECT     = 0x01, // 'C' - Client to Server: Login/Connect
    MSG_ID_ASSIGN   = 0x02, // 'I' - Server to Client: Player ID Assignment
    MSG_PLAYER_LIST = 0x03, // 'L' - Server to Client: Boardcast Player List
    MSG_DISTRIBUTE  = 0x04, // 'D' - Server to Client: Distribute Cards And Initial Game State
    MSG_TURN        = 0x05, // 'M' - Server to Client: Notify Player's Turn
    MSG_ACTION_O    = 0x06, // 'O' - Client to Server: Ask about an Object
    MSG_ACTION_S    = 0x07, // 'S' - Client to Server: Ask a specific player
    MSG_ACTION_G    = 0x08, // 'G' - Client to Server: Make a Guess
    MSG_VERIFY      = 0x09, // 'V' - Server to Client: Broadcast verification result
	MSG_GAME_OVER   = 0x0A,	// 'E' - Server to Client: Game Over Notification
	MSG_ERROR       = 0xFF	// 'X' - Server to Client: Error Message
} MessageType;

// 2. Fixed-Length Header (Header) - 5 bytes total
typedef struct {	
	uint8_t type;   	// Message Type (1 byte)
	uint32_t length; 	// Payload Length (4 bytes, big-endian)
} __attribute__((packed)) PacketHeader;

// 3. Binary Payload (Value) - Variable length based on the message type

// Payload for MSG_CONNECT (Client to Server)
typedef struct {
	char ip[40]; 	// Client's IP address
	int32_t port;   // Client's port number
	char name[40]; 	// Player's name
} __attribute__((packed)) Payload_Connect;

// Payload for MSG_ID_ASSIGN (Server to Client)
typedef struct {
	int32_t playerId; // Assigned Player ID
	int32_t port;     // Server's port number for the game
} __attribute__((packed)) Payload_ID_Assign;


// Payload for MSG_PLAYER_LIST (Server to Client)
typedef struct {
    int32_t id;
    char name[32];
} __attribute__((packed)) Payload_Player_List;

// Payload for MSG_DISTRIBUTE (Server to Client)
typedef struct {
	int32_t Cards[3]; 		// The 3 cards assigned to the player (encoded as integers)
	int32_t objCounts[8]; 	// Initial counts of each object type (8 types)
} __attribute__((packed)) Payload_Distribute;

// Payload for MSG_TURN (Server to Client)
typedef struct {
	int32_t player_id; // Player ID whose turn it is
} __attribute__((packed)) Payload_Turn;

// Payload for MSG_ACTION_O ask_object (Client to Server)
typedef struct {
	int32_t asking_player_id; // Player ID of the one asking
	int32_t object_id;        // Object ID being asked about (0-7)
}__attribute__((packed)) Payload_Action_O;

// Payload for MSG_ACTION_S ask_player (Client to Server)
typedef struct {
	int32_t asking_player_id; // Player ID of the one asking
	int32_t target_player_id; // Player ID being asked about
	int32_t object_id;        // Object ID being asked about (0-7)
}__attribute__((packed)) Payload_Action_S;

// Payload for MSG_ACTION_G make_guess (Client to Server)
typedef struct {
	int32_t asking_player_id; // Player ID of the one making the guess
	int32_t guessed_card_id; // The card being guessed (encoded as integers)
} __attribute__((packed)) Payload_Action_G;

// Payload for MSG_VERIFY broadcast_result (Server to Client)
typedef struct {
	int32_t result_val; // Result of the action:
							// For ask_object: 0 (No), 1 (Yes)
	int32_t target_player_id; // Player ID involved in the action (-1 for global checks)
	int32_t object_id;        // Object ID involved in the action (0-7, -1 for guesses)
} __attribute__((packed)) Payload_Verify;

// Payload for MSG_GAME_OVER (Server to Client)
typedef struct {
	int32_t player_id; // Player ID of the winner or the loser (-1 if it's a draw)
	int32_t is_winner; // 1=WIN, 0=LOSE, -1=DRAW
} __attribute__((packed)) Payload_Game_Over;

#endif
