#pragma once

#include "monitor.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <functional>

namespace ctic {
namespace core {

struct MonitorState {
    std::string creator_name;
    bool running = false;
    bool connected = false;
    int pid = 0;
    std::string started_at;
    int messages_processed = 0;
    int bursts_detected = 0;
};

class MonitorPool {
private:
    std::unordered_map<std::string, std::thread> threads_;
    std::unordered_map<std::string, std::shared_ptr<Monitor>> monitors_;
    std::unordered_map<std::string, MonitorState> states_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_requested_ = false;
    
    ConfigManager config_mgr_;
    
    void monitorThread(const std::string& creator_name);
    void updateState(const std::string& creator_name, const MonitorState& state);
    void saveState();
    void loadState();
    
public:
    MonitorPool();
    ~MonitorPool();
    
    void addCreator(const std::string& creator_name);
    void removeCreator(const std::string& creator_name);
    
    void startAll();
    void stopAll();
    void stopCreator(const std::string& creator_name);
    
    MonitorState getState(const std::string& creator_name);
    std::vector<MonitorState> getAllStates();
    
    bool isRunning(const std::string& creator_name);
    
    void waitForShutdown();
};

}
}
