-- Tank Battle Database Setup Script for MySQL
-- Chạy script này với quyền root: mysql -u root -p < setup_mysql.sql

-- Tạo database
CREATE DATABASE IF NOT EXISTS tank_battle3;
USE tank_battle3;

-- Tạo bảng users
CREATE TABLE IF NOT EXISTS users (
    user_id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password VARCHAR(100) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Tạo bảng history
CREATE TABLE IF NOT EXISTS history (
    history_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    competitor_id INT NOT NULL,
    result VARCHAR(20) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (competitor_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user (user_id),
    INDEX idx_competitor (competitor_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Tạo bảng friendship
CREATE TABLE IF NOT EXISTS friendship (
    user_id INT NOT NULL,
    friend_id INT NOT NULL,
    status VARCHAR(20) DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, friend_id),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Thêm dữ liệu mẫu (optional)
INSERT INTO users (username, email, password) VALUES 
    ('player1', 'player1@test.com', '123456'),
    ('player2', 'player2@test.com', '123456'),
    ('player3', 'player3@test.com', '123456')
ON DUPLICATE KEY UPDATE username=username;

SHOW TABLES;
SELECT 'Database setup completed!' as Status;