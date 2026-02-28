// live-monitor.cpp
// Chat monitor for theburntpeanut with tiered detection
// CSV logs metadata, terminal shows chat flow

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <cstring>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>

struct Config {
    std::unordered_set<std::string> positive_words;
    std::unordered_set<std::string> negative_words;
    int context_window_seconds = 120;
    int burst_window_seconds = 30;
    int burst_threshold = 3;
    double levenshtein_threshold = 0.8;
    bool use_levenshtein = true;
    bool beep_on_match = true;
    std::string tier_name = "medium";
};

struct ChatMessage {
    std::chrono::system_clock::time_point timestamp;
    std::string channel;
    std::string username;
    std::string content;
    std::string normalized_content;
};

// Levenshtein distance
int levenshteinDistance(const std::string& s1, const std::string& s2) {
    int m = s1.length();
    int n = s2.length();
    
    if (m == 0) return n;
    if (n == 0) return m;
    
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    
    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;
    
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i-1][j] + 1,
                dp[i][j-1] + 1,
                dp[i-1][j-1] + cost
            });
        }
    }
    
    return dp[m][n];
}

double calculateSimilarity(const std::string& s1, const std::string& s2) {
    if (s1.empty() && s2.empty()) return 1.0;
    if (s1.empty() || s2.empty()) return 0.0;
    
    int distance = levenshteinDistance(s1, s2);
    int maxLen = std::max(s1.length(), s2.length());
    return 1.0 - (static_cast<double>(distance) / maxLen);
}

std::string normalizeText(const std::string& text) {
    std::string result;
    result.reserve(text.length());
    
    bool lastWasSpace = true;
    for (char c : text) {
        if (std::isspace(c)) {
            if (!lastWasSpace) {
                result += ' ';
                lastWasSpace = true;
            }
        } else {
            result += std::toupper(c);
            lastWasSpace = false;
        }
    }
    
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

// Format time for terminal display
std::string format_time(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%H:%M:%S");
    return ss.str();
}

// Format timestamp for CSV
std::string format_timestamp(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

volatile bool running = true;

void signal_handler(int) {
    running = false;
}

// Build words list string for CSV header
std::string build_words_list(const std::unordered_set<std::string>& words) {
    std::stringstream ss;
    bool first = true;
    for (const auto& word : words) {
        if (!first) ss << ", ";
        ss << word;
        first = false;
    }
    return ss.str();
}

Config load_config(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open " << filename << std::endl;
        exit(1);
    }
    
    std::string line;
    std::string json_content;
    while (std::getline(file, line)) {
        json_content += line;
    }
    
    // Extract tier name
    size_t tier_pos = json_content.find("\"tier\"");
    if (tier_pos != std::string::npos) {
        size_t val_start = json_content.find("\"", tier_pos + 7);
        size_t val_end = json_content.find("\"", val_start + 1);
        if (val_start != std::string::npos && val_end != std::string::npos) {
            config.tier_name = json_content.substr(val_start + 1, val_end - val_start - 1);
        }
    }
    
    auto parse_array = [&](const std::string& key) -> std::unordered_set<std::string> {
        std::unordered_set<std::string> result;
        size_t key_pos = json_content.find("\"" + key + "\"");
        if (key_pos == std::string::npos) return result;
        
        size_t arr_start = json_content.find("[", key_pos);
        size_t arr_end = json_content.find("]", arr_start);
        if (arr_start == std::string::npos || arr_end == std::string::npos) return result;
        
        std::string arr_content = json_content.substr(arr_start + 1, arr_end - arr_start - 1);
        std::stringstream ss(arr_content);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(std::remove_if(item.begin(), item.end(), 
                [](char c) { return c == ' ' || c == '"' || c == '\t' || c == '\n' || c == '\r'; }), item.end());
            if (!item.empty()) {
                result.insert(item);
            }
        }
        return result;
    };
    
    config.positive_words = parse_array("positive_high_value");
    config.negative_words = parse_array("negative_high_value");
    
    // Parse settings
    auto parse_int = [&](const std::string& key, int default_val) -> int {
        size_t pos = json_content.find("\"" + key + "\"");
        if (pos == std::string::npos) return default_val;
        size_t num_start = json_content.find_first_of("0123456789", pos);
        if (num_start == std::string::npos) return default_val;
        try {
            return std::stoi(json_content.substr(num_start));
        } catch (...) {
            return default_val;
        }
    };
    
    auto parse_double = [&](const std::string& key, double default_val) -> double {
        size_t pos = json_content.find("\"" + key + "\"");
        if (pos == std::string::npos) return default_val;
        size_t num_start = json_content.find_first_of("0123456789.", pos);
        if (num_start == std::string::npos) return default_val;
        try {
            return std::stod(json_content.substr(num_start));
        } catch (...) {
            return default_val;
        }
    };
    
    auto parse_bool = [&](const std::string& key, bool default_val) -> bool {
        size_t pos = json_content.find("\"" + key + "\"");
        if (pos == std::string::npos) return default_val;
        if (json_content.find("true", pos) != std::string::npos) return true;
        if (json_content.find("false", pos) != std::string::npos) return false;
        return default_val;
    };
    
    config.burst_threshold = parse_int("burst_threshold", 3);
    config.burst_window_seconds = parse_int("burst_window_seconds", 30);
    config.context_window_seconds = parse_int("context_window_seconds", 120);
    config.levenshtein_threshold = parse_double("levenshtein_threshold", 0.8);
    config.use_levenshtein = parse_bool("use_levenshtein", true);
    config.beep_on_match = parse_bool("beep_on_match", true);
    
    return config;
}

class TwitchIRC {
private:
    int sockfd_ = -1;
    std::string channel_;
    std::string buffer_;
    int message_count_ = 0;
    bool connected_ = false;
    
    void send_raw(const std::string& msg) {
        std::string full_msg = msg + "\r\n";
        ::send(sockfd_, full_msg.c_str(), full_msg.length(), 0);
    }
    
    std::string receive_line() {
        while (true) {
            size_t pos = buffer_.find("\r\n");
            if (pos != std::string::npos) {
                std::string line = buffer_.substr(0, pos);
                buffer_.erase(0, pos + 2);
                return line;
            }
            
            char temp[4096];
            ssize_t received = recv(sockfd_, temp, sizeof(temp) - 1, 0);
            if (received <= 0) {
                connected_ = false;
                return "";
            }
            temp[received] = '\0';
            buffer_ += temp;
        }
    }
    
public:
    TwitchIRC() {}
    
    bool connect(const std::string& channel) {
        channel_ = channel;
        
        std::cout << "[IRC] Resolving irc.chat.twitch.tv..." << std::endl;
        
        struct hostent* server = gethostbyname("irc.chat.twitch.tv");
        if (!server) {
            std::cerr << "[IRC] ERROR: Failed to resolve" << std::endl;
            return false;
        }
        
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            std::cerr << "[IRC] ERROR: Failed to create socket" << std::endl;
            return false;
        }
        
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        serv_addr.sin_port = htons(6667);
        
        std::cout << "[IRC] Connecting to port 6667..." << std::endl;
        
        if (::connect(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "[IRC] ERROR: Connection failed" << std::endl;
            return false;
        }
        
        // Request Twitch capabilities
        std::cout << "[IRC] Requesting capabilities..." << std::endl;
        send_raw("CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership");
        
        // Anonymous login
        std::string nick = "justinfan" + std::to_string(10000 + (rand() % 90000));
        std::cout << "[IRC] Authenticating as " << nick << "..." << std::endl;
        
        send_raw("PASS oauth:blah");  // Dummy pass for anonymous
        send_raw("NICK " + nick);
        send_raw("USER " + nick + " 0 * :" + nick);
        
        // Wait for 001 (success) or error
        auto start = std::chrono::steady_clock::now();
        bool got_welcome = false;
        
        while (!got_welcome) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed > 10) {
                std::cerr << "[IRC] ERROR: Authentication timeout" << std::endl;
                return false;
            }
            
            std::string line = receive_line();
            if (line.empty()) {
                std::cerr << "[IRC] ERROR: Connection closed during auth" << std::endl;
                return false;
            }
            
            // Check for PING during auth
            if (line.find("PING") == 0) {
                std::string pong = "PONG" + line.substr(4);
                send_raw(pong);
                std::cout << "[IRC] PING -> PONG" << std::endl;
                continue;
            }
            
            // Check for welcome message
            if (line.find("001") != std::string::npos) {
                got_welcome = true;
                std::cout << "[IRC] ✓ Authentication successful" << std::endl;
            }
            
            // Check for auth failure
            if (line.find("Login authentication failed") != std::string::npos) {
                std::cerr << "[IRC] ERROR: Auth failed" << std::endl;
                return false;
            }
        }
        
        // Join channel
        std::cout << "[IRC] Joining #" << channel << "..." << std::endl;
        send_raw("JOIN #" + channel);
        
        // Wait for JOIN confirmation
        start = std::chrono::steady_clock::now();
        bool joined = false;
        
        while (!joined) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed > 5) {
                std::cerr << "[IRC] ERROR: Join timeout" << std::endl;
                return false;
            }
            
            std::string line = receive_line();
            if (line.empty()) {
                std::cerr << "[IRC] ERROR: Connection closed during join" << std::endl;
                return false;
            }
            
            // Handle PING
            if (line.find("PING") == 0) {
                std::string pong = "PONG" + line.substr(4);
                send_raw(pong);
                continue;
            }
            
            // Check for JOIN confirmation
            std::string expected_join = "JOIN #" + channel;
            if (line.find(expected_join) != std::string::npos) {
                joined = true;
                std::cout << "[IRC] ✓ Joined #" << channel << std::endl;
            }
            
            // Log other messages during join
            if (line.find("353") != std::string::npos) {
                std::cout << "[IRC] Received user list" << std::endl;
            }
            if (line.find("366") != std::string::npos) {
                std::cout << "[IRC] End of user list" << std::endl;
            }
        }
        
        connected_ = true;
        return true;
    }
    
    std::string read_line() {
        std::string line = receive_line();
        if (line.empty()) return "";
        
        // Handle PING
        if (line.find("PING") == 0) {
            std::string pong = "PONG" + line.substr(4);
            send_raw(pong);
            // Return empty and let caller call again
            return read_line();
        }
        
        message_count_++;
        return line;
    }
    
    bool is_connected() const {
        return connected_;
    }
    
    void disconnect() {
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
        connected_ = false;
    }
};

bool parse_message(const std::string& raw, ChatMessage& msg, const std::string& expected_channel) {
    size_t privmsg_pos = raw.find("PRIVMSG #" + expected_channel + " :");
    if (privmsg_pos == std::string::npos) return false;
    
    size_t user_start = raw.find(":");
    if (user_start == std::string::npos) return false;
    size_t user_end = raw.find("!", user_start);
    if (user_end == std::string::npos) return false;
    msg.username = raw.substr(user_start + 1, user_end - user_start - 1);
    
    size_t content_start = raw.find(" :", privmsg_pos);
    if (content_start == std::string::npos) return false;
    msg.content = raw.substr(content_start + 2);
    msg.normalized_content = normalizeText(msg.content);
    
    msg.channel = expected_channel;
    msg.timestamp = std::chrono::system_clock::now();
    
    return true;
}

bool check_match(const ChatMessage& msg, const Config& config, std::string& matched_word, std::string& sentiment) {
    const std::string& content = msg.normalized_content;
    
    for (const auto& word : config.positive_words) {
        std::string upper_word = normalizeText(word);
        
        if (upper_word.length() > 4 || !config.use_levenshtein) {
            if (content.find(upper_word) != std::string::npos) {
                matched_word = word;
                sentiment = "positive";
                return true;
            }
        } else {
            std::stringstream ss(content);
            std::string msg_word;
            while (ss >> msg_word) {
                double sim = calculateSimilarity(upper_word, msg_word);
                if (sim >= config.levenshtein_threshold) {
                    matched_word = word;
                    sentiment = "positive";
                    return true;
                }
            }
        }
    }
    
    for (const auto& word : config.negative_words) {
        std::string upper_word = normalizeText(word);
        
        if (upper_word.length() > 4 || !config.use_levenshtein) {
            if (content.find(upper_word) != std::string::npos) {
                matched_word = word;
                sentiment = "negative";
                return true;
            }
        } else {
            std::stringstream ss(content);
            std::string msg_word;
            while (ss >> msg_word) {
                double sim = calculateSimilarity(upper_word, msg_word);
                if (sim >= config.levenshtein_threshold) {
                    matched_word = word;
                    sentiment = "negative";
                    return true;
                }
            }
        }
    }
    
    return false;
}

class BurstDetector {
private:
    std::deque<std::pair<std::chrono::system_clock::time_point, std::string>> recent_matches_;
    int window_seconds_;
    int threshold_;
    bool use_levenshtein_;
    
public:
    BurstDetector(int window_sec, int threshold, bool use_lev) 
        : window_seconds_(window_sec), threshold_(threshold), use_levenshtein_(use_lev) {}
    
    int count_burst(const std::string& word, std::chrono::system_clock::time_point now) {
        auto cutoff = now - std::chrono::seconds(window_seconds_);
        while (!recent_matches_.empty() && recent_matches_.front().first < cutoff) {
            recent_matches_.pop_front();
        }
        
        int count = 1;
        std::string norm_word = normalizeText(word);
        
        for (const auto& entry : recent_matches_) {
            if (use_levenshtein_) {
                double sim = calculateSimilarity(norm_word, normalizeText(entry.second));
                if (sim >= 0.8) {
                    count++;
                }
            } else {
                if (norm_word == normalizeText(entry.second)) {
                    count++;
                }
            }
        }
        
        recent_matches_.push_back({now, word});
        return count;
    }
};

class LiveMonitor {
private:
    Config config_;
    std::deque<ChatMessage> context_buffer_;
    std::unordered_map<std::string, BurstDetector> burst_detectors_;
    std::ofstream log_file_;
    TwitchIRC connection_;
    std::string channel_;
    std::string creator_name_;
    int total_messages_ = 0;
    int total_matches_ = 0;
    int total_bursts_ = 0;
    std::string start_time_;
    
public:
    LiveMonitor(const Config& config, const std::string& channel) 
        : config_(config), channel_(channel), creator_name_(channel) {
        
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        start_time_ = format_timestamp(now);
        
        // Create directory structure: outputs/{channel}/{tier}/
        std::string dir_path = "outputs/" + channel + "/" + config.tier_name;
        std::string cmd = "mkdir -p " + dir_path;
        system(cmd.c_str());
        
        std::stringstream filename;
        filename << dir_path << "/matches-" << config.tier_name << "-" 
                 << std::put_time(std::localtime(&t), "%Y%m%d-%H%M%S") << ".csv";
        
        log_file_.open(filename.str());
        
        // Write CSV metadata header
        log_file_ << "# Chatomatic Live Monitor Log\n";
        log_file_ << "# Channel: #" << channel << "\n";
        log_file_ << "# Tier: " << config.tier_name << "\n";
        log_file_ << "# Started: " << start_time_ << "\n";
        log_file_ << "# Positive words: " << build_words_list(config.positive_words) << "\n";
        log_file_ << "# Negative words: " << build_words_list(config.negative_words) << "\n";
        log_file_ << "# Burst threshold: " << config.burst_threshold << " similar messages in " 
                   << config.burst_window_seconds << "s\n";
        log_file_ << "# Levenshtein: " << (config.use_levenshtein ? "ON" : "OFF") 
                   << " (threshold: " << config.levenshtein_threshold << ")\n";
        log_file_ << "#\n";
        log_file_ << "timestamp,channel,username,matched_word,sentiment,similar_count,content,context_start,context_end\n";
    }
    
    void set_creator_name(const std::string& name) {
        creator_name_ = name;
    }
    
    bool start() {
        std::cout << "=== Chatomatic Live Monitor ===" << std::endl;
        std::cout << "Loading config: " << config_.tier_name << "_value_words.json\n" << std::endl;
        
        std::cout << "[CONFIG] Tier: " << config_.tier_name << std::endl;
        std::cout << "[CONFIG] Loaded " << config_.positive_words.size() << " positive words" << std::endl;
        std::cout << "[CONFIG] Loaded " << config_.negative_words.size() << " negative words" << std::endl;
        std::cout << "[CONFIG] Burst threshold: " << config_.burst_threshold << " similar in " 
                  << config_.burst_window_seconds << "s" << std::endl;
        std::cout << "[CONFIG] Levenshtein: " << (config_.use_levenshtein ? "ON" : "OFF") << std::endl;
        std::cout << "[MONITOR] Output folder: outputs/" << channel_ << "/" << config_.tier_name << "/" << std::endl;
        std::cout << "[MONITOR] Tier: " << config_.tier_name << std::endl;
        std::cout << "[MONITOR] Levenshtein: " << (config_.use_levenshtein ? "ON" : "OFF") << std::endl;
        
        std::cout << "\n[MONITOR] Connecting to channel..." << std::endl;
        
        if (!connection_.connect(channel_)) {
            std::cerr << "[MONITOR] ❌ FAILED to connect to #" << channel_ << std::endl;
            return false;
        }
        
        std::cout << "[IRC] ✅ CONNECTED to #" << channel_ << std::endl;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  CHATOMATIC LIVE MONITOR ACTIVE" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Channel:  #" << channel_ << std::endl;
        std::cout << "Tier:     " << config_.tier_name << std::endl;
        std::cout << "Burst:    " << config_.burst_threshold << "+ similar messages\n" << std::endl;
        std::cout << "Status:   WATCHING CHAT..." << std::endl;
        std::cout << "\n[CHAT FLOW START]\n" << std::endl;
        
        return true;
    }
    
    void run() {
        int empty_count = 0;
        
        while (running) {
            std::string line = connection_.read_line();
            
            if (line.empty()) {
                empty_count++;
                // If we get 10 empty lines in a row, connection might be dead
                if (empty_count > 10) {
                    std::cerr << "\n[ERROR] Connection appears dead, exiting..." << std::endl;
                    running = false;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            empty_count = 0;
            
            ChatMessage msg;
            if (parse_message(line, msg, channel_)) {
                total_messages_++;
                
                // Print chat message to terminal
                std::cout << "[" << format_time(msg.timestamp) << "] " 
                          << msg.username << ": " << msg.content << std::endl;
                
                process_message(msg);
            }
            
            cleanup_context();
        }
    }
    
    void process_message(const ChatMessage& msg) {
        context_buffer_.push_back(msg);
        
        std::string matched_word, sentiment;
        if (check_match(msg, config_, matched_word, sentiment)) {
            total_matches_++;
            
            auto it = burst_detectors_.find(msg.channel);
            if (it == burst_detectors_.end()) {
                auto result = burst_detectors_.emplace(
                    msg.channel, 
                    BurstDetector(config_.burst_window_seconds, config_.burst_threshold, config_.use_levenshtein)
                );
                it = result.first;
            }
            
            int similar_count = it->second.count_burst(matched_word, msg.timestamp);
            
            if (similar_count >= config_.burst_threshold) {
                total_bursts_++;
                on_burst(msg, matched_word, sentiment, similar_count);
            } else {
                // Show match building up
                std::cout << "[MATCH: " << matched_word << "] Building burst (" 
                          << similar_count << "/" << config_.burst_threshold << ")" << std::endl;
            }
        }
    }
    
    void on_burst(const ChatMessage& msg, const std::string& word, 
                  const std::string& sentiment, int similar_count) {
        if (config_.beep_on_match) {
            std::cout << "\a";
        }
        
        auto context_start = msg.timestamp - std::chrono::seconds(config_.context_window_seconds);
        auto context_end = msg.timestamp + std::chrono::seconds(config_.context_window_seconds);
        
        // Compact burst display
        std::cout << "\n🎯 BURST DETECTED (" << similar_count << " similar) | " 
                  << word << " | " << format_time(context_start) << "-" << format_time(context_end) 
                  << "\n" << std::endl;
        
        // Log to CSV
        log_file_ << format_timestamp(msg.timestamp) << ","
                  << msg.channel << ","
                  << msg.username << ","
                  << word << ","
                  << sentiment << ","
                  << similar_count << ","
                  << "\"" << msg.content << "\"" << ","
                  << format_timestamp(context_start) << ","
                  << format_timestamp(context_end) << "\n";
        log_file_.flush();
    }
    
    void cleanup_context() {
        auto cutoff = std::chrono::system_clock::now() - 
                     std::chrono::seconds(config_.context_window_seconds * 2);
        
        while (!context_buffer_.empty() && context_buffer_.front().timestamp < cutoff) {
            context_buffer_.pop_front();
        }
    }
    
    void stop() {
        connection_.disconnect();
        log_file_.close();
        
        std::cout << "\n[CHAT FLOW END]" << std::endl;
        std::cout << "\n[STATS] Messages: " << total_messages_ 
                  << " | Matches: " << total_matches_ 
                  << " | Bursts: " << total_bursts_ << std::endl;
        std::cout << "[STOPPED] Check CSV file for logged matches." << std::endl;
    }
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " creator=\"/path/to/creator.json\"\n"
              << "\n"
              << "Creator Config Format (JSON):\n"
              << "  {\n"
              << "    \"creator_name\": \"theburntpeanut\",\n"
              << "    \"channel\": \"theburntpeanut\",\n"
              << "    \"enabled_tiers\": [\"high\", \"easy\"],\n"
              << "    \"tier_configs\": {\n"
              << "      \"high\": \"high_value_words.json\",\n"
              << "      \"medium\": \"medium_value_words.json\",\n"
              << "      \"easy\": \"easy_value_words.json\"\n"
              << "    }\n"
              << "  }\n"
              << "\n"
              << "Legacy (single tier mode):\n"
              << "  --tier=high     Use high_value_words.json (burst 3)\n"
              << "  --tier=medium   Use medium_value_words.json (burst 5) [default]\n"
              << "  --tier=easy     Use easy_value_words.json (burst 10)\n"
              << "\n"
              << "Examples:\n"
              << "  " << program_name << " creator=\"creators/theburntpeanut.json\"\n"
              << "  " << program_name << " creator=\"creators/shroud.json\"\n"
              << "  " << program_name << " --tier=medium     # Legacy mode\n";
}

// Creator config structure
struct CreatorConfig {
    std::string creator_name;
    std::string channel;
    std::vector<std::string> enabled_tiers;
    std::unordered_map<std::string, std::string> tier_config_files;
};

CreatorConfig load_creator_config(const std::string& filepath) {
    CreatorConfig creator;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open creator config: " << filepath << std::endl;
        exit(1);
    }
    
    std::string line;
    std::string json_content;
    while (std::getline(file, line)) {
        json_content += line;
    }
    
    // Parse creator_name
    auto extract_string = [&](const std::string& key) -> std::string {
        std::string search_key = "\"" + key + "\"";
        size_t pos = json_content.find(search_key);
        if (pos == std::string::npos) return "";
        size_t val_start = json_content.find("\"", pos + search_key.length());
        size_t val_end = json_content.find("\"", val_start + 1);
        if (val_start == std::string::npos || val_end == std::string::npos) return "";
        return json_content.substr(val_start + 1, val_end - val_start - 1);
    };
    
    creator.creator_name = extract_string("creator_name");
    creator.channel = extract_string("channel");
    
    // Parse enabled_tiers array
    size_t tiers_pos = json_content.find("\"enabled_tiers\"");
    if (tiers_pos != std::string::npos) {
        size_t arr_start = json_content.find("[", tiers_pos);
        size_t arr_end = json_content.find("]", arr_start);
        if (arr_start != std::string::npos && arr_end != std::string::npos) {
            std::string arr_content = json_content.substr(arr_start + 1, arr_end - arr_start - 1);
            std::stringstream ss(arr_content);
            std::string tier;
            while (std::getline(ss, tier, ',')) {
                tier.erase(std::remove_if(tier.begin(), tier.end(), 
                    [](char c) { return c == ' ' || c == '"' || c == '\t' || c == '\n' || c == '\r'; }), tier.end());
                if (!tier.empty()) {
                    creator.enabled_tiers.push_back(tier);
                }
            }
        }
    }
    
    // Parse tier_configs
    size_t configs_pos = json_content.find("\"tier_configs\"");
    if (configs_pos != std::string::npos) {
        size_t obj_start = json_content.find("{", configs_pos);
        size_t obj_end = json_content.find("}", obj_start);
        if (obj_start != std::string::npos && obj_end != std::string::npos) {
            std::string obj_content = json_content.substr(obj_start + 1, obj_end - obj_start - 1);
            
            // Parse each tier: "filename" pair
            std::string tiers[] = {"high", "medium", "easy"};
            for (const auto& tier : tiers) {
                std::string key = "\"" + tier + "\"";
                size_t key_pos = obj_content.find(key);
                if (key_pos != std::string::npos) {
                    size_t val_start = obj_content.find("\"", key_pos + key.length());
                    size_t val_end = obj_content.find("\"", val_start + 1);
                    if (val_start != std::string::npos && val_end != std::string::npos) {
                        std::string filename = obj_content.substr(val_start + 1, val_end - val_start - 1);
                        creator.tier_config_files[tier] = filename;
                    }
                }
            }
        }
    }
    
    // Set defaults if not found
    if (creator.creator_name.empty()) {
        creator.creator_name = "unknown";
    }
    if (creator.channel.empty()) {
        creator.channel = creator.creator_name;
    }
    if (creator.enabled_tiers.empty()) {
        creator.enabled_tiers = {"medium"};  // Default to medium tier
    }
    if (creator.tier_config_files.empty()) {
        creator.tier_config_files["high"] = "high_value_words.json";
        creator.tier_config_files["medium"] = "medium_value_words.json";
        creator.tier_config_files["easy"] = "easy_value_words.json";
    }
    
    return creator;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    srand(time(nullptr));
    
    std::string creator_path;
    std::string single_tier;
    bool use_creator_mode = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg.find("creator=") == 0) {
            creator_path = arg.substr(8);
            use_creator_mode = true;
        } else if (arg.find("--tier=") == 0) {
            single_tier = arg.substr(7);
        }
    }
    
    if (use_creator_mode) {
        // Creator mode: load config and run specified tiers
        CreatorConfig creator = load_creator_config(creator_path);
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  CREATOR MODE: " << creator.creator_name << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Channel: " << creator.channel << std::endl;
        std::cout << "Enabled tiers: ";
        for (size_t i = 0; i < creator.enabled_tiers.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << creator.enabled_tiers[i];
        }
        std::cout << "\n" << std::endl;
        
        // Run each enabled tier
        for (const auto& tier_name : creator.enabled_tiers) {
            auto it = creator.tier_config_files.find(tier_name);
            if (it == creator.tier_config_files.end()) {
                std::cerr << "WARNING: No config file for tier '" << tier_name << "', skipping..." << std::endl;
                continue;
            }
            
            std::string config_file = it->second;
            std::cout << "\n--- Starting tier: " << tier_name << " ---" << std::endl;
            std::cout << "Config: " << config_file << std::endl;
            
            Config config = load_config(config_file);
            
            LiveMonitor monitor(config, creator.channel);
            monitor.set_creator_name(creator.creator_name);
            
            if (!monitor.start()) {
                std::cerr << "Failed to start tier: " << tier_name << std::endl;
                continue;
            }
            
            monitor.run();
            monitor.stop();
            
            std::cout << "--- Tier " << tier_name << " complete ---\n" << std::endl;
        }
        
        std::cout << "All tiers complete for creator: " << creator.creator_name << std::endl;
        
    } else {
        // Legacy single-tier mode
        if (single_tier.empty()) {
            single_tier = "medium";  // default
        }
        
        if (single_tier != "high" && single_tier != "medium" && single_tier != "easy") {
            std::cerr << "ERROR: Invalid tier '" << single_tier << "'" << std::endl;
            return 1;
        }
        
        std::string config_file = single_tier + "_value_words.json";
        Config config = load_config(config_file);
        
        LiveMonitor monitor(config, "theburntpeanut");
        
        if (!monitor.start()) {
            return 1;
        }
        
        monitor.run();
        monitor.stop();
    }
    
    return 0;
}
