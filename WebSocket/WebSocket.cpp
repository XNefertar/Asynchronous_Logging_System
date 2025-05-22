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
    

    // TODO 
    // MySQL注入问题
    // 构建SQL查询
    std::string sql = "SELECT level, message, DATE_FORMAT(timestamp, '%Y-%m-%d %H:%i:%s') as timestamp FROM log_table";
    
    // 添加过滤条件
    if (!levelFilter.empty()) {
        sql += " WHERE level = '" + levelFilter + "'";
    }
    
    // 添加排序和分页
    sql += " ORDER BY timestamp DESC LIMIT " + std::to_string(offset) + ", " + std::to_string(limit);
    
    // 执行查询
    if (mysql_query(conn, sql.c_str()) != 0) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return logs;
    }
    
    // 获取结果
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "Failed to store result: " << mysql_error(conn) << std::endl;
        return logs;
    }
    
    // 获取字段数量
    int num_fields = mysql_num_fields(result);
    
    // 处理每一行结果
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        std::map<std::string, std::string> log_entry;
        log_entry["level"] = row[0] ? row[0] : "";
        log_entry["message"] = row[1] ? row[1] : "";
        log_entry["timestamp"] = row[2] ? row[2] : "";
        logs.push_back(log_entry);
    }
    
    // 释放结果集
    mysql_free_result(result);
    
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
    
    // TODO
    // MySQL注入问题
    // 执行COUNT查询
    if (mysql_query(conn, "SELECT COUNT(*) FROM log_table") != 0) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return 0;
    }
    
    // 获取结果
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "Failed to store result: " << mysql_error(conn) << std::endl;
        return 0;
    }
    
    // 获取计数值
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
        count = std::stoi(row[0]);
    }
    
    // 释放结果集
    mysql_free_result(result);
    
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
    
    // 构建SQL查询
    std::string sql = "SELECT COUNT(*) FROM log_table WHERE level = '" + level + "'";
    
    // 执行查询
    if (mysql_query(conn, sql.c_str()) != 0) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return 0;
    }
    
    // 获取结果
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "Failed to store result: " << mysql_error(conn) << std::endl;
        return 0;
    }
    
    // 获取计数值
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
        count = std::stoi(row[0]);
    }
    
    // 释放结果集
    mysql_free_result(result);
    
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