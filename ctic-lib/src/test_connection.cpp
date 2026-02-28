#include <iostream>
#include <string>
#include <csignal>
#include <atomic>

#include "twitch_url.h"
#include "twitch_irc.h"

std::atomic<bool> running(true);

void signal_handler(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <twitch_url_or_channel>" << std::endl;
        std::cerr << "Example: " << argv[0] << " https://twitch.tv/theburntpeanut" << std::endl;
        std::cerr << "         " << argv[0] << " theburntpeanut" << std::endl;
        return 1;
    }
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::string input = argv[1];
    std::string channel;
    
    if (ctic::is_valid_twitch_url(input)) {
        channel = ctic::extract_channel_from_url(input);
        std::cout << "[URL] Parsed channel: " << channel << std::endl;
    } else {
        channel = ctic::normalize_channel(input);
        std::cout << "[Channel] Using: " << channel << std::endl;
    }
    
    ctic::TwitchIRC irc;
    
    if (!irc.connect(channel)) {
        std::cerr << "Failed to connect to #" << channel << std::endl;
        return 1;
    }
    
    std::cout << "\n[Connected] Listening to chat. Press Ctrl+C to stop.\n" << std::endl;
    
    while (running && irc.is_connected()) {
        std::string line = irc.read_line();
        if (line.empty()) continue;
        
        size_t privmsg_pos = line.find("PRIVMSG #" + channel + " :");
        if (privmsg_pos == std::string::npos) continue;
        
        size_t user_start = line.find(":");
        size_t user_end = line.find("!", user_start);
        std::string username = line.substr(user_start + 1, user_end - user_start - 1);
        
        size_t content_start = line.find(" :", privmsg_pos);
        std::string content = line.substr(content_start + 2);
        
        std::cout << username << ": " << content << std::endl;
    }
    
    std::cout << "\n[Shutdown] Received " << irc.message_count() << " messages" << std::endl;
    irc.disconnect();
    
    return 0;
}
