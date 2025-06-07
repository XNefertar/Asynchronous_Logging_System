#include "AsyncDBWriter.hpp"

using namespace Server;
namespace AsyncDBWriterSpace {

void AsyncDBWriter::addTask(const DBWriteTask& task) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        taskQueue_.push(task);
    }
    cv_.notify_one(); // 通知工作线程有新任务
}

void AsyncDBWriter::start(int numThreads) {
    for (int i = 0; i < numThreads; ++i) {
        workerThreads_.emplace_back(&AsyncDBWriter::workerThread, this);
        LogMessage::logMessage(INFO, "启动数据库工作线程 #%d", i + 1);
    }
}

void AsyncDBWriter::shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all(); // 通知所有工作线程
    
    // 等待所有线程结束
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // 处理剩余任务
    std::queue<DBWriteTask> remainingTasks;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        remainingTasks.swap(taskQueue_);
    }
    
    LogMessage::logMessage(INFO, "处理剩余的 %zu 个数据库任务", remainingTasks.size());
    while (!remainingTasks.empty()) {
        executeWrite(remainingTasks.front());
        remainingTasks.pop();
    }
}

AsyncDBWriter::~AsyncDBWriter() {
    if (running_) {
        shutdown();
    }
}

int AsyncDBWriter::getQueueSize() {
    std::lock_guard<std::mutex> lock(mutex_);
    return taskQueue_.size();
}

void AsyncDBWriter::workerThread() {
    while (running_) {
        DBWriteTask task = [this] {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(1), [this] { 
                return !taskQueue_.empty() || !running_; 
            });
            
            if (!running_ && taskQueue_.empty()) {
                return DBWriteTask("", "", 0, ""); // 返回空任务表示结束
            }
            
            if (taskQueue_.empty()) {
                return DBWriteTask("", "", 0, ""); // 超时返回空任务
            }
            
            DBWriteTask task = taskQueue_.front();
            taskQueue_.pop();
            return task;
        }();
        
        if (!task.logLevel.empty()) {
            executeWrite(task);
        }
    }
}

bool AsyncDBWriter::executeWrite(const DBWriteTask& task) {
    MYSQL* conn = nullptr;
    SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
    
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
    
    broadcastLogToWebSocket(task.logLevel, task.message, task.timestamp);
    // 关闭预处理语句
    mysql_stmt_close(stmt);
    return true;
}

} // namespace AsyncDBWriter