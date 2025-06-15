#include "ConfigManager.hpp"

namespace ConfigSpace {

// 在.cpp文件中定义静态成员
ConfigManager* ConfigManager::_instance = nullptr;
std::mutex ConfigManager::_mutex;

ConfigManager* ConfigManager::getInstance() {
    if (_instance == nullptr) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_instance == nullptr) {
            _instance = new ConfigManager();
        }
    }
    return _instance;
}

void ConfigManager::initFromUserInput() {
    int option = 0;
    std::cout << "请选择日志选项: " << std::endl;
    std::cout << "0. 普通文件(.txt)" << std::endl;
    std::cout << "1. HTML文件(.html)" << std::endl;
    std::cin >> option;
    
    if (option < 0 || option > 1) {
        throw std::invalid_argument("无效的选项, 请选择0或1。");
    }
    
    setLogFormat(static_cast<LogFormat>(option));
    _initialized.store(true);
    
    std::cout << "配置已设置: " << getFormatString() << std::endl;
}

void ConfigManager::setLogFormat(LogFormat format) {
    _logFormat.store(format, std::memory_order_release);
}

LogFormat ConfigManager::getLogFormat() const {
    return _logFormat.load(std::memory_order_acquire);
}

bool ConfigManager::isTextFormat() const {
    return getLogFormat() == LogFormat::TEXT;
}

bool ConfigManager::isHtmlFormat() const {
    return getLogFormat() == LogFormat::HTML;
}

std::string ConfigManager::getFileExtension() const {
    return isHtmlFormat() ? ".html" : ".txt";
}

std::string ConfigManager::getLogFileName() const {
    return isHtmlFormat() ? "log.html" : "log.txt";
}

std::string ConfigManager::getFormatString() const {
    return isHtmlFormat() ? "HTML" : "TEXT";
}

bool ConfigManager::isInitialized() const {
    return _initialized.load(std::memory_order_acquire);
}

void ConfigManager::cleanup() {
    std::lock_guard<std::mutex> lock(_mutex);
    delete _instance;
    _instance = nullptr;
}

} // namespace ConfigSpace