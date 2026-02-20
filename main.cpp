#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "chat_types.h"
#include "chat_buffer.h"
#include "spike_detector.h"
#include "keyword_matcher.h"
#include "scoring_engine.h"
#include "clip_detector.h"

using namespace chatclipper;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  --stdin              Read chat messages from stdin (default)\n"
              << "  --file <path>        Read chat messages from file\n"
              << "  --config <path>      Load configuration from JSON file\n"
              << "  --test               Run built-in tests\n"
              << "  --help               Show this help message\n"
              << "\nInput format (pipe-delimited):\n"
              << "  timestamp_ms|username|message\n"
              << "\nExample:\n"
              << "  1700000000000|user1|W\n"
              << "  1700000000100|user2|PogChamp\n"
              << "\nOutput format (JSON):\n"
              << "  {\"start_ms\":123,\"end_ms\":456,\"score\":0.85,\"category\":\"win\",\"keywords\":[\"W\",\"PogChamp\"]}\n";
}

void setupDefaultKeywords(KeywordMatcher& matcher) {
    // Win keywords
    KeywordCategory wins;
    wins.name = "wins";
    wins.keywords = {"W", "pog", "clutch", "insane", "ez", "gg", "wp"};
    wins.weight = 1.0;
    matcher.addCategory(wins);
    
    // Loss keywords
    KeywordCategory losses;
    losses.name = "losses";
    losses.keywords = {"L", "ff", "sad", "rip", "unlucky", "oof"};
    losses.weight = 1.0;
    matcher.addCategory(losses);
    
    // Hype keywords
    KeywordCategory hype;
    hype.name = "hype";
    hype.keywords = {"OMEGALUL", "KEKW", "PogChamp", "monkaS", "OMEGALUL", "LUL"};
    hype.weight = 0.8;
    matcher.addCategory(hype);
}

void printClipEvent(const ClipEvent& event) {
    std::cout << "{"
              << "\"start_ms\":" << event.start_time.count() << ","
              << "\"end_ms\":" << event.end_time.count() << ","
              << "\"score\":" << std::fixed << std::setprecision(2) << event.score << ","
              << "\"category\":\"" << event.category << "\","
              << "\"keywords\":[";
    
    for (size_t i = 0; i < event.keywords.size(); ++i) {
        if (i > 0) std::cout << ",";
        std::cout << "\"" << event.keywords[i] << "\"";
    }
    
    std::cout << "]}" << std::endl;
}

void processStream(std::istream& input, ClipDetector& detector, bool skipInvalid) {
    std::string line;
    while (std::getline(input, line)) {
        std::stringstream ss(line);
        std::string timestamp_str, username, content;
        
        if (std::getline(ss, timestamp_str, '|') &&
            std::getline(ss, username, '|') &&
            std::getline(ss, content)) {
            try {
                int64_t timestamp = std::stoll(timestamp_str);
                ChatMessage msg;
                msg.timestamp = std::chrono::milliseconds(timestamp);
                msg.username = username;
                msg.content = content;

                detector.processMessage(msg);
            } catch (const std::invalid_argument& e){
                if(skipInvalid) continue;
                std::cerr << "Invalid timestamp" << timestamp_str << std::endl;
            } catch (const std::out_of_range& e){
                if(skipInvalid) continue;
                std::cerr << "Timestamp out of range: " << timestamp_str << std::endl;
            }
        } else {
            if(skipInvalid) continue;
            std::cerr << "Invalid line format: " << line << std::endl;
        }
    }
}

void runTests() {
    std::cout << "Running ChatClipper tests...\n\n";
    
    // Test 1: Levenshtein Distance
    std::cout << "Test 1: Levenshtein Distance\n";
    KeywordMatcher matcher;
    std::string t1 = "W";
    std::string t2 = "WW";
    double sim1 = matcher.calculateSimilarity(t1,t2); // Will be calculated via match
    std::cout << "  Similarity 'L' vs 'L': " << sim1 << "\n\n";
    
    // Test 2: Spike Detection
    std::cout << "Test 2: Spike Detection\n";
    SpikeDetector detector;
    for (int i = 0; i < 50; ++i) {
        detector.addSample(5.0);  // Baseline: 5 msg/sec
    }
    detector.addSample(25.0);  // Spike: 25 msg/sec
    std::cout << "  Baseline: " << detector.getBaseline() << " msg/sec\n";
    std::cout << "  Is spike: " << (detector.isSpike() ? "yes" : "no") << "\n";
    std::cout << "  Intensity: " << detector.getSpikeIntensity() << "\n\n";
    
    // Test 3: Keyword Matching
    std::cout << "Test 3: Keyword Matching\n";
    setupDefaultKeywords(matcher);
    auto matches = matcher.match("WWW");
    std::cout << "  Matches for 'WWW': " << matches.size() << "\n\n";
    
    // Test 4: Chat Buffer
    std::cout << "Test 4: Chat Buffer\n";
    ChatBuffer buffer(std::chrono::seconds(60));
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    
    for (int i = 0; i < 10; ++i) {
        ChatMessage msg;
        msg.timestamp = now + std::chrono::milliseconds(i * 100);
        msg.username = "user" + std::to_string(i);
        msg.content = "test message";
        buffer.addMessage(msg);
    }
    
    std::cout << "  Buffer size: " << buffer.size() << "\n";
    std::cout << "  Message rate: " << buffer.getMessageRate(std::chrono::seconds(10)) << " msg/sec\n\n";
    
    std::cout << "All tests completed!\n";
}

int main(int argc, char* argv[]) {
    bool use_stdin = true;
    std::string input_file;
    std::string config_file;
    bool run_tests = false;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--stdin") {
            use_stdin = true;
        } else if (arg == "--file" && i + 1 < argc) {
            use_stdin = false;
            input_file = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "--test") {
            run_tests = true;
        }
    }
    
    if (run_tests) {
        runTests();
        return 0;
    }
    
    // Setup components
    KeywordMatcher keyword_matcher(0.8);
    setupDefaultKeywords(keyword_matcher);
    
    ScoringConfig scoring_config;
    scoring_config.weights = {0.4, 0.5, 0.1};
    scoring_config.min_clip_score = 0.7;
    ScoringEngine scoring_engine(scoring_config);
    
    ClipDetectorConfig clip_config;
    clip_config.clip_duration = std::chrono::seconds(60);
    clip_config.lookback = std::chrono::seconds(30);
    clip_config.cooldown = std::chrono::seconds(30);
    clip_config.min_score = 0.7;
    
    ClipDetector detector(clip_config, &keyword_matcher, &scoring_engine);
    
    // Set up clip callback
    detector.onClip([](const ClipEvent& event) {
        printClipEvent(event);
    });
    
    // Process input
    if (use_stdin) {
        processStream(std::cin, detector, true);
    } else if (!input_file.empty()) {
        std::ifstream file(input_file);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << input_file << std::endl;
            return 1;
        }
        processStream(file, detector, true);
    }
    
    return 0;
}
