#ifndef __LOG_MESSAGE_HPP__
#define __LOG_MESSAGE_HPP__

#include <iostream>
#include <string>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include "AsyncLogBuffer.hpp"

#define NORMAL 0
#define INFO 1
#define WARNING 2
#define ERROR 3
#define FATAL 4
#define DEBUG 5

#define LOG_LEVEL int

std::string to_log(LOG_LEVEL level);

class LogMessage {
private:
    static std::string default_log_path;
    static std::unique_ptr<AsyncLogBuffer> logBuffer;
    static std::once_flag initFlag;
    
    static void initializeBuffer();

public:
    static void logMessage(LOG_LEVEL level, const char *message, ...);
    static void setDefaultLogPath(const std::string &path);
    static void flush(); // 手动刷新日志
};

#endif // __LOG_MESSAGE_HPP__