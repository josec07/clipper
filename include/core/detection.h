#pragma once

#include <string>
#include <deque>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace ctic {
namespace core {

struct BurstResult {
    bool detected = false;
    std::string matched_word;
    std::string sentiment;
    int burst_count = 0;
    int users_matched = 0;
};

struct DetectionConfig {
    std::unordered_set<std::string> positive_words;
    std::unordered_set<std::string> negative_words;
    int burst_window_seconds = 30;
    int burst_threshold = 3;
    double levenshtein_threshold = 0.8;
    bool use_levenshtein = true;
    std::string tier_name = "medium";
    int min_word_length = 1;
    int cooldown_seconds = 30;
    int require_unique_users = 1;
};

class BurstDetector {
private:
    struct MatchEntry {
        std::chrono::system_clock::time_point timestamp;
        std::string username;
        std::string matched_word;
    };
    
    std::deque<MatchEntry> recent_matches_;
    int window_seconds_;
    int threshold_;
    double similarity_threshold_;
    bool use_levenshtein_;
    
public:
    BurstDetector(int window_sec = 30, int threshold = 3, double sim_threshold = 0.8, bool use_lev = true);
    
    int count_burst(const std::string& word, const std::string& username,
                    const std::string& matched_word, std::chrono::system_clock::time_point now);
    int unique_users() const;
    void reset();
};

class Detector {
private:
    DetectionConfig config_;
    BurstDetector burst_detector_;
    int total_matches_ = 0;
    int total_bursts_ = 0;
    std::chrono::system_clock::time_point last_detection_time_;
    bool in_cooldown_ = false;
    
public:
    Detector(const DetectionConfig& config);
    
    BurstResult process_message(const std::string& username, const std::string& content,
                                 std::chrono::system_clock::time_point timestamp);
    
    bool check_match(const std::string& content, std::string& matched_word, std::string& sentiment) const;
    
    int total_matches() const { return total_matches_; }
    int total_bursts() const { return total_bursts_; }
    const DetectionConfig& config() const { return config_; }
};

}
}
