#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <atomic>
#include <mysql/mysql.h>
#include "Server.hpp"
#include "../MySQL/SqlConnPool.hpp"
#include "../LogMessage/LogMessage.hpp"

namespace AsyncDBWriterSpace {

// 数据库写入任务结构
struct DBWriteTask {
    std::string logLevel;
    std::string clientIP;
    int clientPort;
    std::string message;
    std::string timestamp;  // 可选，如果需要记录时间戳
    
    DBWriteTask(const std::string& level, const std::string& ip, int port, 
                const std::string& msg, const std::string& time = "")
        : logLevel(level), clientIP(ip), clientPort(port), message(msg), timestamp(time) {}
};

class AsyncDBWriter {
public:
    static AsyncDBWriter& getInstance() {
        static AsyncDBWriter instance;
        return instance;
    }
    
    // 添加任务到队列
    void addTask(const DBWriteTask& task);
    
    // 启动后台处理线程
    void start(int numThreads = 2);
    
    // 优雅关闭
    void shutdown();

    int getQueueSize();

private:
    AsyncDBWriter() : running_(true) {}
    ~AsyncDBWriter();
    
    // 工作线程函数
    void workerThread();
    
    // 执行数据库写入操作
    bool executeWrite(const DBWriteTask& task);
    
    std::queue<DBWriteTask> taskQueue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_;
};

// 创建SyncDBWriter类作为对照组
class SyncDBWriter {
public:
    static bool writeLog(const DBWriteTask& task) {;
        
        MYSQL* conn = nullptr;
        SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
        if (!conn) return false;
        
        if (!conn) {
            LogMessage::logMessage(ERROR, "无法获取数据库连接");
            return false;
        }
        
        // 初始化预处理语句
        MYSQL_STMT *stmt = mysql_stmt_init(conn);
        if (!stmt) {
            LogMessage::logMessage(ERROR, "mysql_stmt_init() 失败: %s", mysql_error(conn));
            return false;
        }
        
        // 准备带有占位符的SQL语句
        const char *query = "INSERT INTO log_table (level, ip, port, message) VALUES (?, ?, ?, ?)";
        if (mysql_stmt_prepare(stmt, query, strlen(query))) {
            LogMessage::logMessage(ERROR, "mysql_stmt_prepare() 失败: %s", mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return false;
        }
        
        // 绑定参数
        MYSQL_BIND bind[4];
        memset(bind, 0, sizeof(bind));
        
        // level 参数
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)task.logLevel.c_str();
        bind[0].buffer_length = task.logLevel.length();
        
        // ip 参数
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)task.clientIP.c_str();
        bind[1].buffer_length = task.clientIP.length();
        
        // port 参数
        unsigned int port = task.clientPort;
        bind[2].buffer_type = MYSQL_TYPE_LONG;
        bind[2].buffer = (void*)&port;
        
        // message 参数
        bind[3].buffer_type = MYSQL_TYPE_STRING;
        bind[3].buffer = (void*)task.message.c_str();
        bind[3].buffer_length = task.message.length();
        
        // 绑定参数到预处理语句
        if (mysql_stmt_bind_param(stmt, bind)) {
            LogMessage::logMessage(ERROR, "mysql_stmt_bind_param() 失败: %s", mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return false;
        }
        
        // 执行预处理语句
        if (mysql_stmt_execute(stmt)) {
            LogMessage::logMessage(ERROR, "mysql_stmt_execute() 失败: %s", mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return false;
        }
        
        LogMessage::logMessage(INFO, "MySQL 成功插入日志: level=%s, ip=%s, port=%u", 
                            task.logLevel.c_str(), task.clientIP.c_str(), port);
        
        // 关闭预处理语句
        mysql_stmt_close(stmt);
        return true;
            
        return true;
    }
};


} // namespace AsyncDBWriter