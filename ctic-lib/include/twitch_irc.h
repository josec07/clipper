#pragma once

#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <cstdlib>

namespace ctic {

class TwitchIRC {
private:
    int sockfd_ = -1;
    std::string channel_;
    std::string buffer_;
    int message_count_ = 0;
    bool connected_ = false;
    
    void send_raw(const std::string& msg) {
        std::string full_msg = msg + "\r\n";
        ::send(sockfd_, full_msg.c_str(), full_msg.length(), 0);
    }
    
    std::string receive_line() {
        while (true) {
            size_t pos = buffer_.find("\r\n");
            if (pos != std::string::npos) {
                std::string line = buffer_.substr(0, pos);
                buffer_.erase(0, pos + 2);
                return line;
            }
            
            char temp[4096];
            ssize_t received = recv(sockfd_, temp, sizeof(temp) - 1, 0);
            if (received <= 0) {
                connected_ = false;
                return "";
            }
            temp[received] = '\0';
            buffer_ += temp;
        }
    }
    
public:
    TwitchIRC() = default;
    ~TwitchIRC() { disconnect(); }
    
    bool connect(const std::string& channel) {
        channel_ = channel;
        
        std::cout << "[IRC] Resolving irc.chat.twitch.tv..." << std::endl;
        
        struct hostent* server = gethostbyname("irc.chat.twitch.tv");
        if (!server) {
            std::cerr << "[IRC] ERROR: Failed to resolve" << std::endl;
            return false;
        }
        
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            std::cerr << "[IRC] ERROR: Failed to create socket" << std::endl;
            return false;
        }
        
        struct sockaddr_in serv_addr;
        std::memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        serv_addr.sin_port = htons(6667);
        
        std::cout << "[IRC] Connecting to port 6667..." << std::endl;
        
        if (::connect(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "[IRC] ERROR: Connection failed" << std::endl;
            return false;
        }
        
        std::cout << "[IRC] Requesting capabilities..." << std::endl;
        send_raw("CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership");
        
        std::string nick = "justinfan" + std::to_string(10000 + (std::rand() % 90000));
        std::cout << "[IRC] Authenticating as " << nick << "..." << std::endl;
        
        send_raw("PASS oauth:blah");
        send_raw("NICK " + nick);
        send_raw("USER " + nick + " 0 * :" + nick);
        
        auto start = std::chrono::steady_clock::now();
        bool got_welcome = false;
        
        while (!got_welcome) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed > 10) {
                std::cerr << "[IRC] ERROR: Authentication timeout" << std::endl;
                return false;
            }
            
            std::string line = receive_line();
            if (line.empty()) {
                std::cerr << "[IRC] ERROR: Connection closed during auth" << std::endl;
                return false;
            }
            
            if (line.find("PING") == 0) {
                std::string pong = "PONG" + line.substr(4);
                send_raw(pong);
                std::cout << "[IRC] PING -> PONG" << std::endl;
                continue;
            }
            
            if (line.find("001") != std::string::npos) {
                got_welcome = true;
                std::cout << "[IRC] Authentication successful" << std::endl;
            }
            
            if (line.find("Login authentication failed") != std::string::npos) {
                std::cerr << "[IRC] ERROR: Auth failed" << std::endl;
                return false;
            }
        }
        
        std::cout << "[IRC] Joining #" << channel << "..." << std::endl;
        send_raw("JOIN #" + channel);
        
        start = std::chrono::steady_clock::now();
        bool joined = false;
        
        while (!joined) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed > 5) {
                std::cerr << "[IRC] ERROR: Join timeout" << std::endl;
                return false;
            }
            
            std::string line = receive_line();
            if (line.empty()) {
                std::cerr << "[IRC] ERROR: Connection closed during join" << std::endl;
                return false;
            }
            
            if (line.find("PING") == 0) {
                std::string pong = "PONG" + line.substr(4);
                send_raw(pong);
                continue;
            }
            
            std::string expected_join = "JOIN #" + channel;
            if (line.find(expected_join) != std::string::npos) {
                joined = true;
                std::cout << "[IRC] Joined #" << channel << std::endl;
            }
            
            if (line.find("353") != std::string::npos) {
                std::cout << "[IRC] Received user list" << std::endl;
            }
            if (line.find("366") != std::string::npos) {
                std::cout << "[IRC] End of user list" << std::endl;
            }
        }
        
        connected_ = true;
        return true;
    }
    
    std::string read_line() {
        std::string line = receive_line();
        if (line.empty()) return "";
        
        if (line.find("PING") == 0) {
            std::string pong = "PONG" + line.substr(4);
            send_raw(pong);
            return read_line();
        }
        
        message_count_++;
        return line;
    }
    
    bool is_connected() const { return connected_; }
    int message_count() const { return message_count_; }
    const std::string& channel() const { return channel_; }
    
    void disconnect() {
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
        connected_ = false;
    }
};

}
