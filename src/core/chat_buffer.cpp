// added by [X] LLM model
#include "chat_buffer.h"
#include <algorithm>

namespace chatclipper {

// Ref: Circular buffer with time expiration - see docs/REFERENCES.md "Circular Buffer"
ChatBuffer::ChatBuffer(std::chrono::seconds max_duration)
    : max_duration_(max_duration), sum_(0.0), sum_sq_(0.0) {}

// Ref: Thread-safe message insertion - see docs/REFERENCES.md "Circular Buffer"
void ChatBuffer::addMessage(const ChatMessage& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push_back(msg);
    cleanup();
}

// Ref: Sliding window query - see docs/REFERENCES.md "Circular Buffer"
std::vector<ChatMessage> ChatBuffer::getWindow(std::chrono::seconds duration) const {
    return getWindow(getCurrentTime() - std::chrono::duration_cast<std::chrono::milliseconds>(duration), 
                     getCurrentTime());
}

// Ref: Time range query - see docs/REFERENCES.md "Circular Buffer"
std::vector<ChatMessage> ChatBuffer::getWindow(std::chrono::milliseconds start, 
                                                std::chrono::milliseconds end) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ChatMessage> result;
    
    for (const auto& msg : buffer_) {
        if (msg.timestamp >= start && msg.timestamp <= end) {
            result.push_back(msg);
        }
    }
    
    return result;
}

// Ref: Message rate calculation - see docs/REFERENCES.md "Circular Buffer"
double ChatBuffer::getMessageRate(std::chrono::seconds window) const {
    auto messages = getWindow(window);
    if (messages.empty()) return 0.0;
    
    return static_cast<double>(messages.size()) / window.count();
}

// Ref: Buffer size configuration - see docs/REFERENCES.md "Circular Buffer"
void ChatBuffer::setMaxDuration(std::chrono::seconds max_duration){
    max_duration = max_duration_;
    cleanup();
}

// Ref: Auto-expiration of old messages - see docs/REFERENCES.md "Circular Buffer"
void ChatBuffer::cleanup() {
    auto now = getCurrentTime();
    auto cutoff = now - std::chrono::duration_cast<std::chrono::milliseconds>(max_duration_);
    
    while (!buffer_.empty() && buffer_.front().timestamp < cutoff) {
        buffer_.pop_front();
    }
}

// Ref: Current timestamp - standard practice
std::chrono::milliseconds ChatBuffer::getCurrentTime() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

} // namespace chatclipper
