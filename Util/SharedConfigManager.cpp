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

namespace ConfigSpace {

// 共享内存中的配置数据结构定义
struct SharedConfig {
    volatile int logFormat;          // 0=TEXT, 1=HTML
    volatile bool initialized;       // 是否已初始化
    volatile pid_t initializer_pid;  // 初始化进程的PID
    char reserved[116];              // 预留空间, 总共128字节
    
    SharedConfig() : logFormat(0), initialized(false), initializer_pid(0) {
        memset(reserved, 0, sizeof(reserved));
    }
};

// 静态成员定义
const char* SharedConfigManager::SEM_NAME = "/async_log_config_sem";

// 构造函数
SharedConfigManager::SharedConfigManager() 
    : shmid_(-1), sharedData_(nullptr), semaphore_(nullptr), isCreator_(false) {
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
    shmid_ = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    
    if (shmid_ != -1) {
        // 成功创建，说明是第一个进程
        isCreator_ = true;
        std::cout << "[SharedConfig] 创建新的共享内存段，ID: " << shmid_ << std::endl;
    } else {
        // 创建失败，尝试连接到已存在的共享内存段
        shmid_ = shmget(SHM_KEY, SHM_SIZE, 0666);
        if (shmid_ == -1) {
            throw std::runtime_error("无法创建或连接到共享内存: " + std::string(strerror(errno)));
        }
        isCreator_ = false;
        std::cout << "[SharedConfig] 连接到已存在的共享内存段，ID: " << shmid_ << std::endl;
    }
    
    // 将共享内存段附加到当前进程的地址空间
    sharedData_ = static_cast<SharedConfig*>(shmat(shmid_, nullptr, 0));
    if (sharedData_ == reinterpret_cast<SharedConfig*>(-1)) {
        throw std::runtime_error("无法附加到共享内存: " + std::string(strerror(errno)));
    }
    
    // 如果是创建者，初始化共享数据结构
    if (isCreator_) {
        new (sharedData_) SharedConfig();  // placement new
        std::cout << "[SharedConfig] 初始化共享数据结构" << std::endl;
    }
}

// 初始化信号量
void SharedConfigManager::initializeSemaphore() {
    // 如果是创建者，先尝试删除可能存在的旧信号量
    if (isCreator_) {
        sem_unlink(SEM_NAME);  // 忽略错误
    }
    
    // 创建或打开命名信号量
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        throw std::runtime_error("无法创建信号量: " + std::string(strerror(errno)));
    }
    
    semaphore_ = static_cast<void*>(sem);
    std::cout << "[SharedConfig] 信号量初始化成功" << std::endl;
}

// 清理资源
void SharedConfigManager::cleanup() {
    // 分离共享内存
    if (sharedData_ != nullptr) {
        if (shmdt(sharedData_) == -1) {
            std::cerr << "[SharedConfig] 分离共享内存失败: " << strerror(errno) << std::endl;
        }
        sharedData_ = nullptr;
    }
    
    // 关闭信号量
    if (semaphore_ != nullptr) {
        sem_t* sem = static_cast<sem_t*>(semaphore_);
        if (sem_close(sem) == -1) {
            std::cerr << "[SharedConfig] 关闭信号量失败: " << strerror(errno) << std::endl;
        }
        semaphore_ = nullptr;
    }
    
    // 如果是创建者进程，负责清理资源
    if (isCreator_) {
        // 删除共享内存段
        if (shmid_ != -1) {
            if (shmctl(shmid_, IPC_RMID, nullptr) == -1) {
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
    sem_t* sem = static_cast<sem_t*>(semaphore_);
    
    // 获取信号量锁
    if (sem_wait(sem) == -1) {
        throw std::runtime_error("获取信号量失败: " + std::string(strerror(errno)));
    }
    
    try {
        // 检查是否已经初始化
        if (sharedData_->initialized) {
            std::cout << "[SharedConfig] 配置已由进程 " << sharedData_->initializer_pid 
                     << " 初始化，当前格式: " << (sharedData_->logFormat == 1 ? "HTML" : "TEXT") << std::endl;
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
        
        // 写入共享内存
        sharedData_->logFormat = option;
        sharedData_->initializer_pid = getpid();
        
        // 使用内存屏障确保写入顺序
        __sync_synchronize();
        
        sharedData_->initialized = true;
        
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
    if (!sharedData_) return false;
    
    // 使用内存屏障确保读取的是最新值
    __sync_synchronize();
    return sharedData_->initialized;
}

// 获取日志格式
int SharedConfigManager::getLogFormat() const {
    if (!isInitialized()) {
        throw std::runtime_error("配置尚未初始化");
    }
    
    return sharedData_->logFormat;
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
    sem_t* sem = static_cast<sem_t*>(semaphore_);
    
    if (sem_wait(sem) == -1) {
        std::cerr << "[SharedConfig] 获取状态失败" << std::endl;
        return;
    }
    
    std::cout << "=== 共享配置状态 ===" << std::endl;
    std::cout << "共享内存ID: " << shmid_ << std::endl;
    std::cout << "当前进程PID: " << getpid() << std::endl;
    std::cout << "是否为创建者: " << (isCreator_ ? "是" : "否") << std::endl;
    
    if (sharedData_->initialized) {
        std::cout << "配置状态: 已初始化" << std::endl;
        std::cout << "初始化进程PID: " << sharedData_->initializer_pid << std::endl;
        std::cout << "日志格式: " << (sharedData_->logFormat == 1 ? "HTML" : "TEXT") << std::endl;
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

} // namespace ConfigSpace