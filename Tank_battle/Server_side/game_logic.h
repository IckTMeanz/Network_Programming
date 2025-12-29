#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "protocol.h"

// Game room structure
typedef struct {
    int room_id;
    int player1_id;
    int player2_id;
    int player1_socket;
    int player2_socket;
    GameState state;
    int active;
    pthread_mutex_t lock;
} GameRoom;

// Initialize game room
void init_game_room(GameRoom* room, int room_id, int p1_id, int p2_id, int p1_sock, int p2_sock);

// Game logic functions
void move_tank(GameState* state, int player_id, int direction);
void shoot_bullet(GameState* state, int player_id);
void update_game_state(GameState* state);
int check_collision(Tank* tank, Bullet* bullet);
int check_game_over(GameState* state);

#endif