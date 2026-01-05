#include "game_render.h"
#include <stdio.h>

void render_tank(SDL_Renderer* renderer, Tank* tank, int is_mine) {
    SDL_Rect tank_rect = {tank->x, tank->y, TANK_SIZE, TANK_SIZE};
    
    // Draw tank body
    if (is_mine) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green for player
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for opponent
    }
    SDL_RenderFillRect(renderer, &tank_rect);
    
    // Draw tank direction indicator (barrel)
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    int barrel_length = 20;
    int cx = tank->x + TANK_SIZE/2;
    int cy = tank->y + TANK_SIZE/2;
    
    switch (tank->direction) {
        case 0: // up
            SDL_RenderDrawLine(renderer, cx, cy, cx, cy - barrel_length);
            break;
        case 1: // right
            SDL_RenderDrawLine(renderer, cx, cy, cx + barrel_length, cy);
            break;
        case 2: // down
            SDL_RenderDrawLine(renderer, cx, cy, cx, cy + barrel_length);
            break;
        case 3: // left
            SDL_RenderDrawLine(renderer, cx, cy, cx - barrel_length, cy);
            break;
    }
    
    // Draw health bar
    SDL_Rect health_bg = {tank->x, tank->y - 10, TANK_SIZE, 5};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &health_bg);
    
    SDL_Rect health_bar = {tank->x, tank->y - 10, 
                           (TANK_SIZE * tank->health) / 3, 5};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &health_bar);
}

void render_bullet(SDL_Renderer* renderer, Bullet* bullet) {
    if (!bullet->active) return;
    
    SDL_Rect bullet_rect = {bullet->x, bullet->y, BULLET_SIZE, BULLET_SIZE};
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderFillRect(renderer, &bullet_rect);
}

void render_game(SDL_Renderer* renderer, GameState* state, int my_player_index) {
    // Clear screen (black background)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Draw grid
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    for (int i = 0; i < MAP_WIDTH; i += 50) {
        SDL_RenderDrawLine(renderer, i, 0, i, MAP_HEIGHT);
    }
    for (int i = 0; i < MAP_HEIGHT; i += 50) {
        SDL_RenderDrawLine(renderer, 0, i, MAP_WIDTH, i);
    }
    
    // Render tanks - use player_index (0 or 1) instead of player_id
    for (int i = 0; i < 2; i++) {
        render_tank(renderer, &state->tanks[i], i == my_player_index);
    }
    
    // Render bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        render_bullet(renderer, &state->bullets[i]);
    }
    
    SDL_RenderPresent(renderer);
}