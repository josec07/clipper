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

inline std::string collapse_repeated_chars(const std::string& s) {
    if (s.empty()) return s;
    
    std::string result;
    result.reserve(s.length());
    
    char last = '\0';
    for (char c : s) {
        if (c != last) {
            result += c;
            last = c;
        }
    }
    
    return result;
}

inline bool contains_word(const std::string& needle, const std::string& haystack) {
    std::string norm_needle = normalize_text(needle);
    std::string norm_haystack = normalize_text(haystack);
    
    if (norm_needle.empty()) return true;
    
    for (char& c : norm_needle) c = std::toupper(c);
    for (char& c : norm_haystack) c = std::toupper(c);
    
    return norm_haystack.find(norm_needle) != std::string::npos;
}

inline bool word_matches(const std::string& word, const std::string& message, double threshold = 0.8) {
    std::string norm_word = normalize_text(word);
    std::string norm_msg = normalize_text(message);
    
    if (norm_word.length() > 4) {
        return contains_word(norm_word, norm_msg);
    }
    
    std::istringstream ss(norm_msg);
    std::string msg_word;
    while (ss >> msg_word) {
        std::string collapsed_token = collapse_repeated_chars(msg_word);
        
        if (norm_word.length() <= 2) {
            if (collapsed_token == norm_word) {
                return true;
            }
            if (collapsed_token.length() >= norm_word.length() &&
                collapsed_token.substr(0, norm_word.length()) == norm_word &&
                collapsed_token.length() <= norm_word.length() + 3) {
                return true;
            }
        } else {
            if (calculate_similarity(norm_word, collapsed_token) >= threshold) {
                return true;
            }
            if (collapsed_token.length() >= norm_word.length() &&
                collapsed_token.substr(0, norm_word.length()) == norm_word) {
                return true;
            }
        }
    }
    
    return false;
}

}
}
