#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <functional>

namespace chatclipper {

struct VODComment {
    double offset_seconds;
    std::string username;
    std::string user_id;
    std::string message;
    std::string message_id;
};

class TwitchVODChat {
public:
    using ProgressCallback = std::function<void(double progress, size_t messages_count)>;
    
    TwitchVODChat();
    ~TwitchVODChat();
    
    bool fetchChat(int64_t videoId, ProgressCallback callback = nullptr);
    
    const std::vector<VODComment>& getComments() const { return comments_; }
    std::vector<VODComment>&& releaseComments() { return std::move(comments_); }
    
    bool saveToFile(const std::string& path) const;
    void printToStdout() const;
    
    size_t size() const { return comments_.size(); }
    void clear() { comments_.clear(); }
    
    const std::string& getLastError() const { return lastError_; }
    
    void setClientId(const std::string& clientId) { clientId_ = clientId; }
    
private:
    std::vector<VODComment> comments_;
    std::string lastError_;
    std::string clientId_;
    
    struct GraphQLResponse {
        std::string data;
        bool success;
        int httpCode;
    };
    
    GraphQLResponse makeGraphQLRequest(const std::string& query);
    bool parseCommentsResponse(const std::string& json, std::string& nextCursor, bool& hasNextPage);
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

} // namespace chatclipper
