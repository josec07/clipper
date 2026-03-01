// Ref: Circular buffer with time expiration - see docs/REFERENCES.md "Circular Buffer"
#include "../../include/core/chat_buffer.h"
#include <algorithm>

namespace ctic {
namespace core {

ChatBuffer::ChatBuffer(std::chrono::seconds max_duration)
    : max_duration_(max_duration), sum_(0.0), sum_sq_(0.0) {}

void ChatBuffer::addMessage(const ChatMessage& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push_back(msg);
    cleanup();
}

std::vector<ChatMessage> ChatBuffer::getWindow(std::chrono::seconds duration) const {
    auto now = getCurrentTime();
    auto start = now - std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return getWindow(start, now);
}

std::vector<ChatMessage> ChatBuffer::getWindow(std::chrono::milliseconds start, 
                                                std::chrono::milliseconds end) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ChatMessage> result;
    
    for (const auto& msg : buffer_) {
        auto msg_ms = msg.timestamp_ms();
        if (msg_ms >= start && msg_ms <= end) {
            result.push_back(msg);
        }
    }
    
    return result;
}

double ChatBuffer::getMessageRate(std::chrono::seconds window) const {
    auto messages = getWindow(window);
    if (messages.empty()) return 0.0;
    
    return static_cast<double>(messages.size()) / window.count();
}

void ChatBuffer::setMaxDuration(std::chrono::seconds max_duration) {
    max_duration_ = max_duration;
    cleanup();
}

void ChatBuffer::cleanup() {
    auto now = getCurrentTime();
    auto cutoff = now - std::chrono::duration_cast<std::chrono::milliseconds>(max_duration_);
    
    while (!buffer_.empty() && buffer_.front().timestamp_ms() < cutoff) {
        buffer_.pop_front();
    }
}

std::chrono::milliseconds ChatBuffer::getCurrentTime() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

}
}
