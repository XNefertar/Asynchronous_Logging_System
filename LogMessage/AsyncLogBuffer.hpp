#ifndef __ASYNC_LOG_BUFFER_HPP__
#define __ASYNC_LOG_BUFFER_HPP__

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class AsyncLogBuffer {
private:
    static const size_t BUFFER_SIZE = 1024 * 1024; // 1MB缓冲区
    
    std::vector<std::string> currentBuffer;  // 前台缓冲区
    std::vector<std::string> nextBuffer;     // 后台缓冲区
    std::vector<std::vector<std::string>> fullBuffers; // 已满的缓冲区队列
    
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread flushThread_;
    std::atomic<bool> running_{true};
    
    std::string logFilePath_;
    
public:
    AsyncLogBuffer(const std::string& logPath);
    ~AsyncLogBuffer();
    
    void append(const std::string& logLine);
    void flushThreadFunc();
};

#endif // __ASYNC_LOG_BUFFER_HPP__