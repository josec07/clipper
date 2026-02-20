// added by [X] LLM model
// Ref: Fuzzy string matching with Levenshtein distance - see docs/REFERENCES.md "Fuzzy String Matching"
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace chatclipper {

struct KeywordCategory {
    std::string name;
    std::vector<std::string> keywords;
    double weight;
};

class KeywordMatcher {
public:

    explicit KeywordMatcher(double similarity_threshold = 0.8);
    
    // Add a category with keywords
    void addCategory(const KeywordCategory& category);
    
    // Match a message against all categories
    // Returns map of category -> best matching keyword
    std::unordered_map<std::string, std::string> match(const std::string& message) const;
    
    // Get match score for a specific category (0.0 - 1.0)
    double getCategoryScore(const std::string& message, 
                           const std::string& category) const;
    double calculateSimilarity(const std::string& a, const std::string& b) const;

    
    // Set similarity threshold (0.0 - 1.0)
    void setThreshold(double threshold);
    
    void setStrings(const std::string& a, const std::string& b);
private:

    std::vector<KeywordCategory> categories_;
    double similarity_threshold_;
    
    // Levenshtein distance implementation
    int levenshteinDistance(const std::string& a, const std::string& b) const;
    
    // Calculate similarity score (0.0 - 1.0)
    
    // Normalize text for matching
    std::string normalize(const std::string& text) const;
};

} // namespace chatclipper
