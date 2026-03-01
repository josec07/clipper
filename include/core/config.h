#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <chrono>

namespace ctic {
namespace core {

struct CreatorConfig {
    std::string name;
    std::string channel;
    std::string twitch_url;
    std::vector<std::string> enabled_tiers;
    std::string profile = "balanced";
    std::string detector_config_id;
    std::string created_at;
    std::string last_monitored;
    int total_sessions = 0;
    int total_clips_detected = 0;
};

struct TierConfig {
    std::string tier_name;
    std::vector<std::string> words;
    int burst_threshold = 3;
    int window_seconds = 30;
    double levenshtein_threshold = 0.8;
    bool use_levenshtein = true;
    int min_word_length = 1;
    int cooldown_seconds = 30;
    int require_unique_users = 1;
};

struct DetectorConfig {
    std::string id;
    std::string name;
    std::string algorithm;
    double similarity_threshold = 0.8;
    std::vector<TierConfig> tiers;
};

class ConfigManager {
private:
    std::string ctic_dir_;
    
public:
    ConfigManager();
    
    bool ensure_ctic_dir();
    std::string get_ctic_dir() const { return ctic_dir_; }
    
    CreatorConfig load_creator(const std::string& name);
    bool save_creator(const CreatorConfig& config);
    bool remove_creator(const std::string& name);
    std::vector<std::string> list_creators();
    bool creator_exists(const std::string& name);
    
    TierConfig load_tier_config(const std::string& tier_name);
    TierConfig load_profile_tier(const std::string& profile_name, const std::string& tier_name);
    DetectorConfig load_detector_config(const std::string& detector_id);
    
    std::string get_output_dir(const std::string& creator, const std::string& tier);
    std::string get_creators_dir();
    std::string get_profiles_dir();
};

std::string format_timestamp(std::chrono::system_clock::time_point tp);

}
}
