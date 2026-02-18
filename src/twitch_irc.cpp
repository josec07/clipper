#include "twitch_irc.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <regex>

namespace chatclipper {

TwitchIRC::TwitchIRC() : sockfd_(-1), connected_(false) {}

TwitchIRC::~TwitchIRC() {
    disconnect();
}

bool TwitchIRC::connect(const std::string& host, int port) {
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string portStr = std::to_string(port);
    int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (status != 0) {
        lastError_ = "getaddrinfo: " + std::string(gai_strerror(status));
        return false;
    }
    
    for (rp = res; rp != nullptr; rp = rp->ai_next) {
        sockfd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd_ == -1) continue;
        
        if (::connect(sockfd_, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(sockfd_);
        sockfd_ = -1;
    }
    
    freeaddrinfo(res);
    
    if (sockfd_ == -1) {
        lastError_ = "Failed to connect to " + host;
        return false;
    }
    
    connected_ = true;
    return true;
}

void TwitchIRC::disconnect() {
    if (sockfd_ != -1) {
        close(sockfd_);
        sockfd_ = -1;
    }
    connected_ = false;
}

bool TwitchIRC::isConnected() const {
    return connected_;
}

bool TwitchIRC::sendCommand(const std::string& command) {
    std::string msg = command + "\r\n";
    ssize_t sent = ::send(sockfd_, msg.c_str(), msg.length(), 0);
    return sent == static_cast<ssize_t>(msg.length());
}

void TwitchIRC::sendRaw(const std::string& message) {
    sendCommand(message);
}

std::string TwitchIRC::receiveLine(int timeoutMs) {
    while (true) {
        size_t pos = buffer_.find("\r\n");
        if (pos != std::string::npos) {
            std::string line = buffer_.substr(0, pos);
            buffer_.erase(0, pos + 2);
            return line;
        }
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd_, &readfds);
        
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        
        int sel = select(sockfd_ + 1, &readfds, nullptr, nullptr, &tv);
        if (sel <= 0) {
            return "";
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

bool TwitchIRC::waitForResponse(const std::string& expected, int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= timeoutMs) {
            return false;
        }
        
        int remaining = timeoutMs - static_cast<int>(elapsed);
        std::string line = receiveLine(remaining);
        if (line.empty()) return false;
        
        if (line.find(expected) != std::string::npos) {
            return true;
        }
        
        if (line.find("Login authentication failed") != std::string::npos) {
            lastError_ = "Authentication failed";
            return false;
        }
    }
}

bool TwitchIRC::authenticate(const std::string& oauth, const std::string& username) {
    sendCommand("CAP REQ :twitch.tv/tags twitch.tv/commands");
    
    if (!waitForResponse("ACK", 5000)) {
        lastError_ = "Failed to negotiate capabilities";
        return false;
    }
    
    sendCommand("PASS " + oauth);
    sendCommand("NICK " + username);
    
    if (!waitForResponse("001", 10000)) {
        if (lastError_.empty()) {
            lastError_ = "Authentication failed - no 001 response";
        }
        return false;
    }
    
    return true;
}

bool TwitchIRC::joinChannel(const std::string& channel) {
    std::string chan = channel;
    if (chan[0] != '#') {
        chan = "#" + chan;
    }
    
    sendCommand("JOIN " + chan);
    return waitForResponse("JOIN", 5000);
}

std::string TwitchIRC::extractTagValue(const std::string& tags, const std::string& key) {
    std::string search = key + "=";
    size_t start = tags.find(search);
    if (start == std::string::npos) return "";
    
    start += search.length();
    size_t end = tags.find(';', start);
    if (end == std::string::npos) {
        end = tags.length();
    }
    return tags.substr(start, end - start);
}

IRCMessage TwitchIRC::parsePRIVMSG(const std::string& line) {
    IRCMessage msg;
    msg.timestamp = std::chrono::milliseconds(0);
    
    if (line.empty() || line[0] != '@') {
        return msg;
    }
    
    size_t tagsEnd = line.find(" :");
    if (tagsEnd == std::string::npos) return msg;
    
    std::string tags = line.substr(1, tagsEnd - 1);
    
    std::string tsStr = extractTagValue(tags, "tmi-sent-ts");
    if (!tsStr.empty()) {
        try {
            int64_t ts = std::stoll(tsStr);
            msg.timestamp = std::chrono::milliseconds(ts);
        } catch (...) {}
    }
    
    if (msg.timestamp.count() == 0) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        msg.timestamp = now;
    }
    
    msg.username = extractTagValue(tags, "display-name");
    
    size_t privmsgPos = line.find("PRIVMSG");
    if (privmsgPos != std::string::npos) {
        size_t chanStart = line.find('#', privmsgPos);
        if (chanStart != std::string::npos) {
            size_t chanEnd = line.find(' ', chanStart);
            if (chanEnd != std::string::npos) {
                msg.channel = line.substr(chanStart, chanEnd - chanStart);
            }
        }
    }
    
    size_t msgStart = line.find(" :", tagsEnd + 2);
    if (msgStart != std::string::npos) {
        msgStart += 2;
    } else {
        msgStart = line.rfind(" :");
        if (msgStart != std::string::npos) {
            msgStart += 2;
        }
    }
    
    if (msgStart != std::string::npos && msgStart < line.length()) {
        msg.content = line.substr(msgStart);
    }
    
    if (msg.username.empty()) {
        size_t sourceStart = line.find(" :");
        if (sourceStart != std::string::npos) {
            sourceStart += 2;
            size_t nickEnd = line.find('!', sourceStart);
            if (nickEnd != std::string::npos) {
                msg.username = line.substr(sourceStart, nickEnd - sourceStart);
            }
        }
    }
    
    return msg;
}

std::optional<IRCMessage> TwitchIRC::readMessage(int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= timeoutMs) {
            lastError_ = "Timeout waiting for message";
            return std::nullopt;
        }
        
        int remaining = timeoutMs - static_cast<int>(elapsed);
        std::string line = receiveLine(remaining);
        if (line.empty()) {
            return std::nullopt;
        }
        
        if (line.substr(0, 4) == "PING") {
            std::string pong = "PONG" + line.substr(4);
            sendCommand(pong);
            continue;
        }
        
        if (line.find("PRIVMSG") != std::string::npos) {
            return parsePRIVMSG(line);
        }
    }
}

} // namespace chatclipper

void printUsage(const char* program) {
    std::cerr << "Usage: " << program << " --channel <channel> [options]\n"
              << "Options:\n"
              << "  --channel <name>   Channel to join (required)\n"
              << "  --oauth <token>    OAuth token (or set TWITCH_OAUTH env)\n"
              << "  --username <name>  Username (or set TWITCH_USERNAME env)\n"
              << "  --continuous       Keep reading messages (default: single message)\n"
              << "  --timeout <ms>     Timeout in milliseconds (default: 30000)\n"
              << "  --help             Show this help\n";
}

int main(int argc, char* argv[]) {
    std::string channel, oauth, username;
    bool continuous = false;
    int timeout = 30000;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--channel" && i + 1 < argc) {
            channel = argv[++i];
        } else if (arg == "--oauth" && i + 1 < argc) {
            oauth = argv[++i];
        } else if (arg == "--username" && i + 1 < argc) {
            username = argv[++i];
        } else if (arg == "--continuous") {
            continuous = true;
        } else if (arg == "--timeout" && i + 1 < argc) {
            timeout = std::stoi(argv[++i]);
        }
    }
    
    if (channel.empty()) {
        std::cerr << "Error: --channel is required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    const char* envOAuth = std::getenv("TWITCH_OAUTH");
    const char* envUser = std::getenv("TWITCH_USERNAME");
    
    if (oauth.empty() && envOAuth) oauth = envOAuth;
    if (username.empty() && envUser) username = envUser;
    
    if (oauth.empty()) {
        std::cerr << "Error: OAuth token required (--oauth or TWITCH_OAUTH env)\n";
        return 1;
    }
    if (username.empty()) {
        std::cerr << "Error: Username required (--username or TWITCH_USERNAME env)\n";
        return 1;
    }
    
    chatclipper::TwitchIRC irc;
    
    if (!irc.connect()) {
        std::cerr << "Connection failed: " << irc.getLastError() << "\n";
        return 1;
    }
    
    if (!irc.authenticate(oauth, username)) {
        std::cerr << "Authentication failed: " << irc.getLastError() << "\n";
        return 1;
    }
    
    if (!irc.joinChannel(channel)) {
        std::cerr << "Failed to join channel: " << irc.getLastError() << "\n";
        return 1;
    }
    
    do {
        auto msg = irc.readMessage(timeout);
        if (!msg) {
            if (!continuous) {
                std::cerr << "No message received: " << irc.getLastError() << "\n";
                return 1;
            }
            break;
        }
        
        std::cout << msg->timestamp.count() << "|" << msg->username << "|" << msg->content << "\n";
        std::cout.flush();
    } while (continuous);
    
    return 0;
}
