#pragma once

#include "chat_buffer.h"
#include "spike_detector.h"
#include "detection.h"
#include "config.h"
#include <string>
#include <fstream>
#include <memory>

namespace ctic {
namespace core {

struct ClipEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string creator_name;
    std::string tier;
    std::string matched_word;
    std::string sentiment;
    int burst_count;
    int spike_z_score;
    int users_matched;
    double spike_intensity;
    std::string sample_messages;
    std::string config_id;
};

class Monitor {
private:
    std::string creator_name_;
    std::string channel_;
    CreatorConfig creator_config_;
    
    ChatBuffer chat_buffer_;
    SpikeDetector spike_detector_;
    
    std::unordered_map<std::string, std::unique_ptr<Detector>> detectors_;
    std::unordered_map<std::string, std::unique_ptr<std::ofstream>> log_files_;
    
    int total_messages_ = 0;
    int total_bursts_ = 0;
    std::chrono::system_clock::time_point last_rate_sample_;
    int messages_in_window_ = 0;
    
    std::deque<std::pair<std::string, std::string>> recent_messages_;
    
    bool running_ = true;
    
    void initializeDetectors(ConfigManager& config_mgr);
    void initializeLogFiles();
    void writeCSVHeader(const std::string& tier);
    void logBurst(const ClipEvent& event, const std::string& tier);
    
public:
    Monitor(const std::string& creator_name, ConfigManager& config_mgr);
    
    void processMessage(const std::string& username, const std::string& content);
    
    void updateSpikeDetector();
    
    bool hasSpike() const;
    double getSpikeIntensity() const;
    
    void stop() { running_ = false; }
    bool isRunning() const { return running_; }
    
    int totalMessages() const { return total_messages_; }
    int totalBursts() const { return total_bursts_; }
    
    void saveCreatorStats(ConfigManager& config_mgr);
};

}
}
