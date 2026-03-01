#pragma once

#include <string>
#include <chrono>

namespace ctic {
namespace core {

struct ChatMessage {
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds timestamp_ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch());
    }
    std::string username;
    std::string content;
    std::string channel;
};

}
}
