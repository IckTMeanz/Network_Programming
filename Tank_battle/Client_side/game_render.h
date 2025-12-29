#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include <SDL2/SDL.h>
#include "protocol.h"

void render_game(SDL_Renderer* renderer, GameState* state, int my_id);

#endif