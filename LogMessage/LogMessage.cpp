#include "LogMessage.hpp"
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <mutex>

std::string to_log(LOG_LEVEL level)
{
    switch (level)
    {
    case NORMAL:
        return "NORMAL";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    case FATAL:
        return "FATAL";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    default:
        return "UNKNOWN";
    }
}

std::string LogMessage::default_log_path = "";
std::unique_ptr<AsyncLogBuffer> LogMessage::logBuffer = nullptr;
std::once_flag LogMessage::initFlag;

void LogMessage::initializeBuffer() {
    if (!default_log_path.empty()) {
        logBuffer = std::make_unique<AsyncLogBuffer>(default_log_path);
    }
}

void LogMessage::logMessage(LOG_LEVEL level, const char *message, ...)
{
    // 初始化缓冲区（线程安全，只会执行一次）
    std::call_once(initFlag, &LogMessage::initializeBuffer);
    
    // 如果缓冲区未初始化或路径为空，无法写入日志
    if (!logBuffer || default_log_path.empty()) {
        return;
    }
    
    va_list args;
    va_start(args, message);

    // 格式化日志头
    char buffer[1024]{};
    sprintf(buffer, "[%s][%ld][%d] ", to_log(level).c_str(), time(nullptr), getpid());

    // 格式化日志内容
    char content[1024]{};
    vsprintf(content, message, args);
    
    // 组合完整日志行
    std::string logLine = std::string(buffer) + std::string(content) + "\n";
    
    // 添加到异步缓冲区
    logBuffer->append(logLine);

    va_end(args);
}

void LogMessage::setDefaultLogPath(const std::string &path)
{
    default_log_path = path;
    
    // 如果更改了路径，重新初始化缓冲区
    if (logBuffer) {
        logBuffer = std::make_unique<AsyncLogBuffer>(path);
    }
}

void LogMessage::flush() {
    // 创建临时空日志，触发缓冲区刷新（仅用于立即需要写入的情况）
    if (logBuffer) {
        logBuffer->append("");
    }
}