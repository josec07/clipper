// added by [X] LLM model
#include "scoring_engine.h"
#include <algorithm>

namespace chatclipper {

// Ref: Weighted scoring configuration - see docs/REFERENCES.md "Weighted Scoring"
ScoringEngine::ScoringEngine(const ScoringConfig& config)
    : config_(config) {}

// Ref: Weighted sum aggregation - see docs/REFERENCES.md "Weighted Scoring"
double ScoringEngine::calculateScore(double spike_score,
                                     const std::unordered_map<std::string, double>& keyword_scores,
                                     double uniqueness_score) const {
    double keyword_score = normalizeKeywordScore(keyword_scores);
    
    double total_score = 
        config_.weights.spike_weight * spike_score +
        config_.weights.keyword_weight * keyword_score +
        config_.weights.uniqueness_weight * uniqueness_score;
    
    return std::min(total_score, 1.0);
}

// Ref: Score threshold detection - see docs/REFERENCES.md "Weighted Scoring"
bool ScoringEngine::shouldTriggerClip(double score) const {
    return score >= config_.min_clip_score;
}

// Ref: Dynamic configuration update - see docs/REFERENCES.md "Weighted Scoring"
void ScoringEngine::setConfig(const ScoringConfig& config) {
    config_ = config;
}

// Ref: Keyword score normalization - domain-specific heuristic
double ScoringEngine::normalizeKeywordScore(const std::unordered_map<std::string, double>& scores) const {
    if (scores.empty()) return 0.0;
    
    double max_score = 0.0;
    for (const auto& pair : scores) {
        max_score = std::max(max_score, pair.second);
    }
    
    return max_score;
}

} // namespace chatclipper
