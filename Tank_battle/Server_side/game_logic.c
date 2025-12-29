#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "game_logic.h"
#include <pthread.h>

void init_game_room(GameRoom* room, int room_id, int p1_id, int p2_id, int p1_sock, int p2_sock) {
    room->room_id = room_id;
    room->player1_id = p1_id;
    room->player2_id = p2_id;
    room->player1_socket = p1_sock;
    room->player2_socket = p2_sock;
    room->active = 1;
    pthread_mutex_init(&room->lock, NULL);

    // Initialize game state
    room->state.game_active = 1;
    room->state.winner_id = -1;

    // Initialize tank 1 (player 1)
    room->state.tanks[0].x = 100;
    room->state.tanks[0].y = MAP_HEIGHT / 2;
    room->state.tanks[0].direction = 1; // facing right
    room->state.tanks[0].health = 3;
    room->state.tanks[0].player_id = p1_id;

    // Initialize tank 2 (player 2)
    room->state.tanks[1].x = MAP_WIDTH - 100;
    room->state.tanks[1].y = MAP_HEIGHT / 2;
    room->state.tanks[1].direction = 3; // facing left
    room->state.tanks[1].health = 3;
    room->state.tanks[1].player_id = p2_id;

    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        room->state.bullets[i].active = 0;
    }
}

void move_tank(GameState* state, int player_id, int direction) {
    Tank* tank = NULL;
    for (int i = 0; i < 2; i++) {
        if (state->tanks[i].player_id == player_id) {
            tank = &state->tanks[i];
            break;
        }
    }

    if (!tank || !state->game_active) return;

    tank->direction = direction;

    int new_x = tank->x;
    int new_y = tank->y;

    switch (direction) {
        case 0: new_y -= TANK_SPEED; break; // up
        case 1: new_x += TANK_SPEED; break; // right
        case 2: new_y += TANK_SPEED; break; // down
        case 3: new_x -= TANK_SPEED; break; // left
    }

    // Boundary check
    if (new_x >= 0 && new_x <= MAP_WIDTH - TANK_SIZE &&
        new_y >= 0 && new_y <= MAP_HEIGHT - TANK_SIZE) {
        tank->x = new_x;
        tank->y = new_y;
    }
}

void shoot_bullet(GameState* state, int player_id) {
    if (!state->game_active) return;

    Tank* tank = NULL;
    for (int i = 0; i < 2; i++) {
        if (state->tanks[i].player_id == player_id) {
            tank = &state->tanks[i];
            break;
        }
    }

    if (!tank) return;

    // Find inactive bullet
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!state->bullets[i].active) {
            state->bullets[i].active = 1;
            state->bullets[i].owner_id = player_id;
            state->bullets[i].direction = tank->direction;
            
            // Position bullet at tank's front
            switch (tank->direction) {
                case 0: // up
                    state->bullets[i].x = tank->x + TANK_SIZE/2 - BULLET_SIZE/2;
                    state->bullets[i].y = tank->y - BULLET_SIZE;
                    break;
                case 1: // right
                    state->bullets[i].x = tank->x + TANK_SIZE;
                    state->bullets[i].y = tank->y + TANK_SIZE/2 - BULLET_SIZE/2;
                    break;
                case 2: // down
                    state->bullets[i].x = tank->x + TANK_SIZE/2 - BULLET_SIZE/2;
                    state->bullets[i].y = tank->y + TANK_SIZE;
                    break;
                case 3: // left
                    state->bullets[i].x = tank->x - BULLET_SIZE;
                    state->bullets[i].y = tank->y + TANK_SIZE/2 - BULLET_SIZE/2;
                    break;
            }
            break;
        }
    }
}

void update_game_state(GameState* state) {
    if (!state->game_active) return;

    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state->bullets[i].active) {
            switch (state->bullets[i].direction) {
                case 0: state->bullets[i].y -= BULLET_SPEED; break;
                case 1: state->bullets[i].x += BULLET_SPEED; break;
                case 2: state->bullets[i].y += BULLET_SPEED; break;
                case 3: state->bullets[i].x -= BULLET_SPEED; break;
            }

            // Check boundary
            if (state->bullets[i].x < 0 || state->bullets[i].x > MAP_WIDTH ||
                state->bullets[i].y < 0 || state->bullets[i].y > MAP_HEIGHT) {
                state->bullets[i].active = 0;
                continue;
            }

            // Check collision with tanks
            for (int j = 0; j < 2; j++) {
                if (state->tanks[j].player_id != state->bullets[i].owner_id) {
                    if (check_collision(&state->tanks[j], &state->bullets[i])) {
                        state->tanks[j].health--;
                        state->bullets[i].active = 0;
                        break;
                    }
                }
            }
        }
    }
}

int check_collision(Tank* tank, Bullet* bullet) {
    return (bullet->x >= tank->x && bullet->x <= tank->x + TANK_SIZE &&
            bullet->y >= tank->y && bullet->y <= tank->y + TANK_SIZE);
}

int check_game_over(GameState* state) {
    for (int i = 0; i < 2; i++) {
        if (state->tanks[i].health <= 0) {
            state->game_active = 0;
            state->winner_id = state->tanks[1-i].player_id;
            return 1;
        }
    }
    return 0;
}