// added by [X] LLM model
// Ref: Circular buffer with time-based expiration - see docs/REFERENCES.md "Circular Buffer"
#pragma once

#include "chat_types.h"
#include <deque>
#include <chrono>
#include <vector>
#include <functional>
#include <mutex>

namespace chatclipper {

class ChatBuffer {
public:
    explicit ChatBuffer(std::chrono::seconds max_duration = std::chrono::seconds(300));
    
    void addMessage(const ChatMessage& msg);
    
    // Get messages within time windows
    std::vector<ChatMessage> getWindow(std::chrono::seconds duration) const;
    std::vector<ChatMessage> getWindow(std::chrono::milliseconds start, 
                                       std::chrono::milliseconds end) const;
    
    // Get message rate (messages per second)
    double getMessageRate(std::chrono::seconds window) const;
    
    // Get total message count in buffer
    size_t size() const { return buffer_.size(); }
    
    // Set max duration
    void setMaxDuration(std::chrono::seconds max_duration);
    
    // Clear expired messages
    void cleanup();
    
private:
    std::deque<ChatMessage> buffer_;
    std::chrono::seconds max_duration_;
    mutable std::mutex mutex_;
    double sum_;
    double sum_sq_;
    
    std::chrono::milliseconds getCurrentTime() const;
};

} // namespace chatclipper
