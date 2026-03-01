#pragma once

#include <string>
#include <regex>
#include <algorithm>

namespace ctic {
namespace providers {

inline std::string extract_channel_from_url(const std::string& url) {
    std::regex pattern(R"(https?:\/\/(?:www\.)?twitch\.tv\/([a-zA-Z0-9_]+))");
    std::smatch match;
    
    if (std::regex_search(url, match, pattern) && match.size() > 1) {
        return match[1].str();
    }
    
    return "";
}

inline std::string normalize_channel(const std::string& channel) {
    std::string result = channel;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    
    if (!result.empty() && result[0] == '#') {
        result = result.substr(1);
    }
    
    return result;
}

inline bool is_valid_twitch_url(const std::string& url) {
    return !extract_channel_from_url(url).empty();
}

inline std::string parse_url_or_channel(const std::string& input) {
    if (is_valid_twitch_url(input)) {
        return extract_channel_from_url(input);
    }
    return normalize_channel(input);
}

}
}
