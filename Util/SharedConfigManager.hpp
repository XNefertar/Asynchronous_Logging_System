#pragma once

#include <string>
#include <atomic>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <ctime>

namespace ConfigSpace {

// 前向声明
struct SharedConfig;

class SharedConfigManager {
private:
    static const char* SEM_NAME;
    static const int SHM_KEY = 0x12345678;
    
    int _shmid;
    SharedConfig* _sharedData;
    // 使用void*避免包含系统头文件
    void* _semaphore;
    bool _isCreator;

    static std::atomic<bool> _shutdownRequested;
    static SharedConfigManager* _instance;
    
    // 私有构造函数
    SharedConfigManager();
    ~SharedConfigManager();
    
    // 禁止拷贝和赋值
    SharedConfigManager(const SharedConfigManager&) = delete;
    SharedConfigManager& operator=(const SharedConfigManager&) = delete;
    
    // 私有初始化方法
    void initializeSharedMemory();
    void initializeSemaphore();
    void cleanup();

    // 信号处理函数和进程检测
    static void signalHandler(int signal);
    void setupSignalHandlers();
    bool isInitializerProcessAlive() const;
    void cleanupStaleConfig();
    
public:
    // 获取单例实例
    static SharedConfigManager& getInstance();
    
    // 配置初始化
    void initFromUserInput();
    
    // 等待配置初始化
    bool waitForInitialization(int timeoutSeconds = 30);
    
    // 状态查询
    bool isInitialized() const;
    int getLogFormat() const;
    bool isTextFormat() const;
    bool isHtmlFormat() const;
    std::string getFormatString() const;
    std::string getLogFileName() const;
    
    // 调试和管理
    void printStatus() const;
    static void forceCleanup();

    // 关闭函数
    static void requestShutdown();
    static bool isShutdownRequested();
};

// RAII 包装器, 确保资源清理
class SharedConfigRAII {
private:
    SharedConfigManager& _manager;
    
public:
    SharedConfigRAII() : _manager(SharedConfigManager::getInstance()) 
    {}
    
    ~SharedConfigRAII() {
        // 析构时确保清理
        if (SharedConfigManager::isShutdownRequested()) {
            SharedConfigManager::forceCleanup();
        }
    }
    
    SharedConfigManager& get() { return _manager; }
    const SharedConfigManager& get() const { return _manager; }
};

} // namespace ConfigSpace