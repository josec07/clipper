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
namespace providers {

class TwitchIRC {
private:
    int sockfd_ = -1;
    std::string channel_;
    std::string buffer_;
    int message_count_ = 0;
    bool connected_ = false;
    
    void send_raw(const std::string& msg);
    std::string receive_line();
    
public:
    TwitchIRC() = default;
    ~TwitchIRC() { disconnect(); }
    
    bool connect(const std::string& channel);
    std::string read_line();
    bool parse_message(const std::string& raw, std::string& username, std::string& content);
    
    bool is_connected() const { return connected_; }
    int message_count() const { return message_count_; }
    const std::string& channel() const { return channel_; }
    
    void disconnect();
    bool test_connection(const std::string& channel, int timeout_seconds = 30);
};

}
}
