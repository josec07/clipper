#include "../../include/cli/commands.h"
#include "../../include/core/config.h"
#include "../../include/providers/twitch_url.h"
#include "../../include/providers/twitch_irc.h"
#include "../../include/core/detection.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace ctic {
namespace cli {

int cmd_add(const std::string& url_or_channel) {
    core::ConfigManager config_mgr;
    config_mgr.ensure_ctic_dir();
    
    std::string channel = providers::parse_url_or_channel(url_or_channel);
    if (channel.empty()) {
        std::cerr << "Error: Invalid Twitch URL or channel name" << std::endl;
        return 1;
    }
    
    if (config_mgr.creator_exists(channel)) {
        std::cout << "Creator '" << channel << "' already exists." << std::endl;
        return 0;
    }
    
    std::cout << "[IRC] Connecting to #" << channel << "..." << std::endl;
    
    providers::TwitchIRC irc;
    if (!irc.connect(channel)) {
        std::cerr << "Error: Failed to connect to #" << channel << std::endl;
        return 1;
    }
    
    std::cout << "[IRC] Connected. Testing chat (30s)..." << std::endl;
    
    int msg_count = 0;
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count() < 30) {
        std::string line = irc.read_line();
        if (!line.empty()) {
            std::string username, content;
            if (irc.parse_message(line, username, content)) {
                msg_count++;
                if (msg_count <= 5) {
                    std::cout << "  " << username << ": " << content << std::endl;
                }
            }
        }
    }
    
    std::cout << "[IRC] Received " << msg_count << " messages" << std::endl;
    irc.disconnect();
    
    core::CreatorConfig creator;
    creator.name = channel;
    creator.channel = channel;
    creator.twitch_url = "https://twitch.tv/" + channel;
    creator.enabled_tiers = {"high", "medium", "easy"};
    creator.detector_config_id = "default";
    creator.created_at = core::format_timestamp(std::chrono::system_clock::now());
    
    if (!config_mgr.save_creator(creator)) {
        std::cerr << "Error: Failed to save creator config" << std::endl;
        return 1;
    }
    
    std::cout << "Creator '" << channel << "' added successfully." << std::endl;
    std::cout << "Config: .ctic/creators/" << channel << ".json" << std::endl;
    
    return 0;
}

int cmd_list() {
    core::ConfigManager config_mgr;
    auto creators = config_mgr.list_creators();
    
    if (creators.empty()) {
        std::cout << "No creators configured." << std::endl;
        std::cout << "Run 'ctic add <url>' to add a creator." << std::endl;
        return 0;
    }
    
    std::cout << "Configured creators:" << std::endl;
    for (const auto& name : creators) {
        auto config = config_mgr.load_creator(name);
        std::cout << "  " << name << " - " << config.twitch_url << std::endl;
    }
    
    return 0;
}

int cmd_status(const std::string& name) {
    core::ConfigManager config_mgr;
    
    if (!name.empty()) {
        if (!config_mgr.creator_exists(name)) {
            std::cerr << "Creator '" << name << "' not found." << std::endl;
            return 1;
        }
        
        auto config = config_mgr.load_creator(name);
        
        std::cout << "Testing connection to #" << config.channel << "..." << std::endl;
        
        providers::TwitchIRC irc;
        bool connected = irc.connect(config.channel);
        
        if (connected) {
            std::cout << name << "  OK Connected" << std::endl;
            irc.disconnect();
        } else {
            std::cout << name << "  OFFLINE" << std::endl;
        }
        
        return connected ? 0 : 1;
    }
    
    auto creators = config_mgr.list_creators();
    if (creators.empty()) {
        std::cout << "No creators configured." << std::endl;
        return 0;
    }
    
    int online = 0;
    for (const auto& creator_name : creators) {
        auto config = config_mgr.load_creator(creator_name);
        
        providers::TwitchIRC irc;
        bool connected = irc.connect(config.channel);
        
        if (connected) {
            std::cout << creator_name << "  OK Connected" << std::endl;
            irc.disconnect();
            online++;
        } else {
            std::cout << creator_name << "  OFFLINE" << std::endl;
        }
    }
    
    std::cout << online << "/" << creators.size() << " creators online." << std::endl;
    return 0;
}

int cmd_run() {
    core::ConfigManager config_mgr;
    auto creators = config_mgr.list_creators();
    
    if (creators.empty()) {
        std::cout << "No creators configured." << std::endl;
        std::cout << "Run 'ctic add <url>' to add a creator." << std::endl;
        return 1;
    }
    
    std::cout << "Monitoring " << creators.size() << " creator(s)..." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl << std::endl;
    
    for (const auto& channel : creators) {
        std::cout << "[" << channel << "] Starting monitor..." << std::endl;
    }
    
    return 0;
}

int cmd_remove(const std::string& name) {
    core::ConfigManager config_mgr;
    
    if (!config_mgr.creator_exists(name)) {
        std::cerr << "Creator '" << name << "' not found." << std::endl;
        return 1;
    }
    
    if (config_mgr.remove_creator(name)) {
        std::cout << "Creator '" << name << "' removed." << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to remove creator." << std::endl;
        return 1;
    }
}

}
}
