#include "catch_amalgamated.hpp"
#include "core/detection.h"
#include "core/text.h"
#include <chrono>

using namespace ctic::core;

TEST_CASE("BurstDetector construction", "[detector]") {
    SECTION("default parameters") {
        BurstDetector detector;
        REQUIRE(detector.unique_users() == 0);
    }
    
    SECTION("custom parameters") {
        BurstDetector detector(60, 5, 0.8, true);
        REQUIRE(detector.unique_users() == 0);
    }
}

TEST_CASE("BurstDetector count_burst", "[detector]") {
    BurstDetector detector(30, 3, 0.8, true);
    auto now = std::chrono::system_clock::now();
    
    SECTION("count increases") {
        int count1 = detector.count_burst("POG", "user1", "POG", now);
        REQUIRE(count1 >= 1);
        
        auto later = now + std::chrono::seconds(1);
        int count2 = detector.count_burst("POG", "user2", "POG", later);
        REQUIRE(count2 > count1);
    }
}

TEST_CASE("BurstDetector unique_users", "[detector]") {
    BurstDetector detector(30, 3, 0.8, true);
    auto now = std::chrono::system_clock::now();
    
    detector.count_burst("POG", "user1", "POG", now);
    detector.count_burst("POG", "user2", "POG", now);
    detector.count_burst("POG", "user1", "POG", now);
    
    SECTION("counts unique usernames") {
        REQUIRE(detector.unique_users() == 2);
    }
}

TEST_CASE("BurstDetector reset", "[detector]") {
    BurstDetector detector(30, 3, 0.8, true);
    auto now = std::chrono::system_clock::now();
    
    detector.count_burst("POG", "user1", "POG", now);
    detector.count_burst("POG", "user2", "POG", now);
    
    REQUIRE(detector.unique_users() > 0);
    
    detector.reset();
    
    SECTION("reset clears state") {
        REQUIRE(detector.unique_users() == 0);
    }
}

TEST_CASE("Detector construction", "[detector]") {
    DetectionConfig config;
    config.positive_words = {"W", "POG", "GG"};
    config.burst_threshold = 3;
    
    Detector detector(config);
    
    REQUIRE(detector.total_matches() == 0);
    REQUIRE(detector.total_bursts() == 0);
}

TEST_CASE("Detector process_message", "[detector]") {
    DetectionConfig config;
    config.positive_words = {"W", "POG", "INSANE"};
    config.burst_threshold = 2;
    config.use_levenshtein = true;
    
    Detector detector(config);
    auto now = std::chrono::system_clock::now();
    
    SECTION("no match returns empty") {
        BurstResult result = detector.process_message("user1", "hello world", now);
        REQUIRE_FALSE(result.detected);
        REQUIRE(result.matched_word.empty());
    }
    
    SECTION("match but no burst") {
        BurstResult result = detector.process_message("user1", "W", now);
        REQUIRE_FALSE(result.detected);
        REQUIRE(result.matched_word == "W");
        REQUIRE(detector.total_matches() == 1);
    }
    
    SECTION("burst detected at threshold") {
        detector.process_message("user1", "W", now);
        auto result = detector.process_message("user2", "W", now + std::chrono::milliseconds(100));
        REQUIRE(result.detected);
        REQUIRE(result.burst_count >= 2);
        REQUIRE(detector.total_bursts() >= 1);
    }
    
    SECTION("cooldown prevents immediate re-detection") {
        detector.process_message("user1", "W", now);
        detector.process_message("user2", "W", now + std::chrono::milliseconds(100));
        
        auto result3 = detector.process_message("user3", "W", now + std::chrono::milliseconds(200));
        REQUIRE_FALSE(result3.detected);
    }
}

TEST_CASE("Detector check_match", "[detector]") {
    DetectionConfig config;
    config.positive_words = {"INSANE", "POG"};
    config.negative_words = {"L", "LOST"};
    
    Detector detector(config);
    
    std::string matched_word, sentiment;
    
    SECTION("positive match") {
        REQUIRE(detector.check_match("that was INSANE", matched_word, sentiment));
        REQUIRE(matched_word == "INSANE");
        REQUIRE(sentiment == "positive");
    }
    
    SECTION("negative match") {
        REQUIRE(detector.check_match("what an L", matched_word, sentiment));
        REQUIRE(matched_word == "L");
        REQUIRE(sentiment == "negative");
    }
    
    SECTION("no match") {
        REQUIRE_FALSE(detector.check_match("hello world", matched_word, sentiment));
    }
}

TEST_CASE("Detector Levenshtein matching", "[detector]") {
    DetectionConfig config;
    config.positive_words = {"W"};
    config.burst_threshold = 3;
    config.use_levenshtein = true;
    config.levenshtein_threshold = 0.8;
    
    Detector detector(config);
    auto now = std::chrono::system_clock::now();
    
    SECTION("fuzzy match for short word") {
        BurstResult result = detector.process_message("user1", "W", now);
        REQUIRE(result.matched_word == "W");
    }
    
    SECTION("exact match also works") {
        BurstResult result = detector.process_message("user1", "W", now);
        REQUIRE(result.matched_word == "W");
    }
}

TEST_CASE("Detector threshold behavior", "[detector]") {
    DetectionConfig config;
    config.positive_words = {"POG"};
    config.burst_threshold = 5;
    config.burst_window_seconds = 30;
    
    Detector detector(config);
    auto now = std::chrono::system_clock::now();
    
    SECTION("below threshold - no burst") {
        for (int i = 0; i < 4; i++) {
            detector.process_message("user" + std::to_string(i), "POG", now + std::chrono::milliseconds(i * 100));
        }
        REQUIRE(detector.total_matches() == 4);
        REQUIRE(detector.total_bursts() == 0);
    }
    
    SECTION("at threshold - burst detected") {
        for (int i = 0; i < 5; i++) {
            detector.process_message("user" + std::to_string(i), "POG", now + std::chrono::milliseconds(i * 100));
        }
        REQUIRE(detector.total_bursts() >= 1);
    }
}
