#pragma once

#include <string>
#include <optional>
#include <chrono>

namespace chatclipper {

struct IRCMessage {
    std::chrono::milliseconds timestamp;
    std::string username;
    std::string content;
    std::string channel;
};

class TwitchIRC {
public:
    TwitchIRC();
    ~TwitchIRC();
    
    bool connect(const std::string& host = "irc.chat.twitch.tv", int port = 6667);
    void disconnect();
    bool isConnected() const;
    
    bool authenticate(const std::string& oauth, const std::string& username);
    bool authenticateAnonymous();
    bool joinChannel(const std::string& channel);
    
    std::optional<IRCMessage> readMessage(int timeoutMs = 30000);
    void sendRaw(const std::string& message);
    
    const std::string& getLastError() const { return lastError_; }
    
private:
    int sockfd_;
    bool connected_;
    std::string lastError_;
    std::string buffer_;
    
    bool sendCommand(const std::string& command);
    std::string receiveLine(int timeoutMs);
    bool waitForResponse(const std::string& expected, int timeoutMs = 10000);
    
    IRCMessage parsePRIVMSG(const std::string& line);
    std::string extractTagValue(const std::string& tags, const std::string& key);
};

} // namespace chatclipper
