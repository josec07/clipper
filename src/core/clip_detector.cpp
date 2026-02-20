// added by [X] LLM model
#include "clip_detector.h"
#include <algorithm>

namespace chatclipper {

// Ref: Detection orchestration setup - see docs/REFERENCES.md "Event Detection + Cooldown"
ClipDetector::ClipDetector(const ClipDetectorConfig& config,
                          KeywordMatcher* keyword_matcher,
                          ScoringEngine* scoring_engine)
    : config_(config),
      buffer_(std::chrono::seconds(300)),
      spike_detector_(60, 3.0),
      keyword_matcher_(keyword_matcher),
      scoring_engine_(scoring_engine),
      last_clip_time_(std::chrono::milliseconds(0)),
      in_cooldown_(false) {}

// Ref: Message processing pipeline - see docs/REFERENCES.md "Event Detection + Cooldown"
void ClipDetector::processMessage(const ChatMessage& msg) {
    buffer_.addMessage(msg);
    
    // Update spike detector with current message rate
    double rate = buffer_.getMessageRate(std::chrono::seconds(10));
    spike_detector_.addSample(rate);
    
    evaluate();
}

// Ref: Event callback registration - see docs/REFERENCES.md "Event Detection + Cooldown"
void ClipDetector::onClip(ClipCallback callback) {
    clip_callback_ = callback;
}

// Ref: Clip detection evaluation - see docs/REFERENCES.md "Event Detection + Cooldown"
void ClipDetector::evaluate() {
    if (checkCooldown()) return;
    
    double spike_score = spike_detector_.getSpikeIntensity();
    
    // Get recent messages and check for keywords
    auto recent_messages = buffer_.getWindow(std::chrono::seconds(30));
    std::unordered_map<std::string, double> keyword_scores;
    std::vector<std::string> matched_keywords;
    
    for (const auto& msg : recent_messages) {
        auto matches = keyword_matcher_->match(msg.content);
        for (const auto& match : matches) {
            keyword_scores[match.first] = std::max(
                keyword_scores[match.first],
                keyword_matcher_->getCategoryScore(msg.content, match.first)
            );
            matched_keywords.push_back(match.second);
        }
    }
    
    // Calculate overall score
    double score = scoring_engine_->calculateScore(spike_score, keyword_scores);
    
    if (scoring_engine_->shouldTriggerClip(score)) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        
        ClipEvent event = createClipEvent(now, score, matched_keywords);
        
        if (clip_callback_) {
            clip_callback_(event);
        }
        
        last_clip_time_ = now;
        in_cooldown_ = true;
    }
}

// Ref: Cooldown state query - see docs/REFERENCES.md "Cooldown Mechanism"
std::chrono::milliseconds ClipDetector::getLastClipTime() const {
    return last_clip_time_;
}

// Ref: State reset - see docs/REFERENCES.md "Event Detection + Cooldown"
void ClipDetector::reset() {
    buffer_.cleanup();
    buffer_.setMaxDuration(std::chrono::seconds(300));
    spike_detector_.reset();
    last_clip_time_ = std::chrono::milliseconds(0);
    in_cooldown_ = false;
}

// Ref: Cooldown enforcement - see docs/REFERENCES.md "Cooldown Mechanism"
bool ClipDetector::checkCooldown() const {
    if (!in_cooldown_) return false;
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    
    auto elapsed = now - last_clip_time_;
    return elapsed < std::chrono::duration_cast<std::chrono::milliseconds>(config_.cooldown);
}

// Ref: 60s clip window creation - see docs/REFERENCES.md "Event Detection + Cooldown"
ClipEvent ClipDetector::createClipEvent(std::chrono::milliseconds peak_time, 
                                       double score,
                                       const std::vector<std::string>& keywords) {
    ClipEvent event;
    event.end_time = peak_time + std::chrono::duration_cast<std::chrono::milliseconds>(config_.lookback);
    event.start_time = event.end_time - std::chrono::duration_cast<std::chrono::milliseconds>(config_.clip_duration);
    event.score = score;
    event.keywords = keywords;
    
    // Determine category based on keywords
    if (!keywords.empty()) {
        // Simple heuristic: check first keyword
        const std::string& kw = keywords[0];
        if (kw == "W" || kw == "pog" || kw == "clutch" || kw == "insane") {
            event.category = "win";
        } else if (kw == "L" || kw == "ff" || kw == "sad") {
            event.category = "loss";
        } else if (kw == "OMEGALUL" || kw == "KEKW") {
            event.category = "hype";
        } else {
            event.category = "spike";
        }
    } else {
        event.category = "spike";
    }
    
    return event;
}

// Ref: Keyword aggregation - domain-specific heuristic
std::vector<std::string> ClipDetector::extractTopKeywords(const std::vector<ChatMessage>& messages) {
    std::vector<std::string> keywords;
    for (const auto& msg : messages) {
        auto matches = keyword_matcher_->match(msg.content);
        for (const auto& match : matches) {
            keywords.push_back(match.second);
        }
    }
    return keywords;
}

} // namespace chatclipper
