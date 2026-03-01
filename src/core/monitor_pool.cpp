#include "../../include/core/monitor_pool.h"
#include "../../include/providers/twitch_irc.h"
#include <iostream>
#include <fstream>
#include <csignal>

namespace ctic {
namespace core {

MonitorPool::MonitorPool() {
    loadState();
}

MonitorPool::~MonitorPool() {
    stopAll();
}

void MonitorPool::addCreator(const std::string& creator_name) {
    if (threads_.find(creator_name) != threads_.end()) {
        return;
    }
    
    auto monitor = std::make_shared<Monitor>(creator_name, config_mgr_);
    monitors_[creator_name] = monitor;
    
    MonitorState state;
    state.creator_name = creator_name;
    state.running = false;
    states_[creator_name] = state;
}

void MonitorPool::removeCreator(const std::string& creator_name) {
    stopCreator(creator_name);
    
    monitors_.erase(creator_name);
    states_.erase(creator_name);
}

void MonitorPool::startAll() {
    shutdown_requested_ = false;
    
    for (auto& [name, monitor] : monitors_) {
        threads_[name] = std::thread(&MonitorPool::monitorThread, this, name);
    }
    
    saveState();
}

void MonitorPool::stopAll() {
    shutdown_requested_ = true;
    cv_.notify_all();
    
    for (auto& [name, monitor] : monitors_) {
        if (monitor) {
            monitor->stop();
        }
    }
    
    for (auto& [name, thread] : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    threads_.clear();
    saveState();
}

void MonitorPool::stopCreator(const std::string& creator_name) {
    auto it = monitors_.find(creator_name);
    if (it != monitors_.end() && it->second) {
        it->second->stop();
    }
    
    auto thread_it = threads_.find(creator_name);
    if (thread_it != threads_.end() && thread_it->second.joinable()) {
        thread_it->second.join();
        threads_.erase(thread_it);
    }
    
    if (states_.find(creator_name) != states_.end()) {
        states_[creator_name].running = false;
    }
    
    saveState();
}

void MonitorPool::monitorThread(const std::string& creator_name) {
    auto creator_config = config_mgr_.load_creator(creator_name);
    
    MonitorState state;
    state.creator_name = creator_name;
    state.running = true;
    state.connected = false;
    state.started_at = format_timestamp(std::chrono::system_clock::now());
    updateState(creator_name, state);
    
    std::cout << "[THREAD:" << creator_name << "] Starting monitor thread" << std::endl;
    
    providers::TwitchIRC irc;
    
    if (!irc.connect(creator_config.channel)) {
        std::cerr << "[THREAD:" << creator_name << "] Failed to connect" << std::endl;
        state.running = false;
        state.connected = false;
        updateState(creator_name, state);
        return;
    }
    
    state.connected = true;
    updateState(creator_name, state);
    
    std::cout << "[THREAD:" << creator_name << "] Connected to #" << creator_config.channel << std::endl;
    
    auto& monitor = monitors_[creator_name];
    
    while (!shutdown_requested_ && monitor->isRunning() && irc.is_connected()) {
        std::string line = irc.read_line();
        if (line.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        std::string username, content;
        if (irc.parse_message(line, username, content)) {
            monitor->processMessage(username, content);
            
            state.messages_processed = monitor->totalMessages();
            state.bursts_detected = monitor->totalBursts();
            updateState(creator_name, state);
        }
    }
    
    irc.disconnect();
    monitor->saveCreatorStats(config_mgr_);
    
    state.running = false;
    state.connected = false;
    updateState(creator_name, state);
    
    std::cout << "[THREAD:" << creator_name << "] Stopped. "
              << "Messages: " << state.messages_processed 
              << ", Bursts: " << state.bursts_detected << std::endl;
}

void MonitorPool::updateState(const std::string& creator_name, const MonitorState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    states_[creator_name] = state;
}

MonitorState MonitorPool::getState(const std::string& creator_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return states_[creator_name];
}

std::vector<MonitorState> MonitorPool::getAllStates() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MonitorState> result;
    for (const auto& [name, state] : states_) {
        result.push_back(state);
    }
    return result;
}

bool MonitorPool::isRunning(const std::string& creator_name) {
    auto it = states_.find(creator_name);
    return it != states_.end() && it->second.running;
}

void MonitorPool::saveState() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    config_mgr_.ensure_ctic_dir();
    std::ofstream file(".ctic/state.json");
    if (!file.is_open()) return;
    
    file << "{\n";
    bool first = true;
    for (const auto& [name, state] : states_) {
        if (!first) file << ",\n";
        first = false;
        
        file << "  \"" << name << "\": {\n";
        file << "    \"creator_name\": \"" << state.creator_name << "\",\n";
        file << "    \"running\": " << (state.running ? "true" : "false") << ",\n";
        file << "    \"connected\": " << (state.connected ? "true" : "false") << ",\n";
        file << "    \"pid\": " << state.pid << ",\n";
        file << "    \"started_at\": \"" << state.started_at << "\",\n";
        file << "    \"messages_processed\": " << state.messages_processed << ",\n";
        file << "    \"bursts_detected\": " << state.bursts_detected << "\n";
        file << "  }";
    }
    file << "\n}\n";
}

void MonitorPool::loadState() {
    std::ifstream file(".ctic/state.json");
    if (!file.is_open()) return;
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    size_t pos = 0;
    while ((pos = content.find("\"creator_name\"", pos)) != std::string::npos) {
        size_t name_start = content.find("\"", pos + 15);
        if (name_start == std::string::npos) break;
        size_t name_end = content.find("\"", name_start + 1);
        if (name_end == std::string::npos) break;
        
        std::string name = content.substr(name_start + 1, name_end - name_start - 1);
        
        MonitorState state;
        state.creator_name = name;
        state.running = false;
        state.connected = false;
        state.started_at = "";
        state.messages_processed = 0;
        state.bursts_detected = 0;
        
        size_t block_end = content.find("}", pos);
        if (block_end != std::string::npos) {
            std::string block = content.substr(pos, block_end - pos);
            
            auto parse_int = [&block](const std::string& key) -> int {
                size_t key_pos = block.find("\"" + key + "\"");
                if (key_pos == std::string::npos) return 0;
                size_t num_start = block.find_first_of("0123456789", key_pos);
                if (num_start == std::string::npos) return 0;
                try {
                    return std::stoi(block.substr(num_start));
                } catch (...) {
                    return 0;
                }
            };
            
            state.messages_processed = parse_int("messages_processed");
            state.bursts_detected = parse_int("bursts_detected");
        }
        
        states_[name] = state;
        pos = block_end + 1;
    }
}

void MonitorPool::waitForShutdown() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return shutdown_requested_; });
}

}
}
