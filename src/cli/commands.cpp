#include "../../include/cli/commands.h"
#include "../../include/core/config.h"
#include "../../include/providers/twitch_url.h"
#include "../../include/providers/twitch_irc.h"
#include "../../include/core/detection.h"
#include "../../include/core/text.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <csignal>
#include <unordered_map>

namespace ctic {
namespace cli {

volatile std::sig_atomic_t running = 1;

void signal_handler(int) {
    running = 0;
}

class CSVLogger {
private:
    std::ofstream file_;
    std::string filepath_;
    bool header_written_ = false;
    
public:
    CSVLogger(const std::string& creator, const std::string& tier) {
        std::stringstream path;
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        
        path << ".ctic/outputs/" << creator << "/" << tier;
        std::string dir = path.str();
        
        std::string cmd = "mkdir -p " + dir;
        system(cmd.c_str());
        
        path << "/matches-" << tier << "-" 
             << std::put_time(std::localtime(&t), "%Y%m%d-%H%M%S") << ".csv";
        filepath_ = path.str();
        
        file_.open(filepath_, std::ios::app);
    }
    
    ~CSVLogger() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void write_header() {
        if (!header_written_ && file_.is_open()) {
            file_ << "timestamp,matched_word,sentiment,burst_count,window_seconds,users_matched,config_id,sample_messages\n";
            header_written_ = true;
        }
    }
    
    void log_burst(const std::string& timestamp, const std::string& word, 
                   const std::string& sentiment, int burst_count, int window_seconds,
                   int users_matched, const std::string& config_id,
                   const std::string& sample_messages) {
        if (!file_.is_open()) return;
        
        write_header();
        
        file_ << timestamp << ","
              << word << ","
              << sentiment << ","
              << burst_count << ","
              << window_seconds << ","
              << users_matched << ","
              << config_id << ",\""
              << sample_messages << "\"\n";
        
        file_.flush();
    }
    
    bool is_open() const { return file_.is_open(); }
    const std::string& path() const { return filepath_; }
};

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

core::DetectionConfig build_detection_config(const core::TierConfig& tier) {
    core::DetectionConfig config;
    config.positive_words = std::unordered_set<std::string>(tier.words.begin(), tier.words.end());
    config.burst_threshold = tier.burst_threshold;
    config.burst_window_seconds = tier.window_seconds;
    config.levenshtein_threshold = tier.levenshtein_threshold;
    config.use_levenshtein = tier.use_levenshtein;
    config.tier_name = tier.tier_name;
    return config;
}

int cmd_run_single(const std::string& creator_name) {
    core::ConfigManager config_mgr;
    
    if (!config_mgr.creator_exists(creator_name)) {
        std::cerr << "Creator '" << creator_name << "' not found." << std::endl;
        return 1;
    }
    
    auto creator = config_mgr.load_creator(creator_name);
    
    std::cout << "Starting monitor for: " << creator_name << std::endl;
    std::cout << "Channel: #" << creator.channel << std::endl;
    std::cout << "Tiers: ";
    for (const auto& tier : creator.enabled_tiers) {
        std::cout << tier << " ";
    }
    std::cout << std::endl;
    
    std::unordered_map<std::string, core::Detector> detectors;
    std::unordered_map<std::string, CSVLogger> loggers;
    
    for (const auto& tier_name : creator.enabled_tiers) {
        auto tier_config = config_mgr.load_tier_config(tier_name);
        auto detection_config = build_detection_config(tier_config);
        detectors.emplace(std::piecewise_construct,
                          std::forward_as_tuple(tier_name),
                          std::forward_as_tuple(detection_config));
        loggers.emplace(std::piecewise_construct,
                        std::forward_as_tuple(tier_name),
                        std::forward_as_tuple(creator_name, tier_name));
        
        std::cout << "  [" << tier_name << "] threshold=" << tier_config.burst_threshold 
                  << " words=" << tier_config.words.size() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Connecting to #" << creator.channel << "..." << std::endl;
    
    providers::TwitchIRC irc;
    if (!irc.connect(creator.channel)) {
        std::cerr << "Error: Failed to connect to #" << creator.channel << std::endl;
        return 1;
    }
    
    std::cout << "Connected. Monitoring started." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    std::cout << std::endl;
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    int total_messages = 0;
    int total_bursts = 0;
    std::deque<std::pair<std::string, std::string>> message_samples;
    
    while (running && irc.is_connected()) {
        std::string line = irc.read_line();
        if (line.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        std::string username, content;
        if (!irc.parse_message(line, username, content)) {
            continue;
        }
        
        total_messages++;
        
        message_samples.push_back({username, content});
        if (message_samples.size() > 10) {
            message_samples.pop_front();
        }
        
        auto timestamp = std::chrono::system_clock::now();
        
        for (auto& [tier_name, detector] : detectors) {
            auto result = detector.process_message(username, content, timestamp);
            
            if (result.detected) {
                total_bursts++;
                
                std::string samples_str;
                for (const auto& [user, msg] : message_samples) {
                    if (!samples_str.empty()) samples_str += " | ";
                    samples_str += user + ": " + msg;
                }
                
                std::string ts = core::format_timestamp(timestamp);
                
                loggers.at(tier_name).log_burst(
                    ts,
                    result.matched_word,
                    result.sentiment,
                    result.burst_count,
                    detector.config().burst_window_seconds,
                    result.users_matched,
                    creator.detector_config_id,
                    samples_str
                );
                
                std::cout << "[BURST:" << tier_name << "] " 
                          << result.matched_word << " "
                          << "x" << result.burst_count 
                          << " (" << result.users_matched << " users)"
                          << " -> " << loggers.at(tier_name).path()
                          << std::endl;
            }
        }
    }
    
    std::cout << std::endl;
    std::cout << "Monitoring stopped." << std::endl;
    std::cout << "Total messages: " << total_messages << std::endl;
    std::cout << "Total bursts detected: " << total_bursts << std::endl;
    
    irc.disconnect();
    
    creator.last_monitored = core::format_timestamp(std::chrono::system_clock::now());
    creator.total_sessions++;
    creator.total_clips_detected += total_bursts;
    config_mgr.save_creator(creator);
    
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
    
    if (creators.size() == 1) {
        return cmd_run_single(creators[0]);
    }
    
    std::cout << "Multiple creators configured. Monitoring first: " << creators[0] << std::endl;
    std::cout << "(Multi-creator support coming in mvp-with-threadpool)" << std::endl;
    std::cout << std::endl;
    
    return cmd_run_single(creators[0]);
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
