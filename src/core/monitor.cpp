#include "../../include/core/monitor.h"
#include "../../include/core/text.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ctic {
namespace core {

Monitor::Monitor(const std::string& creator_name, ConfigManager& config_mgr)
    : creator_name_(creator_name)
    , chat_buffer_(std::chrono::seconds(300))
    , spike_detector_(60, 3.0)
    , last_rate_sample_(std::chrono::system_clock::now())
{
    creator_config_ = config_mgr.load_creator(creator_name);
    channel_ = creator_config_.channel;
    
    initializeDetectors(config_mgr);
    initializeLogFiles();
}

void Monitor::initializeDetectors(ConfigManager& config_mgr) {
    for (const auto& tier_name : creator_config_.enabled_tiers) {
        auto tier_config = config_mgr.load_tier_config(tier_name);
        
        DetectionConfig det_config;
        det_config.positive_words = std::unordered_set<std::string>(
            tier_config.words.begin(), tier_config.words.end());
        det_config.burst_threshold = tier_config.burst_threshold;
        det_config.burst_window_seconds = tier_config.window_seconds;
        det_config.levenshtein_threshold = tier_config.levenshtein_threshold;
        det_config.use_levenshtein = tier_config.use_levenshtein;
        det_config.tier_name = tier_name;
        det_config.min_word_length = tier_config.min_word_length;
        det_config.cooldown_seconds = tier_config.cooldown_seconds;
        det_config.require_unique_users = tier_config.require_unique_users;
        
        detectors_[tier_name] = std::make_unique<Detector>(det_config);
    }
}

void Monitor::initializeLogFiles() {
    for (const auto& tier_name : creator_config_.enabled_tiers) {
        std::stringstream dir_path;
        dir_path << ".ctic/outputs/" << creator_name_ << "/" << tier_name;
        
        std::string cmd = "mkdir -p " + dir_path.str();
        system(cmd.c_str());
        
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream file_path;
        file_path << dir_path.str() << "/matches-" << tier_name << "-"
                  << std::put_time(std::localtime(&t), "%Y%m%d-%H%M%S") << ".csv";
        
        log_files_[tier_name] = std::make_unique<std::ofstream>(file_path.str(), std::ios::app);
        writeCSVHeader(tier_name);
    }
}

void Monitor::writeCSVHeader(const std::string& tier) {
    auto& file = log_files_[tier];
    if (file && file->is_open()) {
        *file << "# CTIC Monitor Log\n";
        *file << "# Creator: " << creator_name_ << "\n";
        *file << "# Channel: #" << channel_ << "\n";
        *file << "# Tier: " << tier << "\n";
        *file << "#\n";
        *file << "timestamp,matched_word,sentiment,burst_count,spike_z_score,users_matched,spike_intensity,config_id,sample_messages\n";
        file->flush();
    }
}

void Monitor::processMessage(const std::string& username, const std::string& content) {
    total_messages_++;
    
    ChatMessage msg;
    msg.timestamp = std::chrono::system_clock::now();
    msg.username = username;
    msg.content = content;
    msg.channel = channel_;
    
    chat_buffer_.addMessage(msg);
    
    recent_messages_.push_back({username, content});
    if (recent_messages_.size() > 10) {
        recent_messages_.pop_front();
    }
    
    messages_in_window_++;
    
    for (auto& [tier_name, detector] : detectors_) {
        auto result = detector->process_message(username, content, msg.timestamp);
        
        if (result.detected) {
            total_bursts_++;
            
            ClipEvent event;
            event.timestamp = msg.timestamp;
            event.creator_name = creator_name_;
            event.tier = tier_name;
            event.matched_word = result.matched_word;
            event.sentiment = result.sentiment;
            event.burst_count = result.burst_count;
            event.spike_z_score = hasSpike() ? static_cast<int>(spike_detector_.getSpikeIntensity() * 5) : 0;
            event.users_matched = result.users_matched;
            event.spike_intensity = getSpikeIntensity();
            event.config_id = creator_config_.detector_config_id;
            
            for (const auto& [user, msg_content] : recent_messages_) {
                if (!event.sample_messages.empty()) event.sample_messages += " | ";
                event.sample_messages += user + ": " + msg_content;
            }
            
            logBurst(event, tier_name);
        }
    }
    
    updateSpikeDetector();
}

void Monitor::updateSpikeDetector() {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_rate_sample_);
    
    if (elapsed.count() >= 1) {
        double rate = static_cast<double>(messages_in_window_) / elapsed.count();
        spike_detector_.addSample(rate);
        
        messages_in_window_ = 0;
        last_rate_sample_ = now;
    }
}

bool Monitor::hasSpike() const {
    return spike_detector_.isSpike();
}

double Monitor::getSpikeIntensity() const {
    return spike_detector_.getSpikeIntensity();
}

void Monitor::logBurst(const ClipEvent& event, const std::string& tier) {
    auto& file = log_files_[tier];
    if (!file || !file->is_open()) return;
    
    std::string ts = format_timestamp(event.timestamp);
    
    *file << ts << ","
          << event.matched_word << ","
          << event.sentiment << ","
          << event.burst_count << ","
          << event.spike_z_score << ","
          << event.users_matched << ","
          << std::fixed << std::setprecision(2) << event.spike_intensity << ","
          << event.config_id << ",\""
          << event.sample_messages << "\"\n";
    
    file->flush();
}

void Monitor::saveCreatorStats(ConfigManager& config_mgr) {
    creator_config_.last_monitored = format_timestamp(std::chrono::system_clock::now());
    creator_config_.total_sessions++;
    creator_config_.total_clips_detected += total_bursts_;
    config_mgr.save_creator(creator_config_);
}

}
}
