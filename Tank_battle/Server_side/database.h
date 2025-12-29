#ifndef DATABASE_H
#define DATABASE_H

#include <mysql/mysql.h>
#include "protocol.h"

// Database functions
int init_database();
void close_database();

// User functions
int register_user(const char* username, const char* email, const char* password);
int login_user(const char* username, const char* password);
int get_user_id(const char* username);

// Friendship functions
int send_friend_request(int user_id, int friend_id);
int accept_friend_request(int user_id, int friend_id);
int is_friend(int user_id, int friend_id);
int get_friends_list(int user_id, int* friends, int max_count);

// History functions
int save_game_history(int user_id, int competitor_id, const char* result);
int get_user_history(int user_id, char* history_buffer, int buffer_size);

#endif