#include "../../include/core/config.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <ctime>

namespace ctic {
namespace core {

ConfigManager::ConfigManager() {
    ctic_dir_ = ".ctic";
}

bool ConfigManager::ensure_ctic_dir() {
    std::string cmd = "mkdir -p " + ctic_dir_ + "/creators " + ctic_dir_ + "/outputs " + ctic_dir_ + "/detectors";
    return system(cmd.c_str()) == 0;
}

std::string ConfigManager::get_creators_dir() {
    return ctic_dir_ + "/creators";
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
    std::string cmd = "ls " + ctic_dir_ + "/creators/*.json 2>/dev/null | xargs -n1 basename | sed 's/.json$//'";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string name(buffer);
            if (!name.empty() && name.back() == '\n') {
                name.pop_back();
            }
            if (!name.empty()) {
                creators.push_back(name);
            }
        }
        pclose(pipe);
    }
    
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
    
    if (tier_name == "high") {
        config.words = {"INSANE", "CLUTCH", "POGCHAMP", "ACE", "PENTA"};
        config.burst_threshold = 3;
    } else if (tier_name == "medium") {
        config.words = {"W", "POG", "GGS", "NICE", "LETS GO"};
        config.burst_threshold = 5;
    } else if (tier_name == "easy") {
        config.words = {"lol", "wow", "nice", "gg", "crazy"};
        config.burst_threshold = 10;
    }
    
    return config;
}

DetectorConfig ConfigManager::load_detector_config(const std::string& detector_id) {
    DetectorConfig config;
    config.id = detector_id;
    config.name = "Default Levenshtein Burst";
    config.algorithm = "levenshtein";
    config.similarity_threshold = 0.8;
    
    config.tiers = {
        load_tier_config("high"),
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
