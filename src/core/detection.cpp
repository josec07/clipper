#include "../../include/core/detection.h"
#include "../../include/core/text.h"
#include <algorithm>

namespace ctic {
namespace core {

BurstDetector::BurstDetector(int window_sec, int threshold, double sim_threshold, bool use_lev)
    : window_seconds_(window_sec)
    , threshold_(threshold)
    , similarity_threshold_(sim_threshold)
    , use_levenshtein_(use_lev) {}

int BurstDetector::count_burst(const std::string& word, const std::string& username,
                               const std::string& content, std::chrono::system_clock::time_point now) {
    auto cutoff = now - std::chrono::seconds(window_seconds_);
    
    while (!recent_matches_.empty() && recent_matches_.front().timestamp < cutoff) {
        recent_matches_.pop_front();
    }
    
    std::string norm_word = normalize_text(word);
    int count = 1;
    
    for (const auto& entry : recent_matches_) {
        std::string norm_entry = normalize_text(entry.content);
        
        if (use_levenshtein_) {
            double sim = calculate_similarity(norm_word, norm_entry);
            if (sim >= similarity_threshold_) {
                count++;
            }
        } else {
            if (norm_word == norm_entry) {
                count++;
            }
        }
    }
    
    recent_matches_.push_back({now, username, content});
    return count;
}

int BurstDetector::unique_users() const {
    std::unordered_set<std::string> users;
    for (const auto& entry : recent_matches_) {
        users.insert(entry.username);
    }
    return users.size();
}

void BurstDetector::reset() {
    recent_matches_.clear();
}

Detector::Detector(const DetectionConfig& config)
    : config_(config)
    , burst_detector_(config.burst_window_seconds, config.burst_threshold,
                       config.levenshtein_threshold, config.use_levenshtein) {}

BurstResult Detector::process_message(const std::string& username, const std::string& content,
                                       std::chrono::system_clock::time_point timestamp) {
    BurstResult result;
    
    if (!check_match(content, result.matched_word, result.sentiment)) {
        return result;
    }
    
    total_matches_++;
    
    int count = burst_detector_.count_burst(result.matched_word, username, content, timestamp);
    
    if (count >= config_.burst_threshold) {
        result.detected = true;
        result.burst_count = count;
        result.users_matched = burst_detector_.unique_users();
        total_bursts_++;
    }
    
    return result;
}

bool Detector::check_match(const std::string& content, std::string& matched_word, 
                           std::string& sentiment) const {
    for (const auto& word : config_.positive_words) {
        if (word_matches(word, content, config_.levenshtein_threshold)) {
            matched_word = word;
            sentiment = "positive";
            return true;
        }
    }
    
    for (const auto& word : config_.negative_words) {
        if (word_matches(word, content, config_.levenshtein_threshold)) {
            matched_word = word;
            sentiment = "negative";
            return true;
        }
    }
    
    return false;
}

}
}
