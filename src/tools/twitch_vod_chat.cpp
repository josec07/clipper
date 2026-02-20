#include "twitch_vod_chat.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>

namespace chatclipper {

TwitchVODChat::TwitchVODChat() : clientId_("kd1unb4b3q4t58fwlpcbzcbnm76a8fp") {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

TwitchVODChat::~TwitchVODChat() {
    curl_global_cleanup();
}

size_t TwitchVODChat::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

TwitchVODChat::GraphQLResponse TwitchVODChat::makeGraphQLRequest(const std::string& query) {
    GraphQLResponse result;
    result.success = false;
    result.httpCode = 0;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        lastError_ = "CURL not initialized";
        return result;
    }
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string clientIdHeader = "Client-ID: " + clientId_;
    headers = curl_slist_append(headers, clientIdHeader.c_str());
    
    std::string responseBody;
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://gql.twitch.tv/gql");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        lastError_ = "CURL error: " + std::string(curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return result;
    }
    
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    result.httpCode = static_cast<int>(httpCode);
    result.data = responseBody;
    result.success = (result.httpCode == 200);
    
    if (!result.success) {
        lastError_ = "HTTP " + std::to_string(result.httpCode) + ": " + responseBody.substr(0, 200);
    }
    
    curl_easy_cleanup(curl);
    return result;
}

bool TwitchVODChat::parseCommentsResponse(const std::string& json, std::string& nextCursor, bool& hasNextPage) {
    try {
        nlohmann::json root = nlohmann::json::parse(json);
        
        if (root.contains("errors")) {
            lastError_ = "GraphQL error: " + root["errors"][0]["message"].get<std::string>();
            return false;
        }
        
        if (!root.contains("data") || root["data"].is_null()) {
            lastError_ = "No data in response";
            return false;
        }
        
        auto& video = root["data"]["video"];
        if (video.is_null()) {
            lastError_ = "Video not found";
            return false;
        }
        
        auto& commentsConn = video["comments"];
        if (commentsConn.is_null()) {
            hasNextPage = false;
            return true;
        }
        
        auto& edges = commentsConn["edges"];
        if (!edges.is_array() || edges.empty()) {
            hasNextPage = false;
            return true;
        }
        
        for (auto& edge : edges) {
            if (!edge.contains("node") || edge["node"].is_null()) continue;
            
            auto& node = edge["node"];
            VODComment c;
            
            if (node.contains("contentOffsetSeconds")) {
                c.offset_seconds = node["contentOffsetSeconds"].get<double>();
            }
            
            if (node.contains("id")) {
                c.message_id = node["id"].get<std::string>();
            }
            
            if (node.contains("commenter") && !node["commenter"].is_null()) {
                auto& commenter = node["commenter"];
                if (commenter.contains("displayName") && !commenter["displayName"].is_null()) {
                    c.username = commenter["displayName"].get<std::string>();
                }
                if (commenter.contains("id") && !commenter["id"].is_null()) {
                    c.user_id = commenter["id"].get<std::string>();
                }
            }
            
            if (node.contains("message") && !node["message"].is_null()) {
                auto& message = node["message"];
                if (message.contains("fragments") && message["fragments"].is_array()) {
                    std::string fullMsg;
                    for (auto& frag : message["fragments"]) {
                        if (frag.contains("text") && !frag["text"].is_null()) {
                            fullMsg += frag["text"].get<std::string>();
                        }
                    }
                    c.message = fullMsg;
                }
            }
            
            if (!c.message.empty() || !c.username.empty()) {
                comments_.push_back(std::move(c));
            }
        }
        
        nextCursor = edges.back()["cursor"].get<std::string>();
        
        if (commentsConn.contains("pageInfo") && !commentsConn["pageInfo"].is_null()) {
            hasNextPage = commentsConn["pageInfo"]["hasNextPage"].get<bool>();
        } else {
            hasNextPage = false;
        }
        
        return true;
    } catch (const nlohmann::json::exception& e) {
        lastError_ = "JSON parse error: " + std::string(e.what());
        return false;
    }
}

bool TwitchVODChat::fetchChat(int64_t videoId, ProgressCallback callback) {
    comments_.clear();
    lastError_.clear();
    
    std::string cursor;
    bool hasNextPage = true;
    size_t totalMessages = 0;
    int requestCount = 0;
    int errorCount = 0;
    const int maxRequests = 50000;
    const int maxErrors = 10;
    
    while (hasNextPage && requestCount < maxRequests) {
        nlohmann::json request;
        request["operationName"] = "VideoCommentsByOffsetOrCursor";
        request["variables"]["videoID"] = std::to_string(videoId);
        
        if (cursor.empty()) {
            request["variables"]["contentOffsetSeconds"] = 0;
        } else {
            request["variables"]["cursor"] = cursor;
        }
        
        request["extensions"]["persistedQuery"]["version"] = 1;
        request["extensions"]["persistedQuery"]["sha256Hash"] = "b70a3591ff0f4e0313d126c6a1502d79a1c02baebb288227c582044aa76adf6a";
        
        auto response = makeGraphQLRequest(request.dump());
        
        if (!response.success) {
            if (++errorCount > maxErrors) {
                lastError_ = "Too many errors. Last error: " + lastError_;
                return false;
            }
            usleep(1000000 * errorCount);
            continue;
        }
        
        if (!parseCommentsResponse(response.data, cursor, hasNextPage)) {
            if (lastError_.find("timeout") != std::string::npos || 
                lastError_.find("service") != std::string::npos) {
                if (++errorCount > maxErrors) {
                    return false;
                }
                usleep(1000000 * errorCount);
                continue;
            }
            return false;
        }
        
        errorCount = 0;
        totalMessages = comments_.size();
        requestCount++;
        
        if (callback) {
            callback(hasNextPage ? 0.5 : 1.0, totalMessages);
        }
        
        usleep(100000);
    }
    
    if (callback) {
        callback(1.0, totalMessages);
    }
    
    return true;
}

bool TwitchVODChat::saveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "[\n";
    for (size_t i = 0; i < comments_.size(); ++i) {
        const auto& c = comments_[i];
        
        nlohmann::json msgJson;
        msgJson["offset_seconds"] = c.offset_seconds;
        msgJson["username"] = c.username;
        msgJson["message"] = c.message;
        
        file << "  " << msgJson.dump();
        if (i < comments_.size() - 1) file << ",";
        file << "\n";
    }
    file << "]\n";
    
    return true;
}

void TwitchVODChat::printToStdout() const {
    for (const auto& c : comments_) {
        int64_t timestamp_ms = static_cast<int64_t>(c.offset_seconds * 1000);
        std::cout << timestamp_ms << "|" << c.username << "|" << c.message << "\n";
    }
    std::cout.flush();
}

} // namespace chatclipper

void printVODUsage(const char* program) {
    std::cerr << "Usage: " << program << " --video <video_id> [options]\n"
              << "Options:\n"
              << "  --video <id>       VOD/Video ID to fetch chat from (required)\n"
              << "  --output <file>    Save chat to JSON file\n"
              << "  --stdout           Print chat in pipe-delimited format (default)\n"
              << "  --client-id <id>   Override Twitch Client-ID\n"
              << "  --help             Show this help\n"
              << "\nOutput format (stdout):\n"
              << "  offset_ms|username|message\n"
              << "\nExample:\n"
              << "  " << program << " --video 2699618601\n";
}

int vodMain(int argc, char* argv[]) {
    int64_t videoId = 0;
    std::string outputFile;
    std::string clientId;
    bool useStdout = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printVODUsage(argv[0]);
            return 0;
        } else if (arg == "--video" && i + 1 < argc) {
            videoId = std::stoll(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
            useStdout = false;
        } else if (arg == "--stdout") {
            useStdout = true;
        } else if (arg == "--client-id" && i + 1 < argc) {
            clientId = argv[++i];
        }
    }
    
    if (videoId == 0) {
        std::cerr << "Error: --video is required\n";
        printVODUsage(argv[0]);
        return 1;
    }
    
    chatclipper::TwitchVODChat fetcher;
    
    if (!clientId.empty()) {
        fetcher.setClientId(clientId);
    }
    
    auto progressCallback = [](double, size_t count) {
        std::cerr << "\rFetching chat... " << count << " messages" << std::flush;
    };
    
    std::cerr << "Fetching chat for video " << videoId << "...\n";
    
    if (!fetcher.fetchChat(videoId, progressCallback)) {
        std::cerr << "\nError: " << fetcher.getLastError() << "\n";
        return 1;
    }
    
    std::cerr << "\nFetched " << fetcher.size() << " messages\n";
    
    if (!outputFile.empty()) {
        if (fetcher.saveToFile(outputFile)) {
            std::cerr << "Saved to " << outputFile << "\n";
        } else {
            std::cerr << "Failed to save to " << outputFile << "\n";
            return 1;
        }
    }
    
    if (useStdout) {
        fetcher.printToStdout();
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    return vodMain(argc, argv);
}
