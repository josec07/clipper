#include "catch_amalgamated.hpp"
#include "core/config.h"
#include <fstream>
#include <filesystem>

using namespace ctic::core;

TEST_CASE("ConfigManager construction", "[config]") {
    ConfigManager mgr;
    REQUIRE(mgr.get_ctic_dir() == ".ctic");
}

TEST_CASE("ConfigManager ensure_ctic_dir", "[config]") {
    ConfigManager mgr;
    
    SECTION("creates directory") {
        REQUIRE(mgr.ensure_ctic_dir());
    }
}

TEST_CASE("ConfigManager save_load_creator", "[config]") {
    ConfigManager mgr;
    mgr.ensure_ctic_dir();
    
    CreatorConfig config;
    config.name = "test_creator";
    config.channel = "testcreator";
    config.twitch_url = "https://twitch.tv/testcreator";
    config.enabled_tiers = {"high", "medium"};
    config.detector_config_id = "default";
    config.created_at = "2026-01-01T00:00:00Z";
    
    SECTION("save and load round-trip") {
        REQUIRE(mgr.save_creator(config));
        REQUIRE(mgr.creator_exists("test_creator"));
        
        CreatorConfig loaded = mgr.load_creator("test_creator");
        REQUIRE(loaded.name == "test_creator");
        REQUIRE(loaded.channel == "testcreator");
        REQUIRE(loaded.twitch_url == "https://twitch.tv/testcreator");
        REQUIRE(loaded.enabled_tiers.size() == 2);
        REQUIRE(loaded.detector_config_id == "default");
        
        mgr.remove_creator("test_creator");
    }
}

TEST_CASE("ConfigManager list_creators", "[config]") {
    ConfigManager mgr;
    mgr.ensure_ctic_dir();
    
    CreatorConfig c1, c2;
    c1.name = "list_test_1";
    c1.channel = "listtest1";
    c1.twitch_url = "https://twitch.tv/listtest1";
    
    c2.name = "list_test_2";
    c2.channel = "listtest2";
    c2.twitch_url = "https://twitch.tv/listtest2";
    
    mgr.save_creator(c1);
    mgr.save_creator(c2);
    
    SECTION("lists all creators") {
        auto creators = mgr.list_creators();
        REQUIRE(std::find(creators.begin(), creators.end(), "list_test_1") != creators.end());
        REQUIRE(std::find(creators.begin(), creators.end(), "list_test_2") != creators.end());
    }
    
    mgr.remove_creator("list_test_1");
    mgr.remove_creator("list_test_2");
}

TEST_CASE("ConfigManager remove_creator", "[config]") {
    ConfigManager mgr;
    mgr.ensure_ctic_dir();
    
    CreatorConfig config;
    config.name = "remove_test";
    config.channel = "removetest";
    config.twitch_url = "https://twitch.tv/removetest";
    
    mgr.save_creator(config);
    REQUIRE(mgr.creator_exists("remove_test"));
    
    SECTION("removes creator") {
        REQUIRE(mgr.remove_creator("remove_test"));
        REQUIRE_FALSE(mgr.creator_exists("remove_test"));
    }
}

TEST_CASE("ConfigManager creator_exists", "[config]") {
    ConfigManager mgr;
    mgr.ensure_ctic_dir();
    
    SECTION("non-existent returns false") {
        REQUIRE_FALSE(mgr.creator_exists("nonexistent_creator_xyz"));
    }
}

TEST_CASE("format_timestamp", "[config]") {
    auto now = std::chrono::system_clock::now();
    std::string ts = format_timestamp(now);
    
    SECTION("ISO 8601 format") {
        REQUIRE(ts.find("T") != std::string::npos);
        REQUIRE(ts.find("Z") != std::string::npos);
        REQUIRE(ts.length() >= 19);
    }
}

TEST_CASE("ConfigManager load_tier_config", "[config]") {
    ConfigManager mgr;
    
    SECTION("high tier defaults") {
        TierConfig config = mgr.load_tier_config("high");
        REQUIRE(config.tier_name == "high");
        REQUIRE(config.burst_threshold == 3);
        REQUIRE_FALSE(config.words.empty());
    }
    
    SECTION("medium tier defaults") {
        TierConfig config = mgr.load_tier_config("medium");
        REQUIRE(config.tier_name == "medium");
        REQUIRE(config.burst_threshold == 8);
        REQUIRE(config.min_word_length == 1);
        REQUIRE(config.cooldown_seconds == 90);
        REQUIRE(config.require_unique_users == 4);
    }
    
    SECTION("easy tier defaults") {
        TierConfig config = mgr.load_tier_config("easy");
        REQUIRE(config.tier_name == "easy");
        REQUIRE(config.burst_threshold == 15);
        REQUIRE(config.min_word_length == 2);
        REQUIRE(config.cooldown_seconds == 120);
        REQUIRE(config.require_unique_users == 5);
    }
}
