/**
 * References:
 * 
 * For Levenshtein Distance implementation:
 * @see docs/literature/levenshtein.bib
 *   Wagner & Fischer (1974) - The String-to-String Correction Problem
 * 
 * For stream highlight detection approach:
 * @see docs/literature/ringer_2020_twitchchat.bib
 *   Ringer et al. (2020) - TwitchChat: A Dataset for Exploring Livestream Chat
 * 
 * @see docs/literature/ringer_2022_phd.bib
 *   Ringer (2022) - Multi-Modal Livestream Highlight Detection from Audio, 
 *                    Visual, and Language Data
 * 
 * For Twitch chat and emote understanding:
 * @see docs/literature/barbieri_2017_emotes.bib
 *   Barbieri et al. (2017) - Towards the Understanding of Gaming Audiences 
 *                            by Modeling Twitch Emotes
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <cmath>

// Simple Levenshtein Distance implementation
int levenshteinDistance(const std::string& a, const std::string& b) {
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

// Normalize Levenshtein Distance to a similarity score between 0 and 1
double similarityScore(const std::string& a, const std::string& b) {
    int distance = levenshteinDistance(a, b);
    int maxLen = std::max(a.size(), b.size());
    return 1.0 - (static_cast<double>(distance) / maxLen);
}

// Struct to represent a single chat message
struct ChatMessage {
    std::string user;
    std::string content;
};

// Class to hold and compare chat logs
class ChatLog {
public:
    std::vector<ChatMessage> messages;

    // Add a message to the log
    void addMessage(const ChatMessage& message) {
        messages.push_back(message);
    }

    // Compare two users' messages and return a similarity score
    double compareUsers(const std::string& user1, const std::string& user2) {
        std::string msg1, msg2;

        // Find messages from both users
        for (const auto& msg : messages) {
            if (msg.user == user1) msg1 = msg.content;
            if (msg.user == user2) msg2 = msg.content;
        }

        // If either user has no message, return 0
        if (msg1.empty() || msg2.empty()) return 0.0;

        return similarityScore(msg1, msg2);
    }

    // Print all messages
    void printMessages() const {
        for (const auto& msg : messages) {
            std::cout << "User: " << msg.user << ", Message: " << msg.content << std::endl;
        }
    }
};