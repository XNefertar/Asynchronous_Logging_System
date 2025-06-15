#pragma once
#include <atomic>
#include <mutex>
#include <iostream>
#include <stdexcept>

namespace ConfigSpace {

enum class LogFormat {
    TEXT = 0,
    HTML = 1
};

class ConfigManager {
private:
    // 只声明，不定义
    static ConfigManager* _instance;
    static std::mutex _mutex;
    
    std::atomic<LogFormat> _logFormat{LogFormat::TEXT};
    std::atomic<bool> _initialized{false};
    
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
public:
    static ConfigManager* getInstance();
    void initFromUserInput();
    void setLogFormat(LogFormat format);
    LogFormat getLogFormat() const;
    bool isTextFormat() const;
    bool isHtmlFormat() const;
    std::string getFileExtension() const;
    std::string getLogFileName() const;
    std::string getFormatString() const;
    bool isInitialized() const;
    static void cleanup();
};

} // namespace ConfigSpace