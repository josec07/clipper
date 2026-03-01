// Ref: Circular buffer with time-based expiration - see docs/REFERENCES.md "Circular Buffer"
#pragma once

#include "chat_types.h"
#include <deque>
#include <chrono>
#include <vector>
#include <functional>
#include <mutex>

namespace ctic {
namespace core {

class ChatBuffer {
public:
    explicit ChatBuffer(std::chrono::seconds max_duration = std::chrono::seconds(300));
    
    void addMessage(const ChatMessage& msg);
    
    std::vector<ChatMessage> getWindow(std::chrono::seconds duration) const;
    std::vector<ChatMessage> getWindow(std::chrono::milliseconds start, 
                                       std::chrono::milliseconds end) const;
    
    double getMessageRate(std::chrono::seconds window) const;
    
    size_t size() const { return buffer_.size(); }
    
    void setMaxDuration(std::chrono::seconds max_duration);
    
    void cleanup();
    
private:
    std::deque<ChatMessage> buffer_;
    std::chrono::seconds max_duration_;
    mutable std::mutex mutex_;
    double sum_;
    double sum_sq_;
    
    std::chrono::milliseconds getCurrentTime() const;
};

}
}
