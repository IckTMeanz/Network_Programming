#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"
#include "database.h"
#include "game_logic.h"

#define MAX_ROOMS 50

typedef struct {
    int socket;
    int user_id;
    char username[MAX_USERNAME];
    int active;
} Client;

Client clients[MAX_CLIENTS];
GameRoom rooms[MAX_ROOMS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;
int next_room_id = 1;

void send_message(int socket, Message* msg) {
    send(socket, msg, sizeof(Message), 0);
}

int find_client_by_id(int user_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].user_id == user_id) {
            return i;
        }
    }
    return -1;
}

GameRoom* find_room_by_players(int p1_id, int p2_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active && 
            ((rooms[i].player1_id == p1_id && rooms[i].player2_id == p2_id) ||
             (rooms[i].player1_id == p2_id && rooms[i].player2_id == p1_id))) {
            return &rooms[i];
        }
    }
    return NULL;
}

void* game_loop(void* arg) {
    GameRoom* room = (GameRoom*)arg;
    Message msg;

    while (room->active && room->state.game_active) {
        pthread_mutex_lock(&room->lock);
        
        update_game_state(&room->state);
        
        if (check_game_over(&room->state)) {
            // Send game over message
            msg.type = MSG_GAME_OVER;
            msg.user_id = room->state.winner_id;
            
            send_message(room->player1_socket, &msg);
            send_message(room->player2_socket, &msg);
            
            // Save history
            if (room->state.winner_id == room->player1_id) {
                save_game_history(room->player1_id, room->player2_id, "WIN");
                save_game_history(room->player2_id, room->player1_id, "LOSE");
            } else {
                save_game_history(room->player2_id, room->player1_id, "WIN");
                save_game_history(room->player1_id, room->player2_id, "LOSE");
            }
            
            room->active = 0;
            pthread_mutex_unlock(&room->lock);
            break;
        }
        
        // Send game state to both players
        msg.type = MSG_GAME_STATE;
        memcpy(msg.data, &room->state, sizeof(GameState));
        
        send_message(room->player1_socket, &msg);
        send_message(room->player2_socket, &msg);
        
        pthread_mutex_unlock(&room->lock);
        
        usleep(33000); // ~30 FPS
    }
    
    return NULL;
}

void handle_message(int client_idx, Message* msg) {
    Message response;
    memset(&response, 0, sizeof(Message));
    
    switch (msg->type) {
        case MSG_REGISTER: {
            int result = register_user(msg->username, msg->email, msg->password);
            response.type = MSG_RESPONSE;
            response.user_id = result;
            if (result > 0) {
                strcpy(response.message, "Registration successful");
            } else {
                strcpy(response.message, "Registration failed");
            }
            send_message(clients[client_idx].socket, &response);
            break;
        }
        
        case MSG_LOGIN: {
            int user_id = login_user(msg->username, msg->password);
            response.type = MSG_RESPONSE;
            response.user_id = user_id;
            if (user_id > 0) {
                pthread_mutex_lock(&clients_mutex);
                clients[client_idx].user_id = user_id;
                strcpy(clients[client_idx].username, msg->username);
                pthread_mutex_unlock(&clients_mutex);
                strcpy(response.message, "Login successful");
            } else {
                strcpy(response.message, "Login failed");
            }
            send_message(clients[client_idx].socket, &response);
            break;
        }
        
        case MSG_SEND_FRIEND_REQUEST: {
            int target_id = get_user_id(msg->username);
            if (target_id > 0) {
                send_friend_request(clients[client_idx].user_id, target_id);
                
                // Notify target user if online
                int target_idx = find_client_by_id(target_id);
                if (target_idx >= 0) {
                    response.type = MSG_SEND_FRIEND_REQUEST;
                    response.user_id = clients[client_idx].user_id;
                    strcpy(response.username, clients[client_idx].username);
                    send_message(clients[target_idx].socket, &response);
                }
                
                strcpy(response.message, "Friend request sent");
            } else {
                strcpy(response.message, "User not found");
            }
            response.type = MSG_RESPONSE;
            send_message(clients[client_idx].socket, &response);
            break;
        }
        
        case MSG_ACCEPT_FRIEND_REQUEST: {
            accept_friend_request(clients[client_idx].user_id, msg->target_id);
            response.type = MSG_RESPONSE;
            strcpy(response.message, "Friend request accepted");
            send_message(clients[client_idx].socket, &response);
            break;
        }
        
        case MSG_INVITE_TO_ROOM: {
            int friend_id = msg->target_id;
            if (!is_friend(clients[client_idx].user_id, friend_id)) {
                response.type = MSG_ERROR;
                strcpy(response.message, "Not friends");
                send_message(clients[client_idx].socket, &response);
                break;
            }
            
            int friend_idx = find_client_by_id(friend_id);
            if (friend_idx < 0) {
                response.type = MSG_ERROR;
                strcpy(response.message, "Friend not online");
                send_message(clients[client_idx].socket, &response);
                break;
            }
            
            // Send invitation
            response.type = MSG_INVITE_TO_ROOM;
            response.user_id = clients[client_idx].user_id;
            strcpy(response.username, clients[client_idx].username);
            send_message(clients[friend_idx].socket, &response);
            break;
        }
        
        case MSG_JOIN_ROOM: {
            int host_id = msg->target_id;
            int host_idx = find_client_by_id(host_id);
            
            if (host_idx < 0) {
                response.type = MSG_ERROR;
                strcpy(response.message, "Host not found");
                send_message(clients[client_idx].socket, &response);
                break;
            }
            
            // Create game room
            pthread_mutex_lock(&rooms_mutex);
            GameRoom* room = NULL;
            for (int i = 0; i < MAX_ROOMS; i++) {
                if (!rooms[i].active) {
                    room = &rooms[i];
                    break;
                }
            }
            
            if (room) {
                init_game_room(room, next_room_id++, host_id, 
                             clients[client_idx].user_id,
                             clients[host_idx].socket,
                             clients[client_idx].socket);
                
                pthread_mutex_unlock(&rooms_mutex);
                
                // Notify both players
                response.type = MSG_RESPONSE;
                response.room_id = room->room_id;
                strcpy(response.message, "Game starting");
                send_message(clients[host_idx].socket, &response);
                send_message(clients[client_idx].socket, &response);
                
                // Start game loop in new thread
                pthread_t game_thread;
                pthread_create(&game_thread, NULL, game_loop, room);
                pthread_detach(game_thread);
            } else {
                pthread_mutex_unlock(&rooms_mutex);
                response.type = MSG_ERROR;
                strcpy(response.message, "No room available");
                send_message(clients[client_idx].socket, &response);
            }
            break;
        }
        
        case MSG_GAME_MOVE: {
            GameRoom* room = find_room_by_players(clients[client_idx].user_id, msg->target_id);
            if (room) {
                pthread_mutex_lock(&room->lock);
                move_tank(&room->state, clients[client_idx].user_id, msg->data[0]);
                pthread_mutex_unlock(&room->lock);
            }
            break;
        }
        
        case MSG_GAME_SHOOT: {
            GameRoom* room = find_room_by_players(clients[client_idx].user_id, msg->target_id);
            if (room) {
                pthread_mutex_lock(&room->lock);
                shoot_bullet(&room->state, clients[client_idx].user_id);
                pthread_mutex_unlock(&room->lock);
            }
            break;
        }
        
        case MSG_VIEW_HISTORY: {
            char history[2048];
            get_user_history(clients[client_idx].user_id, history, sizeof(history));
            response.type = MSG_RESPONSE;
            strncpy(response.message, history, sizeof(response.message) - 1);
            send_message(clients[client_idx].socket, &response);
            break;
        }
        
        case MSG_LOGOUT: {
            pthread_mutex_lock(&clients_mutex);
            clients[client_idx].active = 0;
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
        default:{
    // ignore or log
            break;}
    }
}

void* client_handler(void* arg) {
    int client_idx = *(int*)arg;
    free(arg);
    
    Message msg;
    
    while (clients[client_idx].active) {
        int bytes = recv(clients[client_idx].socket, &msg, sizeof(Message), 0);
        if (bytes <= 0) {
            break;
        }
        
        handle_message(client_idx, &msg);
    }
    
    pthread_mutex_lock(&clients_mutex);
    close(clients[client_idx].socket);
    clients[client_idx].active = 0;
    pthread_mutex_unlock(&clients_mutex);
    
    printf("Client disconnected\n");
    return NULL;
}

int main() {
    if (init_database() < 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        return 1;
    }
    
    printf("Server listening on port %d\n", PORT);
    
    memset(clients, 0, sizeof(clients));
    memset(rooms, 0, sizeof(rooms));
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("New client connected\n");
        
        pthread_mutex_lock(&clients_mutex);
        int idx = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                idx = i;
                clients[i].socket = client_socket;
                clients[i].active = 1;
                clients[i].user_id = -1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        
        if (idx < 0) {
            printf("Max clients reached\n");
            close(client_socket);
            continue;
        }
        
        int* arg = malloc(sizeof(int));
        *arg = idx;
        pthread_t thread;
        pthread_create(&thread, NULL, client_handler, arg);
        pthread_detach(thread);
    }
    
    close(server_fd);
    close_database();
    return 0;
}