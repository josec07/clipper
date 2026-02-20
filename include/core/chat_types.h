// added by [X] LLM model
#pragma once

#include <string>
#include <chrono>
#include <vector>

namespace chatclipper {

struct ChatMessage {
    std::chrono::milliseconds timestamp;
    std::string username;
    std::string content;
};

struct ClipEvent {
    std::chrono::milliseconds start_time;
    std::chrono::milliseconds end_time;
    double score;
    std::vector<std::string> keywords;
    std::string category;  // "win", "loss", "hype", "spike"
};

} // namespace chatclipper
