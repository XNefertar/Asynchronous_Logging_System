#include "AsyncLogBuffer.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

AsyncLogBuffer::AsyncLogBuffer(const std::string& logPath) : logFilePath_(logPath) {
    currentBuffer.reserve(BUFFER_SIZE);
    nextBuffer.reserve(BUFFER_SIZE);
    
    // 启动后台刷新线程
    flushThread_ = std::thread(&AsyncLogBuffer::flushThreadFunc, this);
}

AsyncLogBuffer::~AsyncLogBuffer() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
        cv_.notify_one();
    }
    
    if (flushThread_.joinable()) {
        flushThread_.join();
    }
}

void AsyncLogBuffer::append(const std::string& logLine) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (currentBuffer.size() < BUFFER_SIZE) {
        currentBuffer.push_back(logLine);
    } else {
        // 当前缓冲区已满, 交换到fullBuffers队列
        fullBuffers.push_back(std::move(currentBuffer));
        
        // 如果nextBuffer可用, 则与currentBuffer交换
        if (!nextBuffer.empty()) {
            currentBuffer = std::move(nextBuffer);
        } else {
            // 创建新缓冲区
            currentBuffer.clear();
            currentBuffer.reserve(BUFFER_SIZE);
        }
        
        currentBuffer.push_back(logLine);
        cv_.notify_one(); // 通知后台线程开始写入
    }
}

void AsyncLogBuffer::flushThreadFunc() {
    std::vector<std::vector<std::string>> buffersToWrite;
    
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // 等待直到有数据可写或程序退出
            cv_.wait_for(lock, std::chrono::seconds(3), [this] {
                return !fullBuffers.empty() || !running_;
            });
            
            // 收集所有待写入的缓冲区
            if (!fullBuffers.empty()) {
                buffersToWrite.swap(fullBuffers);
                
                // 如果当前缓冲区也已经有内容, 将其加入待写队列
                if (!currentBuffer.empty()) {
                    buffersToWrite.push_back(std::move(currentBuffer));
                    
                    // 恢复缓冲区
                    if (!nextBuffer.empty()) {
                        currentBuffer = std::move(nextBuffer);
                    } else {
                        currentBuffer.clear();
                        currentBuffer.reserve(BUFFER_SIZE);
                    }
                }
                
                // 确保nextBuffer可用
                if (nextBuffer.empty()) {
                    nextBuffer.reserve(BUFFER_SIZE);
                }
            }
        }
        
        // 批量写入文件
        if (!buffersToWrite.empty()) {
            int fd = open(logFilePath_.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
            if (fd >= 0) {
                for (const auto& buffer : buffersToWrite) {
                    for (const auto& line : buffer) {
                        write(fd, line.c_str(), line.size());
                    }
                }
                close(fd);
            }
            
            buffersToWrite.clear();
        }
    }
    
    // 程序退出前确保所有日志都写入
    int fd = open(logFilePath_.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
    if (fd >= 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 写入剩余的所有缓冲区
        for (const auto& buffer : fullBuffers) {
            for (const auto& line : buffer) {
                write(fd, line.c_str(), line.size());
            }
        }
        
        // 写入当前缓冲区
        for (const auto& line : currentBuffer) {
            write(fd, line.c_str(), line.size());
        }
        
        close(fd);
    }
}