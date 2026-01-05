#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

// Thư viện MySQL C Connector
#include <mysql/mysql.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT_MIN 1024

// --- THÔNG TIN KẾT NỐI MYSQL ---
#define DB_HOST "localhost"
#define DB_USER "javauser"  
#define DB_PASS "yourpassword"
#define DB_NAME "tank_battle" 

// Cấu trúc để truyền dữ liệu kết nối tới luồng xử lý
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_info_t;

// Hàm kết nối và kiểm tra MySQL
MYSQL *db_connect() {
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init failed: %s\n", mysql_error(conn));
        return NULL;
    }

    // Kết nối đến Database
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, " MySQL connection failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    printf("Successfully connected to MySQL Database.\n");
    return conn;
}

// Hàm giả định kiểm tra đăng nhập (sẽ được phát triển sau)
int check_login(MYSQL *conn, const char *email, const char *password) {
    char query[BUFFER_SIZE * 2];
    int result = 0;
    char escaped_email[256];
    char escaped_password[256];

    mysql_real_escape_string(conn, escaped_email, email, strlen(email));
    mysql_real_escape_string(conn, escaped_password, password, strlen(password));

    // Truy vấn: Đếm số lượng user có email và password khớp
    snprintf(query, sizeof(query),
             "SELECT COUNT(*) FROM users WHERE email = '%s' AND password = '%s'",
             escaped_email, escaped_password);

    printf("   [DB Query] Executing: %s\n", query);

    // Thực thi truy vấn
    if (mysql_query(conn, query)) {
        fprintf(stderr, "MySQL Query failed: %s\n", mysql_error(conn));
        return 0;
    }

    // Lấy kết quả
    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, " mysql_store_result failed: %s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_ROW row;
    if ((row = mysql_fetch_row(res))) {
        // Hàng đầu tiên (và duy nhất) chứa COUNT(*)
        int count = atoi(row[0]);
        if (count == 1) {
            result = 1; // Đăng nhập thành công
        }
    }

    mysql_free_result(res);
    return result;
}
// Hàm xử lý kết nối của mỗi Client
void *handle_client(void *arg) {
    client_info_t *info = (client_info_t *)arg;
    int client_socket = info->client_socket;
    char *client_ip = inet_ntoa(info->client_addr.sin_addr);
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    // 1. Kết nối MySQL (Mỗi luồng nên có kết nối riêng)
    MYSQL *conn = db_connect();
    if (conn == NULL) {
        send(client_socket, "SERVER_ERROR|DB_FAIL", 20, 0);
        close(client_socket);
        free(info);
        pthread_exit(NULL);
    }

    printf(" New connection from %s:%d (Thread ID: %lu)\n", 
           client_ip, ntohs(info->client_addr.sin_port), pthread_self());

    // 2. Vòng lặp nhận dữ liệu (ví dụ: lệnh đăng nhập)
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("   [Received from %s] %s\n", client_ip, buffer);
        
        // --- Xử lý Lệnh Đăng nhập ---
        if (strncmp(buffer, "LOGIN|", 6) == 0) {
            // Tạo bản sao của buffer để sử dụng strtok, vì strtok sửa đổi chuỗi gốc
            char *command_line = strdup(buffer); 
            
            char *token = strtok(command_line, "|"); // token = "LOGIN"
            char *email = strtok(NULL, "|");          // token = email
            char *password = strtok(NULL, "|");       // token = password
            
            if (email && password) {
                if (check_login(conn, email, password)) {
                    // **THÀNH CÔNG:** Sau này bạn cần tạo Session Token/ID ở đây
                    char *msg = "LOGIN_SUCCESS|";
                    send(client_socket, msg, strlen(msg), 0);
                    printf("   [AUTH] User %s logged in successfully.\n", email);
                } else {
                    char *msg = "LOGIN_FAILED|Invalid email or password.";
                    send(client_socket, msg, strlen(msg), 0);
                    printf("   [AUTH] Login failed for user %s.\n", email);
                }
            } else {
                 send(client_socket, "INVALID_FORMAT|Missing email or password.", 40, 0);
            }

            free(command_line); // Giải phóng bộ nhớ của strdup
        } else {
            // Lệnh khác
            send(client_socket, "UNKNOWN_COMMAND|Please login first.", 35, 0);
        }
        // --------------------------------------
    }

    // 3. Đóng kết nối
    if (bytes_received == 0) {
        printf(" Client %s disconnected.\n", client_ip);
    } else if (bytes_received == -1) {
        perror(" recv failed");
    }

    // Đóng kết nối MySQL của luồng này
    mysql_close(conn); 
    close(client_socket);
    free(info); 
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int port;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PortNumber>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    port = atoi(argv[1]);
    
    printf("Starting TCP Server on port %d...\n", port);

    // 1. Tạo socket và SO_REUSEADDR
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror(" socket failed");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // 2. Bind và Listen
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror(" bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror(" listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d. Waiting for connections...\n", port);

    // 3. Vòng lặp Accept và Tạo Luồng (Pthread)
    while (1) {
        client_info_t *info = (client_info_t *)malloc(sizeof(client_info_t));
        if (info == NULL) {
            perror("malloc failed");
            continue;
        }

        info->client_socket = accept(server_fd, (struct sockaddr *)&info->client_addr, (socklen_t *)&addrlen);
        if (info->client_socket < 0) {
            perror(" accept failed");
            free(info);
            continue;
        }
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)info) != 0) {
            perror(" could not create thread");
            close(info->client_socket);
            free(info);
        }
        // Tách luồng (Detaching)
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}