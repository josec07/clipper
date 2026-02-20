// added by [X] LLM model
#include "keyword_matcher.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace chatclipper {

// Ref: Fuzzy matching threshold - see docs/REFERENCES.md "Fuzzy String Matching"
KeywordMatcher::KeywordMatcher(double similarity_threshold)
    : similarity_threshold_(similarity_threshold) {}

// Ref: Keyword category management - see docs/REFERENCES.md "Fuzzy String Matching"
void KeywordMatcher::addCategory(const KeywordCategory& category) {
    categories_.push_back(category);
}

// Ref: Fuzzy keyword matching - see docs/REFERENCES.md "Fuzzy String Matching"
std::unordered_map<std::string, std::string> KeywordMatcher::match(const std::string& message) const {
    std::unordered_map<std::string, std::string> results;
    std::string normalized_msg = normalize(message);
    
    for (const auto& category : categories_) {
        double best_score = 0.0;
        std::string best_match;
        
        for (const auto& keyword : category.keywords) {
            double similarity = calculateSimilarity(normalized_msg, normalize(keyword));
            if (similarity > best_score && similarity >= similarity_threshold_) {
                best_score = similarity;
                best_match = keyword;
            }
        }
        
        if (!best_match.empty()) {
            results[category.name] = best_match;
        }
    }
    
    return results;
}

// Ref: Category-specific scoring - see docs/REFERENCES.md "Fuzzy String Matching"
double KeywordMatcher::getCategoryScore(const std::string& message, 
                                       const std::string& category_name) const {
    auto matches = match(message);
    auto it = matches.find(category_name);
    if (it != matches.end()) {
        return calculateSimilarity(normalize(message), normalize(it->second));
    }
    return 0.0;
}

// Ref: Similarity threshold configuration - see docs/REFERENCES.md "Fuzzy String Matching"
void KeywordMatcher::setThreshold(double threshold) {
    similarity_threshold_ = threshold;
}




// Ref: Levenshtein edit distance - see docs/REFERENCES.md "Levenshtein Distance"
int KeywordMatcher::levenshteinDistance(const std::string& a, const std::string& b) const {
    int m = a.size();
    int n = b.size();
    
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    
    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;
    
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min(
                std::min(dp[i - 1][j] + 1, dp[i][j - 1] + 1),
                dp[i - 1][j - 1] + cost
            );
        }
    }
    
    return dp[m][n];
}

// Ref: Normalized Levenshtein similarity - see docs/REFERENCES.md "Levenshtein Distance"
double KeywordMatcher::calculateSimilarity(const std::string& a, const std::string& b) const {
    if (a.empty() && b.empty()) return 1.0;
    if (a.empty() || b.empty()) return 0.0;
    
    // Normalize inputs to handle case and repeated characters consistently
    std::string normA = normalize(a);
    std::string normB = normalize(b);
    
    int distance = levenshteinDistance(normA, normB);
    int maxLen = std::max(normA.size(), normB.size());
    return maxLen == 0 ? 1.0 : 1.0 - (static_cast<double>(distance) / maxLen);
}

// Ref: Text normalization - see docs/REFERENCES.md "Text Normalization"
std::string KeywordMatcher::normalize(const std::string& text) const {
    std::string result;
    result.reserve(text.size());
    
    for (char c : text) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        }
    }
    
    // Remove repeated characters (e.g., "wwww" -> "w")
    std::string deduped;
    deduped.reserve(result.size());
    char prev = '\0';
    for (char c : result) {
        if (c != prev) {
            deduped += c;
            prev = c;
        }
    }
    
    return deduped;
}

} // namespace chatclipper
