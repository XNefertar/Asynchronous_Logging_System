#include "SharedConfigManager.hpp"

#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <ctime>
#include <csignal>

namespace ConfigSpace {

// 共享内存中的配置数据结构定义
struct SharedConfig {
    volatile int logFormat;          // 0=TEXT, 1=HTML
    volatile bool initialized;       // 是否已初始化
    volatile pid_t initializer_pid;  // 初始化进程的PID
    volatile time_t init_timestamp;  // 初始化时间戳
    char reserved[112];              // 预留空间, 总共128字节
    
    SharedConfig() : logFormat(0), initialized(false), initializer_pid(0) {
        memset(reserved, 0, sizeof(reserved));
    }
};

// 静态成员定义
const char* SharedConfigManager::SEM_NAME = "/async_log_config_sem";
std::atomic<bool> SharedConfigManager::_shutdownRequested{false};
SharedConfigManager* SharedConfigManager::_instance = nullptr;

// 信号处理函数
void SharedConfigManager::signalHandler(int signal) {
    std::cout << "\n[SharedConfig] 收到信号 " << signal << ", 准备优雅关闭..." << std::endl;
    _shutdownRequested.store(true);
    
    // 立即清理共享内存资源
    if (_instance) {
        _instance->cleanup();
    }
    forceCleanup();

    // 恢复默认信号处理并重新发送信号
    // std::signal(signal, SIG_DFL);
    // std::raise(signal);

    // 使用 exit() 而不是 raise()，这样会调用析构函数
    std::cout << "[SharedConfig] 清理完成，正在退出..." << std::endl;
    std::exit(0);  // 正常退出，会调用全局对象的析构函数
}

// 设置信号处理器
void SharedConfigManager::setupSignalHandlers() {
    std::signal(SIGINT, signalHandler);   // Ctrl+C
    std::signal(SIGTERM, signalHandler);  // terminate
    std::signal(SIGQUIT, signalHandler);  // Ctrl+\
    std::signal(SIGHUP, signalHandler);   // 终端断开
}

// 检查初始化进程是否还活着
bool SharedConfigManager::isInitializerProcessAlive() const {
    if (!_sharedData || !_sharedData->initialized) {
        return false;
    }
    
    pid_t pid = _sharedData->initializer_pid;
    if (pid <= 0) {
        return false;
    }
    
    // 使用 kill(pid, 0) 检查进程是否存在
    return kill(pid, 0) == 0;
}

// 清理过期配置
void SharedConfigManager::cleanupStaleConfig() {
    if (!_sharedData) return;
    
    if (_sharedData->initialized && !isInitializerProcessAlive()) {
        std::cout << "[SharedConfig] 检测到初始化进程 " << _sharedData->initializer_pid 
                  << " 已退出, 清理过期配置" << std::endl;
        
        // 重置共享内存状态
        _sharedData->initialized = false;
        _sharedData->initializer_pid = 0;
        _sharedData->logFormat = 0;
        _sharedData->init_timestamp = 0;
        
        std::cout << "[SharedConfig] 过期配置已清理" << std::endl;
    }
}

// 构造函数
SharedConfigManager::SharedConfigManager() 
    : _shmid(-1), _sharedData(nullptr), _semaphore(nullptr), _isCreator(false) {
    setupSignalHandlers();  // 设置信号处理器
    initializeSharedMemory();
    initializeSemaphore();
}

// 析构函数
SharedConfigManager::~SharedConfigManager() {
    cleanup();
}

// 获取单例实例
SharedConfigManager& SharedConfigManager::getInstance() {
    static SharedConfigManager instance;
    return instance;
}

// 初始化共享内存
void SharedConfigManager::initializeSharedMemory() {
    const int SHM_SIZE = sizeof(SharedConfig);
    
    // 尝试创建新的共享内存段
    _shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    
    if (_shmid != -1) {
        // 成功创建, 说明是第一个进程
        _isCreator = true;
        std::cout << "[SharedConfig] 创建新的共享内存段, ID: " << _shmid << std::endl;
    } else {
        // 创建失败, 尝试连接到已存在的共享内存段
        _shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
        if (_shmid == -1) {
            throw std::runtime_error("无法创建或连接到共享内存: " + std::string(strerror(errno)));
        }
        _isCreator = false;
        std::cout << "[SharedConfig] 连接到已存在的共享内存段, ID: " << _shmid << std::endl;
    }
    
    // 将共享内存段附加到当前进程的地址空间
    _sharedData = static_cast<SharedConfig*>(shmat(_shmid, nullptr, 0));
    if (_sharedData == reinterpret_cast<SharedConfig*>(-1)) {
        throw std::runtime_error("无法附加到共享内存: " + std::string(strerror(errno)));
    }
    
    // 如果是创建者, 初始化共享数据结构
    if (_isCreator) {
        new (_sharedData) SharedConfig();  // placement new
        std::cout << "[SharedConfig] 初始化共享数据结构" << std::endl;
    }
}

// 初始化信号量
void SharedConfigManager::initializeSemaphore() {
    // 如果是创建者, 先尝试删除可能存在的旧信号量
    if (_isCreator) {
        sem_unlink(SEM_NAME);  // 忽略错误
    }
    
    // 创建或打开命名信号量
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        throw std::runtime_error("无法创建信号量: " + std::string(strerror(errno)));
    }
    
    _semaphore = static_cast<void*>(sem);
    std::cout << "[SharedConfig] 信号量初始化成功" << std::endl;
}

// 清理资源
void SharedConfigManager::cleanup() {
    // 分离共享内存
    if (_sharedData != nullptr) {
        if (shmdt(_sharedData) == -1) {
            std::cerr << "[SharedConfig] 分离共享内存失败: " << strerror(errno) << std::endl;
        }
        _sharedData = nullptr;
    }
    
    // 关闭信号量
    if (_semaphore != nullptr) {
        sem_t* sem = static_cast<sem_t*>(_semaphore);
        if (sem_close(sem) == -1) {
            std::cerr << "[SharedConfig] 关闭信号量失败: " << strerror(errno) << std::endl;
        }
        _semaphore = nullptr;
    }
    
    // 如果是创建者进程, 负责清理资源
    if (_isCreator) {
        // 删除共享内存段
        if (_shmid != -1) {
            if (shmctl(_shmid, IPC_RMID, nullptr) == -1) {
                std::cerr << "[SharedConfig] 删除共享内存段失败: " << strerror(errno) << std::endl;
            } else {
                std::cout << "[SharedConfig] 共享内存段已删除" << std::endl;
            }
        }
        
        // 删除信号量
        if (sem_unlink(SEM_NAME) == -1) {
            std::cerr << "[SharedConfig] 删除信号量失败: " << strerror(errno) << std::endl;
        } else {
            std::cout << "[SharedConfig] 信号量已删除" << std::endl;
        }
    }
}


// 从用户输入初始化配置
void SharedConfigManager::initFromUserInput() {
    sem_t* sem = static_cast<sem_t*>(_semaphore);
    
    // 获取信号量锁
    if (sem_wait(sem) == -1) {
        throw std::runtime_error("获取信号量失败: " + std::string(strerror(errno)));
    }
    
    try {
        // 检查并清理过期配置
        cleanupStaleConfig();
        
        // 检查是否已经初始化
        if (_sharedData->initialized && isInitializerProcessAlive()) {
            std::cout << "[SharedConfig] 配置已由进程 " << _sharedData->initializer_pid 
                     << " 初始化, 当前格式: " << (_sharedData->logFormat == 1 ? "HTML" : "TEXT") << std::endl;
            sem_post(sem);
            return;
        }
        
        // 进行初始化
        int option = 0;
        std::cout << "请选择日志选项: " << std::endl;
        std::cout << "0. 普通文件(.txt)" << std::endl;
        std::cout << "1. HTML文件(.html)" << std::endl;
        std::cin >> option;
        
        if (option < 0 || option > 1) {
            sem_post(sem);
            throw std::invalid_argument("无效的选项, 请选择0或1。");
        }
        std::cout << "[SharedConfig] 用户选择的日志格式: " 
                  << (option == 1 ? "HTML" : "TEXT") << std::endl;
        
        // 写入共享内存
        _sharedData->logFormat = option;
        _sharedData->initializer_pid = getpid();
        _sharedData->init_timestamp = time(nullptr);
        
        std::cout << "[SharedConfig] logFormat 设置为: " << _sharedData->logFormat << std::endl;
        
        // 使用内存屏障确保写入顺序
        __sync_synchronize();
        
        _sharedData->initialized = true;
        
        std::cout << "[SharedConfig] 配置已设置: " << (option == 1 ? "HTML" : "TEXT") 
                 << " (PID: " << getpid() << ")" << std::endl;
        
    } catch (...) {
        sem_post(sem);
        throw;
    }
    
    // 释放信号量
    sem_post(sem);
}

// 等待配置初始化完成
bool SharedConfigManager::waitForInitialization(int timeoutSeconds) {
    time_t startTime = time(nullptr);
    
    while (!isInitialized()) {
        if (_shutdownRequested.load()) {
            std::cout << "[SharedConfig] 收到关闭请求, 停止等待" << std::endl;
            return false;
        }
        
        if (time(nullptr) - startTime > timeoutSeconds) {
            std::cerr << "[SharedConfig] 等待配置初始化超时" << std::endl;
            return false;
        }
        
        std::cout << "[SharedConfig] 等待配置初始化..." << std::endl;
        usleep(500000);  // 500ms
    }
    
    return true;
}

// 检查是否已初始化
bool SharedConfigManager::isInitialized() const {
    if (!_sharedData) return false;
    
    // 使用内存屏障确保读取的是最新值
    __sync_synchronize();
    
    // 如果显示已初始化, 但初始化进程已死, 则认为未初始化
    if (_sharedData->initialized && !isInitializerProcessAlive()) {
        const_cast<SharedConfigManager*>(this)->cleanupStaleConfig();
        return false;
    }
    
    return _sharedData->initialized;
}

// 获取日志格式
int SharedConfigManager::getLogFormat() const {
    if (!isInitialized()) {
        throw std::runtime_error("配置尚未初始化");
    }
    
    return _sharedData->logFormat;
}

// 检查是否为TEXT格式
bool SharedConfigManager::isTextFormat() const {
    return getLogFormat() == 0;
}

// 检查是否为HTML格式
bool SharedConfigManager::isHtmlFormat() const {
    return getLogFormat() == 1;
}

// 获取格式字符串
std::string SharedConfigManager::getFormatString() const {
    return isHtmlFormat() ? "HTML" : "TEXT";
}

// 获取日志文件名
std::string SharedConfigManager::getLogFileName() const {
    return isHtmlFormat() ? "log.html" : "log.txt";
}

// 打印配置状态
void SharedConfigManager::printStatus() const {
    sem_t* sem = static_cast<sem_t*>(_semaphore);
    
    if (sem_wait(sem) == -1) {
        std::cerr << "[SharedConfig] 获取状态失败" << std::endl;
        return;
    }
    
    std::cout << "=== 共享配置状态 ===" << std::endl;
    std::cout << "共享内存ID: " << _shmid << std::endl;
    std::cout << "当前进程PID: " << getpid() << std::endl;
    std::cout << "是否为创建者: " << (_isCreator ? "是" : "否") << std::endl;
    
    if (_sharedData->initialized) {
        std::cout << "配置状态: 已初始化" << std::endl;
        std::cout << "初始化进程PID: " << _sharedData->initializer_pid << std::endl;
        std::cout << "日志格式: " << (_sharedData->logFormat == 1 ? "HTML" : "TEXT") << std::endl;
    } else {
        std::cout << "配置状态: 未初始化" << std::endl;
    }
    std::cout << "===================" << std::endl;
    
    sem_post(sem);
}

// 强制清理资源
void SharedConfigManager::forceCleanup() {
    // 删除共享内存段
    int shmid = shmget(SHM_KEY, 0, 0);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, nullptr);
    }
    
    // 删除信号量
    sem_unlink(SEM_NAME);
    
    std::cout << "[SharedConfig] 强制清理完成" << std::endl;
}

void SharedConfigManager::requestShutdown() {
    _shutdownRequested.store(true);
    std::cout << "[SharedConfig] 请求关闭" << std::endl;
}

bool SharedConfigManager::isShutdownRequested(){
    return _shutdownRequested.load();
}

} // namespace ConfigSpace