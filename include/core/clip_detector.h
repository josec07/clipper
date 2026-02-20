// added by [X] LLM model
// Ref: Clip event detection with cooldown - see docs/REFERENCES.md "Event Detection + Cooldown"
#pragma once

#include "chat_types.h"
#include "chat_buffer.h"
#include "spike_detector.h"
#include "keyword_matcher.h"
#include "scoring_engine.h"
#include <functional>
#include <chrono>

namespace chatclipper {

struct ClipDetectorConfig {
    std::chrono::seconds clip_duration{60};
    std::chrono::seconds lookback{30};
    std::chrono::seconds cooldown{30};
    double min_score{0.7};
};

class ClipDetector {
public:
    using ClipCallback = std::function<void(const ClipEvent&)>;
    
    ClipDetector(const ClipDetectorConfig& config,
                KeywordMatcher* keyword_matcher,
                ScoringEngine* scoring_engine);
    
    // Process a new message
    void processMessage(const ChatMessage& msg);
    
    // Set callback for when clip is detected
    void onClip(ClipCallback callback);
    
    // Force evaluation (call periodically or on buffer updates)
    void evaluate();
    
    // Get last clip time
    std::chrono::milliseconds getLastClipTime() const;
    
    // Reset detector state
    void reset();
    
private:
    ClipDetectorConfig config_;
    ChatBuffer buffer_;
    SpikeDetector spike_detector_;
    KeywordMatcher* keyword_matcher_;
    ScoringEngine* scoring_engine_;
    ClipCallback clip_callback_;
    
    std::chrono::milliseconds last_clip_time_;
    bool in_cooldown_;
    
    bool checkCooldown() const;
    ClipEvent createClipEvent(std::chrono::milliseconds peak_time, 
                             double score,
                             const std::vector<std::string>& keywords);
    std::vector<std::string> extractTopKeywords(const std::vector<ChatMessage>& messages);
};

} // namespace chatclipper
