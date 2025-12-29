#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "database.h"

static MYSQL* conn = NULL;

// Database configuration
#define DB_HOST "localhost"
#define DB_USER "javauser"
#define DB_PASS "yourpassword"  // Thay đổi password của bạn
#define DB_NAME "tank_battle3"
#define DB_PORT 3306

int init_database() {
    conn = mysql_init(NULL);
    
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return -1;
    }
    
    // Connect to MySQL server
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, NULL, DB_PORT, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }
    
    // Create database if not exists
    char query[1024];
    snprintf(query, sizeof(query), "CREATE DATABASE IF NOT EXISTS %s", DB_NAME);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "CREATE DATABASE failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }
    
    // Select database
    if (mysql_select_db(conn, DB_NAME)) {
        fprintf(stderr, "mysql_select_db() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }
    
    // Create tables
    const char* create_users = 
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id INT AUTO_INCREMENT PRIMARY KEY,"
        "username VARCHAR(50) UNIQUE NOT NULL,"
        "email VARCHAR(100) UNIQUE NOT NULL,"
        "password VARCHAR(100) NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    const char* create_history = 
        "CREATE TABLE IF NOT EXISTS history ("
        "history_id INT AUTO_INCREMENT PRIMARY KEY,"
        "user_id INT NOT NULL,"
        "competitor_id INT NOT NULL,"
        "result VARCHAR(20) NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,"
        "FOREIGN KEY (competitor_id) REFERENCES users(user_id) ON DELETE CASCADE,"
        "INDEX idx_user (user_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    const char* create_friendship = 
        "CREATE TABLE IF NOT EXISTS friendship ("
        "user_id INT NOT NULL,"
        "friend_id INT NOT NULL,"
        "status VARCHAR(20) DEFAULT 'pending',"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (user_id, friend_id),"
        "FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,"
        "FOREIGN KEY (friend_id) REFERENCES users(user_id) ON DELETE CASCADE"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    if (mysql_query(conn, create_users)) {
        fprintf(stderr, "CREATE TABLE users failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_query(conn, create_history)) {
        fprintf(stderr, "CREATE TABLE history failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_query(conn, create_friendship)) {
        fprintf(stderr, "CREATE TABLE friendship failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Database initialized successfully\n");
    return 0;
}

void close_database() {
    if (conn) {
        mysql_close(conn);
        conn = NULL;
    }
}

int register_user(const char* username, const char* email, const char* password) {
    char query[512];
    char esc_username[101], esc_email[201], esc_password[201];
    
    mysql_real_escape_string(conn, esc_username, username, strlen(username));
    mysql_real_escape_string(conn, esc_email, email, strlen(email));
    mysql_real_escape_string(conn, esc_password, password, strlen(password));
    
    snprintf(query, sizeof(query),
             "INSERT INTO users (username, email, password) VALUES ('%s', '%s', '%s')",
             esc_username, esc_email, esc_password);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Register failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    return mysql_insert_id(conn);
}

int login_user(const char* username, const char* password) {
    char query[512];
    char esc_username[101], esc_password[201];
    
    mysql_real_escape_string(conn, esc_username, username, strlen(username));
    mysql_real_escape_string(conn, esc_password, password, strlen(password));
    
    snprintf(query, sizeof(query),
             "SELECT user_id FROM users WHERE username='%s' AND password='%s'",
             esc_username, esc_password);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Login query failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        fprintf(stderr, "mysql_store_result() failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    int user_id = -1;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        user_id = atoi(row[0]);
    }
    
    mysql_free_result(result);
    return user_id;
}

int get_user_id(const char* username) {
    char query[256];
    char esc_username[101];
    
    mysql_real_escape_string(conn, esc_username, username, strlen(username));
    snprintf(query, sizeof(query),
             "SELECT user_id FROM users WHERE username='%s'", esc_username);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Get user_id query failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return -1;
    }
    
    int user_id = -1;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        user_id = atoi(row[0]);
    }
    
    mysql_free_result(result);
    return user_id;
}

int send_friend_request(int user_id, int friend_id) {
    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO friendship (user_id, friend_id, status) "
             "VALUES (%d, %d, 'pending') "
             "ON DUPLICATE KEY UPDATE status='pending'",
             user_id, friend_id);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Send friend request failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    return 0;
}

int accept_friend_request(int user_id, int friend_id) {
    // Update the existing request
    char query1[256];
    snprintf(query1, sizeof(query1),
             "UPDATE friendship SET status='accepted' "
             "WHERE user_id=%d AND friend_id=%d",
             friend_id, user_id);
    
    if (mysql_query(conn, query1)) {
        fprintf(stderr, "Accept friend request failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    // Create reciprocal friendship
    char query2[256];
    snprintf(query2, sizeof(query2),
             "INSERT IGNORE INTO friendship (user_id, friend_id, status) "
             "VALUES (%d, %d, 'accepted')",
             user_id, friend_id);
    
    if (mysql_query(conn, query2)) {
        fprintf(stderr, "Create reciprocal friendship failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    return 0;
}

int is_friend(int user_id, int friend_id) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT COUNT(*) FROM friendship WHERE "
             "((user_id=%d AND friend_id=%d) OR (user_id=%d AND friend_id=%d)) "
             "AND status='accepted'",
             user_id, friend_id, friend_id, user_id);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Is friend query failed: %s\n", mysql_error(conn));
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return 0;
    }
    
    int count = 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        count = atoi(row[0]);
    }
    
    mysql_free_result(result);
    return count > 0;
}

int get_friends_list(int user_id, int* friends, int max_count) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT DISTINCT CASE "
             "WHEN user_id=%d THEN friend_id "
             "ELSE user_id END as friend "
             "FROM friendship WHERE (user_id=%d OR friend_id=%d) "
             "AND status='accepted' LIMIT %d",
             user_id, user_id, user_id, max_count);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Get friends list failed: %s\n", mysql_error(conn));
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return 0;
    }
    
    int count = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && count < max_count) {
        friends[count++] = atoi(row[0]);
    }
    
    mysql_free_result(result);
    return count;
}

int save_game_history(int user_id, int competitor_id, const char* result) {
    char query[256];
    char esc_result[41];
    
    mysql_real_escape_string(conn, esc_result, result, strlen(result));
    
    snprintf(query, sizeof(query),
             "INSERT INTO history (user_id, competitor_id, result) "
             "VALUES (%d, %d, '%s')",
             user_id, competitor_id, esc_result);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Save history failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    return 0;
}

int get_user_history(int user_id, char* history_buffer, int buffer_size) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT h.result, u.username, h.created_at "
             "FROM history h JOIN users u ON h.competitor_id = u.user_id "
             "WHERE h.user_id=%d ORDER BY h.created_at DESC LIMIT 10",
             user_id);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Get history failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return -1;
    }
    
    int offset = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && offset < buffer_size - 100) {
        offset += snprintf(history_buffer + offset, buffer_size - offset,
                          "%s vs %s - %s\n", row[0], row[1], row[2]);
    }
    
    mysql_free_result(result);
    return 0;
}