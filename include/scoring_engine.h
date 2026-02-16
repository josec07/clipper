// added by [X] LLM model
// Ref: Weighted scoring for clip detection - see docs/REFERENCES.md "Weighted Scoring"
#pragma once

#include "chat_types.h"
#include <unordered_map>
#include <string>

namespace chatclipper {

struct ScoringWeights {
    double spike_weight = 0.4;
    double keyword_weight = 0.5;
    double uniqueness_weight = 0.1;
};

struct ScoringConfig {
    ScoringWeights weights;
    double min_clip_score = 0.7;
    double keyword_threshold = 0.3;  // Minimum keyword density for category
};

class ScoringEngine {
public:
    explicit ScoringEngine(const ScoringConfig& config = ScoringConfig{});
    
    // Calculate overall clip score
    double calculateScore(double spike_score,
                         const std::unordered_map<std::string, double>& keyword_scores,
                         double uniqueness_score = 1.0) const;
    
    // Check if score exceeds threshold for clip
    bool shouldTriggerClip(double score) const;
    
    // Update configuration
    void setConfig(const ScoringConfig& config);
    
    // Get current config
    ScoringConfig getConfig() const { return config_; }
    
private:
    ScoringConfig config_;
    
    double normalizeKeywordScore(const std::unordered_map<std::string, double>& scores) const;
};

} // namespace chatclipper
