#include "../../include/providers/twitch_irc.h"
#include <iostream>

namespace ctic {
namespace providers {

void TwitchIRC::send_raw(const std::string& msg) {
    std::string full_msg = msg + "\r\n";
    ::send(sockfd_, full_msg.c_str(), full_msg.length(), 0);
}

std::string TwitchIRC::receive_line() {
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

bool TwitchIRC::connect(const std::string& channel) {
    channel_ = channel;
    
    struct hostent* server = gethostbyname("irc.chat.twitch.tv");
    if (!server) {
        return false;
    }
    
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        return false;
    }
    
    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(6667);
    
    if (::connect(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        return false;
    }
    
    send_raw("CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership");
    
    std::string nick = "justinfan" + std::to_string(10000 + (std::rand() % 90000));
    send_raw("PASS oauth:blah");
    send_raw("NICK " + nick);
    send_raw("USER " + nick + " 0 * :" + nick);
    
    auto start = std::chrono::steady_clock::now();
    bool got_welcome = false;
    
    while (!got_welcome) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (elapsed > 10) {
            return false;
        }
        
        std::string line = receive_line();
        if (line.empty()) {
            return false;
        }
        
        if (line.find("PING") == 0) {
            std::string pong = "PONG" + line.substr(4);
            send_raw(pong);
            continue;
        }
        
        if (line.find("001") != std::string::npos) {
            got_welcome = true;
        }
        
        if (line.find("Login authentication failed") != std::string::npos) {
            return false;
        }
    }
    
    send_raw("JOIN #" + channel);
    
    start = std::chrono::steady_clock::now();
    bool joined = false;
    
    while (!joined) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (elapsed > 5) {
            return false;
        }
        
        std::string line = receive_line();
        if (line.empty()) {
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
        }
    }
    
    connected_ = true;
    return true;
}

std::string TwitchIRC::read_line() {
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

bool TwitchIRC::parse_message(const std::string& raw, std::string& username, std::string& content) {
    size_t privmsg_pos = raw.find("PRIVMSG #" + channel_);
    if (privmsg_pos == std::string::npos) return false;
    
    size_t user_start = raw.find(":");
    if (user_start == std::string::npos) return false;
    
    if (raw[0] == '@') {
        size_t tag_end = raw.find(" :");
        if (tag_end != std::string::npos) {
            user_start = raw.find(":", tag_end);
            if (user_start == std::string::npos) return false;
        }
    }
    
    size_t user_end = raw.find("!", user_start);
    if (user_end == std::string::npos) return false;
    username = raw.substr(user_start + 1, user_end - user_start - 1);
    
    size_t content_start = raw.rfind(" :");
    if (content_start == std::string::npos || content_start < privmsg_pos) return false;
    content = raw.substr(content_start + 2);
    
    return true;
}

void TwitchIRC::disconnect() {
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
    connected_ = false;
}

bool TwitchIRC::test_connection(const std::string& channel, int timeout_seconds) {
    if (!connect(channel)) {
        return false;
    }
    
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count() < timeout_seconds) {
        std::string line = read_line();
        if (!line.empty()) {
            std::string username, content;
            if (parse_message(line, username, content)) {
                disconnect();
                return true;
            }
        }
    }
    
    disconnect();
    return true;
}

}
}
