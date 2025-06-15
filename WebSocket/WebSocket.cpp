#include "WebSocket.hpp"
#include <openssl/sha.h>  // 需要 OpenSSL 库
#include <openssl/evp.h>

#include "WebSocket.hpp"
#include "../Util/SessionManager.hpp"
#include <mysql/mysql.h>
#include <iostream>

// 全局服务器引用 (extern 声明)
extern EpollServer* g_server;

void setGlobalServerReference(EpollServer* server) {
    g_server = server;
}

std::vector<std::map<std::string, std::string>> fetchLogsFromDatabase(int limit, int offset, const std::string& levelFilter) {
    std::vector<std::map<std::string, std::string>> logs;
    
    // 获取数据库连接
    MYSQL* conn = nullptr;
    SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
    
    if (!conn) {
        std::cerr << "Failed to get database connection" << std::endl;
        return logs;
    }
    
    // 启动读事务，设置隔离级别
    mysql_autocommit(conn, 0);  // 关闭自动提交
    mysql_query(conn, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
    mysql_query(conn, "START TRANSACTION");
    
    // 使用预处理语句防止SQL注入并支持事务
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        mysql_rollback(conn);
        std::cerr << "mysql_stmt_init failed: " << mysql_error(conn) << std::endl;
        return logs;
    }
    
    // 构建安全的SQL查询
    std::string sql;
    int paramCount = 2; // offset 和 limit
    
    if (!levelFilter.empty()) {
        sql = "SELECT level, message, DATE_FORMAT(timestamp, '%Y-%m-%d %H:%i:%s') as timestamp "
              "FROM log_table WHERE level = ? ORDER BY timestamp DESC LIMIT ?, ?";
        paramCount = 3; // level, offset, limit
    } else {
        sql = "SELECT level, message, DATE_FORMAT(timestamp, '%Y-%m-%d %H:%i:%s') as timestamp "
              "FROM log_table ORDER BY timestamp DESC LIMIT ?, ?";
    }
    
    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length())) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << std::endl;
        return logs;
    }
    
    // 绑定参数
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    
    int paramIndex = 0;
    
    // level 参数 (如果有过滤条件)
    if (!levelFilter.empty()) {
        bind[paramIndex].buffer_type = MYSQL_TYPE_STRING;
        bind[paramIndex].buffer = (void*)levelFilter.c_str();
        bind[paramIndex].buffer_length = levelFilter.length();
        paramIndex++;
    }
    
    // offset 参数
    bind[paramIndex].buffer_type = MYSQL_TYPE_LONG;
    bind[paramIndex].buffer = (void*)&offset;
    paramIndex++;
    
    // limit 参数
    bind[paramIndex].buffer_type = MYSQL_TYPE_LONG;
    bind[paramIndex].buffer = (void*)&limit;
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt) << std::endl;
        return logs;
    }
    
    // 执行查询
    if (mysql_stmt_execute(stmt)) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << std::endl;
        return logs;
    }
    
    // 获取结果元数据
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_result_metadata failed: " << mysql_stmt_error(stmt) << std::endl;
        return logs;
    }
    
    // 绑定结果
    MYSQL_BIND resultBind[3];
    memset(resultBind, 0, sizeof(resultBind));
    
    char level[64], message[2048], timestamp[32];
    unsigned long levelLen, messageLen, timestampLen;
    bool levelIsNull, messageIsNull, timestampIsNull;
    
    resultBind[0].buffer_type = MYSQL_TYPE_STRING;
    resultBind[0].buffer = level;
    resultBind[0].buffer_length = sizeof(level);
    resultBind[0].length = &levelLen;
    resultBind[0].is_null = &levelIsNull;
    
    resultBind[1].buffer_type = MYSQL_TYPE_STRING;
    resultBind[1].buffer = message;
    resultBind[1].buffer_length = sizeof(message);
    resultBind[1].length = &messageLen;
    resultBind[1].is_null = &messageIsNull;
    
    resultBind[2].buffer_type = MYSQL_TYPE_STRING;
    resultBind[2].buffer = timestamp;
    resultBind[2].buffer_length = sizeof(timestamp);
    resultBind[2].length = &timestampLen;
    resultBind[2].is_null = &timestampIsNull;
    
    if (mysql_stmt_bind_result(stmt, resultBind)) {
        mysql_rollback(conn);
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_bind_result failed: " << mysql_stmt_error(stmt) << std::endl;
        return logs;
    }
    
    // 获取数据并处理结果
    while (mysql_stmt_fetch(stmt) == 0) {
        std::map<std::string, std::string> log_entry;
        
        log_entry["level"] = levelIsNull ? "" : std::string(level, levelLen);
        log_entry["message"] = messageIsNull ? "" : std::string(message, messageLen);
        log_entry["timestamp"] = timestampIsNull ? "" : std::string(timestamp, timestampLen);
        
        logs.push_back(log_entry);
    }

    // 提交读事务
    if (mysql_commit(conn) != 0) {
        mysql_rollback(conn);
        std::cerr << "Read transaction commit failed: " << mysql_error(conn) << std::endl;
    }
    
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    return logs;
}

int getTotalLogsCount() {
    int count = 0;
    
    // 获取数据库连接
    MYSQL* conn = nullptr;
    SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
    
    if (!conn) {
        std::cerr << "Failed to get database connection" << std::endl;
        return 0;
    }
    
    // 启动读事务
    mysql_autocommit(conn, 0);
    mysql_query(conn, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
    mysql_query(conn, "START TRANSACTION");
    
    // 使用预处理语句
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        mysql_rollback(conn);
        std::cerr << "mysql_stmt_init failed: " << mysql_error(conn) << std::endl;
        return 0;
    }
    
    const char* sql = "SELECT COUNT(*) FROM log_table";
    if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << std::endl;
        return 0;
    }
    
    if (mysql_stmt_execute(stmt)) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << std::endl;
        return 0;
    }
    
    // 绑定结果
    MYSQL_BIND resultBind[1];
    memset(resultBind, 0, sizeof(resultBind));
    
    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = &count;
    
    if (mysql_stmt_bind_result(stmt, resultBind)) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_bind_result failed: " << mysql_stmt_error(stmt) << std::endl;
        return 0;
    }
    
    if (mysql_stmt_fetch(stmt) == 0) {
        // count 已经被填充
    }
    
    // 提交读事务
    mysql_commit(conn);
    mysql_stmt_close(stmt);
    
    return count;
}

int getLogCountByLevel(const std::string& level) {
    int count = 0;
    
    // 获取数据库连接
    MYSQL* conn = nullptr;
    SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
    
    if (!conn) {
        std::cerr << "Failed to get database connection" << std::endl;
        return 0;
    }
    
    // 启动读事务
    mysql_autocommit(conn, 0);
    mysql_query(conn, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
    mysql_query(conn, "START TRANSACTION");
    
    //  使用预处理语句防止SQL注入
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        mysql_rollback(conn);
        std::cerr << "mysql_stmt_init failed: " << mysql_error(conn) << std::endl;
        return 0;
    }
    
    const char* sql = "SELECT COUNT(*) FROM log_table WHERE level = ?";
    if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << std::endl;
        return 0;
    }
    
    // 绑定参数
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)level.c_str();
    bind[0].buffer_length = level.length();
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt) << std::endl;
        return 0;
    }
    
    if (mysql_stmt_execute(stmt)) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << std::endl;
        return 0;
    }
    
    // 绑定结果
    MYSQL_BIND resultBind[1];
    memset(resultBind, 0, sizeof(resultBind));
    
    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = &count;
    
    if (mysql_stmt_bind_result(stmt, resultBind)) {
        mysql_rollback(conn);
        mysql_stmt_close(stmt);
        std::cerr << "mysql_stmt_bind_result failed: " << mysql_stmt_error(stmt) << std::endl;
        return 0;
    }
    
    if (mysql_stmt_fetch(stmt) == 0) {
        // count 已经被填充
    }
    
    // 提交读事务
    mysql_commit(conn);
    mysql_stmt_close(stmt);
    
    return count;
}

std::vector<char> createWebSocketFrameWrapper(const std::string& message) {
    if (g_server) {
        // 使用EpollServer的createWebSocketFrame方法
        return g_server->createWebSocketFrame(message);
    }
    // 返回空向量作为默认值
    return std::vector<char>();
}

void broadcastWebSocketMessageWrapper(const std::string& message) {
    if (g_server) {
        g_server->broadcastWebSocketMessage(message);
    }
}