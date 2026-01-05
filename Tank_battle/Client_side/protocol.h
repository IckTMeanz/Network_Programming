#ifndef PROTOCOL_H
#define PROTOCOL_H

#define PORT 8888
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096
#define MAX_USERNAME 50
#define MAX_PASSWORD 100
#define MAX_EMAIL 100

// Message types
typedef enum {
    MSG_REGISTER = 1,
    MSG_LOGIN,
    MSG_LOGOUT,
    MSG_CREATE_ROOM,
    MSG_SEND_FRIEND_REQUEST,
    MSG_ACCEPT_FRIEND_REQUEST,
    MSG_REJECT_FRIEND_REQUEST,
    MSG_INVITE_TO_ROOM,
    MSG_JOIN_ROOM,
    MSG_VIEW_HISTORY,
    MSG_GAME_MOVE,
    MSG_GAME_SHOOT,
    MSG_GAME_STATE,
    MSG_GAME_OVER,
    MSG_LIST_USERS,
    MSG_LIST_FRIENDS,
    MSG_RESPONSE,
    MSG_ERROR
} MessageType;

// Game constants
#define MAP_WIDTH 800
#define MAP_HEIGHT 600
#define TANK_SIZE 40
#define BULLET_SIZE 10
#define TANK_SPEED 5
#define BULLET_SPEED 10
#define MAX_BULLETS 10

// Tank structure
typedef struct {
    int x, y;
    int direction; // 0:up, 1:right, 2:down, 3:left
    int health;
    int player_id;
} Tank;

// Bullet structure
typedef struct {
    int x, y;
    int direction;
    int active;
    int owner_id;
} Bullet;

// Game state
typedef struct {
    Tank tanks[2];
    Bullet bullets[MAX_BULLETS];
    int game_active;
    int winner_id;
} GameState;

// Message structure
typedef struct {
    MessageType type;
    int user_id;
    int player_index; // 0 or 1 (for game room)
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char email[MAX_EMAIL];
    int target_id;
    int room_id;
    int data[4]; // For game commands (direction, etc)
    GameState game_state; // Full game state for MSG_GAME_STATE
    char message[256];
} Message;

#endif