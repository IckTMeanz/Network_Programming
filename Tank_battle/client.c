#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

// Hàm đọc input từ bàn phím và gửi đi
void send_login_request(int sock) {
    char email[100];
    char password[100];
    char send_buffer[BUFFER_SIZE];
    
    printf("\n--- Chức năng Đăng nhập ---\n");
    printf("Email: ");
    if (fgets(email, sizeof(email), stdin) == NULL) return;
    email[strcspn(email, "\n")] = 0; // Loại bỏ ký tự xuống dòng
    
    printf("Password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) return;
    password[strcspn(password, "\n")] = 0; // Loại bỏ ký tự xuống dòng
    
    // Tạo gói tin theo định dạng: LOGIN|email|password
    snprintf(send_buffer, BUFFER_SIZE, "LOGIN|%s|%s", email, password);
    
    // Gửi gói tin đăng nhập
    if (send(sock, send_buffer, strlen(send_buffer), 0) == -1) {
        perror("Lỗi gửi dữ liệu");
    } else {
        printf("[SENT] Yêu cầu: %s\n", send_buffer);
    }
}

// Hàm nhận phản hồi từ Server
void receive_response(int sock) {
    char recv_buffer[BUFFER_SIZE];
    int bytes_received;
    
    bytes_received = recv(sock, recv_buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received > 0) {
        recv_buffer[bytes_received] = '\0';
        printf("\n<<< SERVER RESPONSE >>>\n");
        
        // Phân tích phản hồi
        if (strncmp(recv_buffer, "LOGIN_SUCCESS", 13) == 0) {
            printf(" Đăng nhập THÀNH CÔNG!\n");
        } else if (strncmp(recv_buffer, "LOGIN_FAILED", 12) == 0) {
            printf("Đăng nhập THẤT BẠI. Kiểm tra lại email hoặc mật khẩu.\n");
        } else {
            printf(" Phản hồi khác: %s\n", recv_buffer);
        }
        printf("-------------------------\n");
    } else if (bytes_received == 0) {
        printf("Server đã đóng kết nối.\n");
        exit(EXIT_SUCCESS);
    } else {
        perror(" Lỗi nhận dữ liệu");
    }
}

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *server_ip;
    int port;

    // 1. Kiểm tra tham số dòng lệnh
    if (argc != 3) {
        fprintf(stderr, "Cách dùng: %s <IPAddress> <PortNumber>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    server_ip = argv[1];
    port = atoi(argv[2]);
    
    // 2. Tạo socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(" Lỗi tạo Socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    // Chuyển đổi địa chỉ IP
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror(" Địa chỉ IP không hợp lệ");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // 3. Kết nối đến Server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror(" Kết nối Server thất bại");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf(" Đã kết nối tới server %s:%d.\n", server_ip, port);
    
    // 4. Gửi và nhận liên tục
    while (1) {
        send_login_request(sock);
        receive_response(sock);
        
        // Sau khi kiểm tra đăng nhập xong, bạn có thể thêm logic để thoát
        printf("\nGõ 'exit' để thoát hoặc Enter để thử lại: ");
        char exit_command[10];
        if (fgets(exit_command, sizeof(exit_command), stdin) != NULL) {
            if (strncmp(exit_command, "exit", 4) == 0) {
                break;
            }
        }
    }

    // 5. Đóng socket
    close(sock);
    return 0;
}