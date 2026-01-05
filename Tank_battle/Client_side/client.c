#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include "protocol.h"
#include "game_render.h"

int sock = 0;
int my_user_id = -1;
int my_player_index = -1; // 0 or 1 in game room
int my_room_id = -1; // Current game room ID
int in_game = 0;
int opponent_id = -1;
GameState current_state;
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message(Message* msg) {
    send(sock, msg, sizeof(Message), 0);
}

void* receive_messages(void* arg) {
    Message msg;
    while (1) {
        int bytes = recv(sock, &msg, sizeof(Message), 0);
        if (bytes <= 0) {
            printf("Disconnected from server\n");
            break;
        }
        
        switch (msg.type) {
            case MSG_RESPONSE:
                if (msg.user_id > 0 && my_user_id < 0) {
                    my_user_id = msg.user_id;
                }
                if (msg.player_index >= 0) {
                    my_player_index = msg.player_index;
                    printf("You are Player %d\n", msg.player_index + 1);
                }
                if (msg.room_id > 0) {
                    my_room_id = msg.room_id;
                    printf("Game Room ID: %d\n", msg.room_id);
                }
                printf("Server: %s\n", msg.message);
                break;
                
            case MSG_SEND_FRIEND_REQUEST:
                printf("Friend request from %s (ID: %d)\n", msg.username, msg.user_id);
                printf("Accept? (y/n): ");
                break;
                
            case MSG_INVITE_TO_ROOM:
                printf("Game invitation from %s (ID: %d)\n", msg.username, msg.user_id);
                printf("Accept? (y/n): ");
                opponent_id = msg.user_id;
                break;
                
            case MSG_GAME_STATE:
                pthread_mutex_lock(&state_mutex);
                memcpy(&current_state, &msg.game_state, sizeof(GameState));
                pthread_mutex_unlock(&state_mutex);
                break;
                
            case MSG_GAME_OVER:
                printf("\n=== GAME OVER ===\n");
                if (msg.user_id == my_user_id) {
                    printf("You WIN!\n");
                } else {
                    printf("You LOSE!\n");
                }
                in_game = 0;
                break;
                
            case MSG_ERROR:
                printf("Error: %s\n", msg.message);
                break;
        }
    }
    return NULL;
}

void register_user() {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_REGISTER;
    
    printf("Username: ");
    scanf("%s", msg.username);
    printf("Email: ");
    scanf("%s", msg.email);
    printf("Password: ");
    scanf("%s", msg.password);
    
    send_message(&msg);
}

void login_user() {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_LOGIN;
    
    printf("Username: ");
    scanf("%s", msg.username);
    printf("Password: ");
    scanf("%s", msg.password);
    
    send_message(&msg);
    sleep(1); // Wait for response
}

void send_friend_request() {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_SEND_FRIEND_REQUEST;
    msg.user_id = my_user_id;
    
    printf("Friend username: ");
    scanf("%s", msg.username);
    
    send_message(&msg);
}

void accept_friend_request() {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_ACCEPT_FRIEND_REQUEST;
    msg.user_id = my_user_id;
    
    printf("Friend ID: ");
    scanf("%d", &msg.target_id);
    
    send_message(&msg);
}

void invite_to_room() {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_INVITE_TO_ROOM;
    msg.user_id = my_user_id;
    
    printf("Friend ID to invite: ");
    scanf("%d", &msg.target_id);
    
    send_message(&msg);
}

void join_room() {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_JOIN_ROOM;
    msg.user_id = my_user_id;
    msg.target_id = opponent_id;
    
    send_message(&msg);
    in_game = 1;
}

void view_history() {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_VIEW_HISTORY;
    msg.user_id = my_user_id;
    
    send_message(&msg);
    sleep(1); // Wait for response
}

void play_game() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return;
    }
    
    SDL_Window* window = SDL_CreateWindow("Tank Battle",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        MAP_WIDTH, MAP_HEIGHT, SDL_WINDOW_SHOWN);
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    SDL_Event e;
    int quit = 0;
    
    while (!quit && in_game) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
                in_game = 0;  
            } else if (e.type == SDL_KEYDOWN) {
                Message msg;
                memset(&msg, 0, sizeof(Message));
                msg.user_id = my_user_id;
                msg.player_index = my_player_index;
                msg.room_id = my_room_id;
                msg.target_id = opponent_id;
                
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        msg.type = MSG_GAME_MOVE;
                        msg.data[0] = 0;
                        send_message(&msg);
                        break;
                    case SDLK_RIGHT:
                        msg.type = MSG_GAME_MOVE;
                        msg.data[0] = 1;
                        send_message(&msg);
                        break;
                    case SDLK_DOWN:
                        msg.type = MSG_GAME_MOVE;
                        msg.data[0] = 2;
                        send_message(&msg);
                        break;
                    case SDLK_LEFT:
                        msg.type = MSG_GAME_MOVE;
                        msg.data[0] = 3;
                        send_message(&msg);
                        break;
                    case SDLK_SPACE:
                        msg.type = MSG_GAME_SHOOT;
                        send_message(&msg);
                        break;
                    case SDLK_ESCAPE:
                        quit = 1;
                        in_game = 0;
                        break;
                }
            }
        }
        
        pthread_mutex_lock(&state_mutex);
        render_game(renderer, &current_state, my_player_index);
        pthread_mutex_unlock(&state_mutex);
        
        SDL_Delay(33); // ~30 FPS
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void show_menu() {
    printf("\n=== Tank Battle Menu ===\n");
    printf("1. Register\n");
    printf("2. Login\n");
    printf("3. Send Friend Request\n");
    printf("4. Accept Friend Request\n");
    printf("5. Invite Friend to Game\n");
    printf("6. Join Game\n");
    printf("7. View History\n");
    printf("8. Exit\n");
    printf("Choice: ");
}

int main() {
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return -1;
    }
    
    printf("Connected to server\n");
    
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_detach(recv_thread);
    
    int choice;
    while (1) {
        if (in_game) {
            play_game();
            continue;
        }
        
        show_menu();
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: register_user(); break;
            case 2: login_user(); break;
            case 3: send_friend_request(); break;
            case 4: accept_friend_request(); break;
            case 5: invite_to_room(); break;
            case 6: join_room(); break;
            case 7: view_history(); break;
            case 8:
                close(sock);
                return 0;
            default:
                printf("Invalid choice\n");
        }
    }
    
    return 0;
}