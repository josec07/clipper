#include "catch_amalgamated.hpp"
#include "core/text.h"

using namespace ctic::core;
using Catch::Approx;

TEST_CASE("levenshtein_distance", "[text]") {
    SECTION("empty strings") {
        REQUIRE(levenshtein_distance("", "") == 0);
        REQUIRE(levenshtein_distance("hello", "") == 5);
        REQUIRE(levenshtein_distance("", "world") == 5);
    }
    
    SECTION("identical strings") {
        REQUIRE(levenshtein_distance("hello", "hello") == 0);
        REQUIRE(levenshtein_distance("POG", "POG") == 0);
    }
    
    SECTION("single character difference") {
        REQUIRE(levenshtein_distance("cat", "bat") == 1);
        REQUIRE(levenshtein_distance("cat", "cats") == 1);
    }
    
    SECTION("known cases") {
        REQUIRE(levenshtein_distance("kitten", "sitting") == 3);
        REQUIRE(levenshtein_distance("saturday", "sunday") == 3);
    }
}

TEST_CASE("calculate_similarity", "[text]") {
    SECTION("100% similarity") {
        REQUIRE(calculate_similarity("POG", "POG") == Approx(1.0));
        REQUIRE(calculate_similarity("INSANE", "INSANE") == Approx(1.0));
    }
    
    SECTION("0% similarity") {
        REQUIRE(calculate_similarity("", "POG") == Approx(0.0));
        REQUIRE(calculate_similarity("POG", "") == Approx(0.0));
    }
    
    SECTION("partial similarity") {
        double sim = calculate_similarity("POG", "POGGG");
        REQUIRE(sim >= 0.5);
        REQUIRE(sim < 1.0);
    }
    
    SECTION("case insensitive via normalize") {
        std::string norm1 = normalize_text("POG");
        std::string norm2 = normalize_text("pog");
        REQUIRE(calculate_similarity(norm1, norm2) == Approx(1.0));
    }
}

TEST_CASE("normalize_text", "[text]") {
    SECTION("converts to uppercase") {
        REQUIRE(normalize_text("hello") == "HELLO");
        REQUIRE(normalize_text("POG") == "POG");
        REQUIRE(normalize_text("InSaNe") == "INSANE");
    }
    
    SECTION("collapses whitespace") {
        REQUIRE(normalize_text("hello  world") == "HELLO WORLD");
        REQUIRE(normalize_text("  hello   world  ") == "HELLO WORLD");
    }
    
    SECTION("handles empty string") {
        REQUIRE(normalize_text("") == "");
    }
    
    SECTION("removes leading/trailing spaces") {
        REQUIRE(normalize_text("  hello  ") == "HELLO");
    }
}

TEST_CASE("word_matches", "[text]") {
    SECTION("exact match in message") {
        REQUIRE(word_matches("INSANE", "that was INSANE moment"));
        REQUIRE(word_matches("POG", "POG moment"));
    }
    
    SECTION("long word substring match") {
        REQUIRE(word_matches("POGCHAMP", "look at that POGCHAMP emote"));
        REQUIRE_FALSE(word_matches("POGCHAMP", "nothing here"));
    }
    
    SECTION("short word fuzzy match") {
        REQUIRE(word_matches("W", "W W W W W"));
    }
    
    SECTION("no match") {
        REQUIRE_FALSE(word_matches("INSANE", "nothing matches here"));
        REQUIRE_FALSE(word_matches("POG", "hello world"));
    }
    
    SECTION("case insensitive") {
        REQUIRE(word_matches("pog", "POG MOMENT", 0.8));
        REQUIRE(word_matches("Pog", "pog moment", 0.8));
    }
    
    SECTION("repeated chars match") {
        REQUIRE(word_matches("W", "WWWW"));
        REQUIRE(word_matches("W", "W W W W"));
        REQUIRE(word_matches("L", "LOL"));
    }
    
    SECTION("substring with repeated chars") {
        REQUIRE(word_matches("W", "WWW POG"));
        REQUIRE(word_matches("POG", "POGGERS"));
    }
}

TEST_CASE("collapse_repeated_chars", "[text]") {
    SECTION("no repeated chars") {
        REQUIRE(collapse_repeated_chars("ABC") == "ABC");
        REQUIRE(collapse_repeated_chars("POG") == "POG");
    }
    
    SECTION("repeated chars collapsed") {
        REQUIRE(collapse_repeated_chars("WWWW") == "W");
        REQUIRE(collapse_repeated_chars("AAAA") == "A");
        REQUIRE(collapse_repeated_chars("WWWAAA") == "WA");
    }
    
    SECTION("empty string") {
        REQUIRE(collapse_repeated_chars("") == "");
    }
    
    SECTION("all same char") {
        REQUIRE(collapse_repeated_chars("LLLLLLLLL") == "L");
    }
    
    SECTION("mixed case preserved") {
        REQUIRE(collapse_repeated_chars("WWWppp") == "Wp");
    }
}

TEST_CASE("contains_word", "[text]") {
    SECTION("word found") {
        REQUIRE(contains_word("W", "WWWW"));
        REQUIRE(contains_word("POG", "POGGERS"));
        REQUIRE(contains_word("INSANE", "that was INSANE"));
    }
    
    SECTION("word not found") {
        REQUIRE_FALSE(contains_word("POG", "hello world"));
        REQUIRE_FALSE(contains_word("W", "ABC"));
    }
    
    SECTION("case insensitive") {
        REQUIRE(contains_word("pog", "POG"));
        REQUIRE(contains_word("W", "w"));
    }
    
    SECTION("empty needle") {
        REQUIRE(contains_word("", "anything"));
    }
}
