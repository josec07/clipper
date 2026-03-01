#include "catch_amalgamated.hpp"
#include "providers/twitch_url.h"

using namespace ctic::providers;

TEST_CASE("extract_channel_from_url", "[url]") {
    SECTION("valid URL") {
        REQUIRE(extract_channel_from_url("https://twitch.tv/xqc") == "xqc");
        REQUIRE(extract_channel_from_url("https://twitch.tv/thEburntpeanut") == "thEburntpeanut");
        REQUIRE(extract_channel_from_url("https://www.twitch.tv/shroud") == "shroud");
    }
    
    SECTION("invalid URL") {
        REQUIRE(extract_channel_from_url("") == "");
        REQUIRE(extract_channel_from_url("https://youtube.com/xqc") == "");
        REQUIRE(extract_channel_from_url("not a url") == "");
        REQUIRE(extract_channel_from_url("https://twitch.tv/") == "");
    }
    
    SECTION("with underscore in name") {
        REQUIRE(extract_channel_from_url("https://twitch.tv/pokimane") == "pokimane");
        REQUIRE(extract_channel_from_url("https://twitch.tv/user_name_123") == "user_name_123");
    }
    
    SECTION("case preserved from URL") {
        REQUIRE(extract_channel_from_url("https://twitch.tv/XQC") == "XQC");
        REQUIRE(extract_channel_from_url("https://twitch.tv/ShRoUd") == "ShRoUd");
    }
}

TEST_CASE("normalize_channel", "[url]") {
    SECTION("converts to lowercase") {
        REQUIRE(normalize_channel("XQC") == "xqc");
        REQUIRE(normalize_channel("TheBurntPeanut") == "theburntpeanut");
    }
    
    SECTION("removes hash prefix") {
        REQUIRE(normalize_channel("#xqc") == "xqc");
        REQUIRE(normalize_channel("#TheBurntPeanut") == "theburntpeanut");
    }
    
    SECTION("already normalized") {
        REQUIRE(normalize_channel("xqc") == "xqc");
        REQUIRE(normalize_channel("shroud") == "shroud");
    }
    
    SECTION("empty string") {
        REQUIRE(normalize_channel("") == "");
    }
}

TEST_CASE("is_valid_twitch_url", "[url]") {
    SECTION("valid URLs") {
        REQUIRE(is_valid_twitch_url("https://twitch.tv/xqc"));
        REQUIRE(is_valid_twitch_url("https://www.twitch.tv/xqc"));
    }
    
    SECTION("invalid URLs") {
        REQUIRE_FALSE(is_valid_twitch_url(""));
        REQUIRE_FALSE(is_valid_twitch_url("https://youtube.com/xqc"));
        REQUIRE_FALSE(is_valid_twitch_url("not a url"));
    }
}

TEST_CASE("parse_url_or_channel", "[url]") {
    SECTION("URL input") {
        REQUIRE(parse_url_or_channel("https://twitch.tv/xqc") == "xqc");
        REQUIRE(parse_url_or_channel("https://www.twitch.tv/ShRoUd") == "ShRoUd");
    }
    
    SECTION("channel name input") {
        REQUIRE(parse_url_or_channel("XQC") == "xqc");
        REQUIRE(parse_url_or_channel("#xqc") == "xqc");
        REQUIRE(parse_url_or_channel("TheBurntPeanut") == "theburntpeanut");
    }
    
    SECTION("empty input") {
        REQUIRE(parse_url_or_channel("") == "");
    }
}
