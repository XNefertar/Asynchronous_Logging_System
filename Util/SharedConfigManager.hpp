#pragma once

#include <string>

namespace ConfigSpace {

// 前向声明
struct SharedConfig;

class SharedConfigManager {
private:
    static const char* SEM_NAME;
    static const int SHM_KEY = 0x12345678;
    
    int shmid_;
    SharedConfig* sharedData_;
    void* semaphore_;  // 使用void*避免包含系统头文件
    bool isCreator_;
    
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
};

} // namespace ConfigSpace