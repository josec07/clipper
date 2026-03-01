#include "../../include/core/config.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <dirent.h>
#include <algorithm>

namespace ctic {
namespace core {

ConfigManager::ConfigManager() {
    ctic_dir_ = ".ctic";
}

bool ConfigManager::ensure_ctic_dir() {
    std::string cmd = "mkdir -p " + ctic_dir_ + "/creators " + ctic_dir_ + "/outputs " + ctic_dir_ + "/detectors " + ctic_dir_ + "/profiles";
    return system(cmd.c_str()) == 0;
}

std::string ConfigManager::get_creators_dir() {
    return ctic_dir_ + "/creators";
}

std::string ConfigManager::get_profiles_dir() {
    return ctic_dir_ + "/profiles";
}

CreatorConfig ConfigManager::load_creator(const std::string& name) {
    CreatorConfig config;
    std::string filepath = ctic_dir_ + "/creators/" + name + ".json";
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return config;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    auto parse_string = [&](const std::string& key) -> std::string {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        size_t start = content.find("\"", pos + key.length() + 3);
        if (start == std::string::npos) return "";
        size_t end = content.find("\"", start + 1);
        return content.substr(start + 1, end - start - 1);
    };
    
    auto parse_int = [&](const std::string& key, int default_val) -> int {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return default_val;
        size_t num_start = content.find_first_of("0123456789", pos);
        if (num_start == std::string::npos) return default_val;
        try {
            return std::stoi(content.substr(num_start));
        } catch (...) {
            return default_val;
        }
    };
    
    config.name = name;
    config.channel = parse_string("channel");
    config.twitch_url = parse_string("twitch_url");
    config.profile = parse_string("profile");
    if (config.profile.empty()) config.profile = "balanced";
    config.detector_config_id = parse_string("detector_config");
    config.created_at = parse_string("created_at");
    config.last_monitored = parse_string("last_monitored");
    config.total_sessions = parse_int("total_sessions", 0);
    config.total_clips_detected = parse_int("total_clips_detected", 0);
    
    size_t tiers_pos = content.find("\"enabled_tiers\"");
    if (tiers_pos != std::string::npos) {
        size_t array_start = content.find("[", tiers_pos);
        size_t array_end = content.find("]", array_start);
        if (array_start != std::string::npos && array_end != std::string::npos) {
            std::string array_content = content.substr(array_start, array_end - array_start + 1);
            size_t pos = 0;
            while ((pos = array_content.find("\"", pos)) != std::string::npos) {
                size_t end = array_content.find("\"", pos + 1);
                if (end != std::string::npos) {
                    config.enabled_tiers.push_back(array_content.substr(pos + 1, end - pos - 1));
                    pos = end + 1;
                } else {
                    break;
                }
            }
        }
    }
    
    return config;
}

bool ConfigManager::save_creator(const CreatorConfig& config) {
    ensure_ctic_dir();
    
    std::string filepath = ctic_dir_ + "/creators/" + config.name + ".json";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    file << "  \"name\": \"" << config.name << "\",\n";
    file << "  \"channel\": \"" << config.channel << "\",\n";
    file << "  \"twitch_url\": \"" << config.twitch_url << "\",\n";
    file << "  \"profile\": \"" << config.profile << "\",\n";
    file << "  \"enabled_tiers\": [";
    for (size_t i = 0; i < config.enabled_tiers.size(); ++i) {
        file << "\"" << config.enabled_tiers[i] << "\"";
        if (i < config.enabled_tiers.size() - 1) file << ", ";
    }
    file << "],\n";
    file << "  \"detector_config\": \"" << config.detector_config_id << "\",\n";
    file << "  \"created_at\": \"" << config.created_at << "\",\n";
    file << "  \"last_monitored\": \"" << config.last_monitored << "\",\n";
    file << "  \"total_sessions\": " << config.total_sessions << ",\n";
    file << "  \"total_clips_detected\": " << config.total_clips_detected << "\n";
    file << "}\n";
    
    file.close();
    return true;
}

bool ConfigManager::remove_creator(const std::string& name) {
    std::string filepath = ctic_dir_ + "/creators/" + name + ".json";
    return std::remove(filepath.c_str()) == 0;
}

std::vector<std::string> ConfigManager::list_creators() {
    std::vector<std::string> creators;
    std::string creators_dir = ctic_dir_ + "/creators";
    
    DIR* dir = opendir(creators_dir.c_str());
    if (!dir) {
        return creators;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 5 && filename.substr(filename.length() - 5) == ".json") {
            creators.push_back(filename.substr(0, filename.length() - 5));
        }
    }
    
    closedir(dir);
    std::sort(creators.begin(), creators.end());
    return creators;
}

bool ConfigManager::creator_exists(const std::string& name) {
    std::string filepath = ctic_dir_ + "/creators/" + name + ".json";
    std::ifstream file(filepath);
    return file.good();
}

std::string ConfigManager::get_output_dir(const std::string& creator, const std::string& tier) {
    return ctic_dir_ + "/outputs/" + creator + "/" + tier;
}

TierConfig ConfigManager::load_tier_config(const std::string& tier_name) {
    TierConfig config;
    config.tier_name = tier_name;
    config.window_seconds = 30;
    config.levenshtein_threshold = 0.8;
    config.use_levenshtein = true;
    
    if (tier_name == "high") {
        config.words = {
            "POG", "POGGERS", "POGCHAMP", "INSANE", "LETS GO", "CLUTCH", "ACE", "PENTA",
            "CRACKED", "GOATED", "DIFF", "FINAL", "K", "GOD", "CRITICAL", "MONSTER",
            "LEGENDARY", "NUTS", "WTF", "OMFG", "SHEESH", "DAMN", "WHOA", "NO WAY",
            "YOOO", "GOAT", "FINAL BOSS", "200IQ", "BIG BRAIN", "ONE TAP", "SPEEDRUN",
            "WORLD RECORD", "HES HIM", "DIFFERENT BREED", "ABSOLUTE UNIT", "CINEMATIC",
            "MOVIE", "THEATRE", "MAIN CHARACTER", "PROTAGONIST", "HIM", "HERO", "GOATED"
        };
        config.burst_threshold = 3;
        config.min_word_length = 3;
        config.cooldown_seconds = 60;
        config.require_unique_users = 2;
    } else if (tier_name == "high-negative") {
        config.words = {
            "L", "LMAO", "LFMAO", "RIP", "F", "F IN CHAT", "LOST", "BOT", "DOG",
            "TRASH", "CRINGE", "OMEGALUL", "HUHH", "AIM ASSIST", "WORST", "FAILED",
            "CHOKE", "BRUH", "NOT LIKE THIS", "NOT THE WAY", "NOOO", "YIKES",
            "EMBARRASSING", "OOF", "SKILL ISSUE", "REPORT", "UNINSTALL", "FIX YOUR GAME",
            "LITERALLY UNPLAYABLE", "WHAT WAS THAT", "INTING", "THROWING", "GRIEFING",
            "NPC", "HARDSTUCK", "BOOSTED", "CARRIED", "BAD", "TERRIBLE", "HORRIBLE"
        };
        config.burst_threshold = 5;
        config.min_word_length = 1;
        config.cooldown_seconds = 45;
        config.require_unique_users = 3;
    } else if (tier_name == "medium") {
        config.words = {
            "W", "GG", "GGS", "EZ", "NICE", "SHEESH", "DAMN", "OH", "YT", "PEPE",
            "MONKA", "KEKW", "BASED", "TRUE", "REAL", "MOGGED", "OWNED", "SAUCE",
            "CLEAN", "NASTY", "P", "VP", "POGU", "POGGIES", "KEKL", "KEKWAIT",
            "MONKAGUN", "PEPELA", "FEELSMAN", "SAVAGE", "HEAT", "ON FIRE", "COOKING",
            "LETHAL", "DEADLY", "VICIOUS", "CRUEL", "UNFAIR", "UNMATCHED", "INHUMAN"
        };
        config.burst_threshold = 8;
        config.min_word_length = 1;
        config.cooldown_seconds = 90;
        config.require_unique_users = 4;
    } else if (tier_name == "easy") {
        config.words = {
            "lol", "wow", "true", "real", "?", "??", "xd", "lmao", "ok", "sure",
            "yeah", "no", "yes", "ok", "hmm", "oof", "rip", "loll", "lool", "lmaoo",
            "bruh", "bro", "man", "dude", "fr", "for real", "actually", "literally",
            "honestly", "probably", "maybe", "fr fr", "no cap", "no cap fr", "bet",
            "say less", "facts", "fax", "printer", "slaps", "hard", "valid", "fair"
        };
        config.burst_threshold = 15;
        config.min_word_length = 2;
        config.cooldown_seconds = 120;
        config.require_unique_users = 5;
    }
    
    return config;
}

TierConfig ConfigManager::load_profile_tier(const std::string& profile_name, const std::string& tier_name) {
    TierConfig base_config = load_tier_config(tier_name);
    
    std::string profile_path = ctic_dir_ + "/profiles/" + profile_name + ".json";
    std::ifstream file(profile_path);
    if (!file.is_open()) {
        return base_config;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    size_t tier_pos = content.find("\"" + tier_name + "\"");
    if (tier_pos == std::string::npos) {
        return base_config;
    }
    
    size_t block_start = content.find("{", tier_pos);
    size_t block_end = content.find("}", block_start);
    if (block_start == std::string::npos || block_end == std::string::npos) {
        return base_config;
    }
    
    std::string block = content.substr(block_start, block_end - block_start + 1);
    
    auto parse_int_override = [&block](const std::string& key, int default_val) -> int {
        size_t pos = block.find("\"" + key + "\"");
        if (pos == std::string::npos) return default_val;
        size_t num_start = block.find_first_of("0123456789", pos);
        if (num_start == std::string::npos) return default_val;
        try {
            return std::stoi(block.substr(num_start));
        } catch (...) {
            return default_val;
        }
    };
    
    base_config.burst_threshold = parse_int_override("burst_threshold", base_config.burst_threshold);
    base_config.min_word_length = parse_int_override("min_word_length", base_config.min_word_length);
    base_config.cooldown_seconds = parse_int_override("cooldown_seconds", base_config.cooldown_seconds);
    base_config.require_unique_users = parse_int_override("require_unique_users", base_config.require_unique_users);
    
    return base_config;
}

DetectorConfig ConfigManager::load_detector_config(const std::string& detector_id) {
    DetectorConfig config;
    config.id = detector_id;
    config.name = "Default Levenshtein Burst";
    config.algorithm = "levenshtein";
    config.similarity_threshold = 0.8;
    
    config.tiers = {
        load_tier_config("high"),
        load_tier_config("high-negative"),
        load_tier_config("medium"),
        load_tier_config("easy")
    };
    
    return config;
}

std::string format_timestamp(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

}
}
