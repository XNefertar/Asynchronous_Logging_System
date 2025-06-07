#include "AsyncDBWriter.hpp"

using namespace Server;
namespace AsyncDBWriterSpace {

void AsyncDBWriter::addTask(const DBWriteTask& task) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        taskQueue_.push(task);
    }
    cv_.notify_one(); // ֪ͨ�����߳���������
}

void AsyncDBWriter::start(int numThreads) {
    for (int i = 0; i < numThreads; ++i) {
        workerThreads_.emplace_back(&AsyncDBWriter::workerThread, this);
        LogMessage::logMessage(INFO, "�������ݿ⹤���߳� #%d", i + 1);
    }
}

void AsyncDBWriter::shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all(); // ֪ͨ���й����߳�
    
    // �ȴ������߳̽���
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // ����ʣ������
    std::queue<DBWriteTask> remainingTasks;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        remainingTasks.swap(taskQueue_);
    }
    
    LogMessage::logMessage(INFO, "����ʣ��� %zu �����ݿ�����", remainingTasks.size());
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
                return DBWriteTask("", "", 0, ""); // ���ؿ������ʾ����
            }
            
            if (taskQueue_.empty()) {
                return DBWriteTask("", "", 0, ""); // ��ʱ���ؿ�����
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
        LogMessage::logMessage(ERROR, "�޷���ȡ���ݿ�����");
        return false;
    }
    
    // ��ʼ��Ԥ�������
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        LogMessage::logMessage(ERROR, "mysql_stmt_init() ʧ��: %s", mysql_error(conn));
        return false;
    }
    
    // ׼������ռλ����SQL���
    const char *query = "INSERT INTO log_table (level, ip, port, message) VALUES (?, ?, ?, ?)";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        LogMessage::logMessage(ERROR, "mysql_stmt_prepare() ʧ��: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // �󶨲���
    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    
    // level ����
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)task.logLevel.c_str();
    bind[0].buffer_length = task.logLevel.length();
    
    // ip ����
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)task.clientIP.c_str();
    bind[1].buffer_length = task.clientIP.length();
    
    // port ����
    unsigned int port = task.clientPort;
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = (void*)&port;
    
    // message ����
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (void*)task.message.c_str();
    bind[3].buffer_length = task.message.length();
    
    // �󶨲�����Ԥ�������
    if (mysql_stmt_bind_param(stmt, bind)) {
        LogMessage::logMessage(ERROR, "mysql_stmt_bind_param() ʧ��: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // ִ��Ԥ�������
    if (mysql_stmt_execute(stmt)) {
        LogMessage::logMessage(ERROR, "mysql_stmt_execute() ʧ��: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    LogMessage::logMessage(INFO, "MySQL �ɹ�������־: level=%s, ip=%s, port=%u", 
                           task.logLevel.c_str(), task.clientIP.c_str(), port);
    
    broadcastLogToWebSocket(task.logLevel, task.message, task.timestamp);
    // �ر�Ԥ�������
    mysql_stmt_close(stmt);
    return true;
}

} // namespace AsyncDBWriter