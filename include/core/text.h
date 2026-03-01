#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace ctic {
namespace core {

int levenshtein_distance(const std::string& s1, const std::string& s2);

inline double calculate_similarity(const std::string& s1, const std::string& s2) {
    if (s1.empty() && s2.empty()) return 1.0;
    if (s1.empty() || s2.empty()) return 0.0;
    
    int distance = levenshtein_distance(s1, s2);
    int max_len = std::max(s1.length(), s2.length());
    return 1.0 - (static_cast<double>(distance) / max_len);
}

inline std::string normalize_text(const std::string& text) {
    std::string result;
    result.reserve(text.length());
    
    bool last_was_space = true;
    for (char c : text) {
        if (std::isspace(c)) {
            if (!last_was_space) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += std::toupper(c);
            last_was_space = false;
        }
    }
    
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

inline bool word_matches(const std::string& word, const std::string& message, double threshold = 0.8) {
    std::string norm_word = normalize_text(word);
    std::string norm_msg = normalize_text(message);
    
    if (norm_word.length() > 4) {
        return norm_msg.find(norm_word) != std::string::npos;
    }
    
    std::istringstream ss(norm_msg);
    std::string msg_word;
    while (ss >> msg_word) {
        if (calculate_similarity(norm_word, msg_word) >= threshold) {
            return true;
        }
    }
    
    return false;
}

}
}
