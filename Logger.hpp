#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include "LogQueue.hpp"
#include "ThreadPool.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

const int THREADS_SIZE = 10;
using INFO_LEVEL = int;
enum LogLevel{
    INFO,
    DEBUG,
    WARNING,
    ERROR,
    FATAL
};

class Logger{
    friend class ThreadPool;
private:
    LogQueue m_queue;
    std::unique_ptr<ThreadPool> m_threads;
    std::ofstream m_file;
    std::mutex m_mutex;
    fs::path m_logPath;

public:
    Logger(const std::string& filename)
        : m_threads(new ThreadPool(THREADS_SIZE)), m_logPath(filename)
    {
        // 确保日志文件所在目录存在
        fs::path logDir = m_logPath.parent_path();
        if (!logDir.empty() && !fs::exists(logDir)) {
            fs::create_directories(logDir);
        }

        // 打开日志文件
        m_file.open(m_logPath, std::ios::out | std::ios::app);
        if(!m_file.is_open()){
            throw std::runtime_error("Failed to open log file: " + m_logPath.string());
        }
        
        // 记录日志系统启动信息
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char time_buffer[100];
        std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
        
        m_file << "[SYSTEM] Logger started at " << time_buffer 
               << " (File: " << fs::absolute(m_logPath).string() << ")" << std::endl;
    }

    ~Logger(){
        m_queue.stop();
        if(m_file.is_open()){
            // 记录日志系统关闭信息
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            char time_buffer[100];
            std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
            
            m_file << "[SYSTEM] Logger stopped at " << time_buffer 
                   << " (File size: " << fs::file_size(m_logPath) << " bytes)" << std::endl;
                   
            m_file.flush();
            m_file.close();
        }
    }

    // 日志文件轮转功能
    bool rotateLogFile(const std::string& newFilename) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 关闭现有文件
        if(m_file.is_open()) {
            m_file.flush();
            m_file.close();
        }
        
        // 如果指定了新文件名，则使用它，否则自动生成带时间戳的文件名
        fs::path newPath;
        if (!newFilename.empty()) {
            newPath = newFilename;
        } else {
            // 生成时间戳文件名
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            char time_buffer[100];
            std::strftime(time_buffer, sizeof(time_buffer), "%Y%m%d_%H%M%S", std::localtime(&now_time));
            
            newPath = m_logPath.parent_path() / (m_logPath.stem().string() + "_" + time_buffer + m_logPath.extension().string());
        }
        
        // 确保目录存在
        fs::path logDir = newPath.parent_path();
        if (!logDir.empty() && !fs::exists(logDir)) {
            fs::create_directories(logDir);
        }
        
        // 打开新文件
        m_file.open(newPath, std::ios::out | std::ios::app);
        if (!m_file.is_open()) {
            // 如果新文件打不开，尝试重新打开原文件
            m_file.open(m_logPath, std::ios::out | std::ios::app);
            return false;
        }
        
        // 更新文件路径
        m_logPath = newPath;
        return true;
    }

    // 获取当前日志文件大小
    std::uintmax_t getLogFileSize() const {
        try {
            return fs::file_size(m_logPath);
        } catch (const fs::filesystem_error&) {
            return 0;
        }
    }

    template <typename ...Args>
    void log(INFO_LEVEL level, const std::string& format, Args ...args){
        m_queue.push(process_string(level, format, args...));
        m_threads->submitTask([this](){
            process();
        });
    }

private:
    std::string getLevelString(INFO_LEVEL level) const {
        switch (level) {
            case INFO: return "INFO";
            case DEBUG: return "DEBUG";
            case WARNING: return "WARNING";
            case ERROR: return "ERROR";
            case FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    void process(){
        std::string msg;
        std::lock_guard<std::mutex> lock(m_mutex);
        while(m_queue.pop(msg)){
            m_file << msg << std::endl;
        }
    }

    template <typename ...Args>
    std::string process_string(INFO_LEVEL level, const std::string& format, Args ...args){
        std::vector<std::string> args_str = {to_string(args)...};
        std::ostringstream os;
        os.clear();

        os << "[" << getLevelString(level) << "] ";
        int args_index = 0, last_position = 0;
        int placeholder_index = format.find("{}");
        while(placeholder_index != std::string::npos){
            if(args_index >= args_str.size()){
                break;
            }
            os << format.substr(last_position, placeholder_index - last_position);
            os << args_str[args_index++];
            last_position = placeholder_index + 2;
            placeholder_index = format.find("{}", last_position);
        }

        if(!format.empty() && args_index >= args_str.size()){
            os << format.substr(last_position);
        }
        while(args_index < args_str.size()){
            os << args_str[args_index++];
        }

        return os.str();
    }
};

#endif // __LOGGER_HPP__