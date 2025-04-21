#include "../../MySQL/SqlConnPool.hpp"
#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

// 静态实例指针 - 根据您实际使用的变量名
// 这里不使用外部变量声明，而是定义一个私有静态变量
// 当实际实现调用getInstance时，会返回这个私有实例
static SqlConnPool* sqlConnPoolInstance = nullptr;

// 静态假连接
static char fakeConn[sizeof(MYSQL)];

// 模拟查询结果数据
static std::unordered_map<std::string, std::vector<std::unordered_map<std::string, std::string>>> mockData;

// 初始化一些测试数据
void initMockData() {
    // 添加日志示例数据
    std::vector<std::unordered_map<std::string, std::string>> logs;
    
    std::unordered_map<std::string, std::string> log1;
    log1["id"] = "1";
    log1["level"] = "INFO";
    log1["message"] = "测试信息日志";
    log1["timestamp"] = "2023-01-01 10:00:00";
    logs.push_back(log1);
    
    std::unordered_map<std::string, std::string> log2;
    log2["id"] = "2";
    log2["level"] = "WARNING";
    log2["message"] = "测试警告日志";
    log2["timestamp"] = "2023-01-01 10:05:00";
    logs.push_back(log2);
    
    std::unordered_map<std::string, std::string> log3;
    log3["id"] = "3";
    log3["level"] = "ERROR";
    log3["message"] = "测试错误日志";
    log3["timestamp"] = "2023-01-01 10:10:00";
    logs.push_back(log3);
    
    mockData["logs"] = logs;
}

// SqlConnPool方法实现
SqlConnPool* SqlConnPool::getInstance() {
    if (sqlConnPoolInstance == nullptr) {
        sqlConnPoolInstance = new SqlConnPool();
        initMockData(); // 初始化模拟数据
    }
    return sqlConnPoolInstance;
}

bool SqlConnPool::init(const char* host, const char* user, const char* pwd, const char* dbName, int port, int connSize) {
    // 模拟成功初始化
    return true;
}

void SqlConnPool::createDatabase(std::string dbName) {
    // 模拟创建数据库，不做任何实际操作
}

void SqlConnPool::destroyPool() {
    if (sqlConnPoolInstance) {
        delete sqlConnPoolInstance;
        sqlConnPoolInstance = nullptr;
    }
}

MYSQL* SqlConnPool::getConnection() {
    return reinterpret_cast<MYSQL*>(fakeConn);
}

void SqlConnPool::releaseConnection(MYSQL* conn) {
    // 空实现
}

// 添加模拟数据库查询函数
extern "C" {
    std::vector<std::map<std::string, std::string>> fetchLogsFromDatabase(int limit, int offset, const std::string& levelFilter) {
        std::vector<std::map<std::string, std::string>> result;
        auto& logs = mockData["logs"];
        
        for (auto& log : logs) {
            if (levelFilter.empty() || log["level"] == levelFilter) {
                if (offset > 0) {
                    offset--;
                    continue;
                }
                
                if (result.size() < static_cast<size_t>(limit)) {
                    std::map<std::string, std::string> entry;
                    for (auto& pair : log) {
                        entry[pair.first] = pair.second;
                    }
                    result.push_back(entry);
                }
            }
        }
        
        return result;
    }
    
    int getTotalLogsCount() {
        return mockData["logs"].size();
    }
    
    int getLogCountByLevel(const std::string& level) {
        int count = 0;
        for (auto& log : mockData["logs"]) {
            if (log["level"] == level) {
                count++;
            }
        }
        return count;
    }
}