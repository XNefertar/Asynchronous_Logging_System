#include <gtest/gtest.h>
#include "../../WebSocket/WebSocket.hpp"
#include "../../WebSocket/WebSocketApiHandlers.hpp"
#include "../../MySQL/SqlConnPool.hpp"
#include <thread>

// 数据库连接池测试辅助类
class DatabaseTestHelper {
public:
    static bool setupTestDatabase() {
        auto pool = SqlConnPool::getInstance();
        return pool->init("localhost", "test_user", "test_password", "test_db", 3306, 5);
    }
    
    static void cleanupTestDatabase() {
        // 清理测试数据
        MYSQL* conn = nullptr;
        SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
        if (conn) {
            mysql_query(conn, "DELETE FROM log_table WHERE 1=1");
        }
    }
    
    static void insertTestLogs() {
        MYSQL* conn = nullptr;
        SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
        if (conn) {
            mysql_query(conn, "INSERT INTO log_table (level, message, timestamp) VALUES ('INFO', 'Test info message', NOW())");
            mysql_query(conn, "INSERT INTO log_table (level, message, timestamp) VALUES ('WARNING', 'Test warning message', NOW())");
            mysql_query(conn, "INSERT INTO log_table (level, message, timestamp) VALUES ('ERROR', 'Test error message', NOW())");
        }
    }
};

class WebSocketTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
        DatabaseTestHelper::setupTestDatabase();
        DatabaseTestHelper::cleanupTestDatabase();
        DatabaseTestHelper::insertTestLogs();
    }
    
    void TearDown() override {
        // 清理测试环境
        DatabaseTestHelper::cleanupTestDatabase();
    }
};

// 测试从数据库中获取日志
TEST_F(WebSocketTest, FetchLogsFromDatabase) {
    // 测试不带过滤的情况
    auto logs = fetchLogsFromDatabase(10, 0, "");
    EXPECT_EQ(3, logs.size());
    
    // 测试带过滤的情况
    logs = fetchLogsFromDatabase(10, 0, "WARNING");
    EXPECT_EQ(1, logs.size());
    EXPECT_EQ("WARNING", logs[0]["level"]);
    EXPECT_EQ("Test warning message", logs[0]["message"]);
}

// 测试获取日志总数量
TEST_F(WebSocketTest, GetTotalLogsCount) {
    int count = getTotalLogsCount();
    EXPECT_EQ(3, count);
}

// 测试获取特定级别的日志数量
TEST_F(WebSocketTest, GetLogCountByLevel) {
    EXPECT_EQ(1, getLogCountByLevel("INFO"));
    EXPECT_EQ(1, getLogCountByLevel("WARNING"));
    EXPECT_EQ(1, getLogCountByLevel("ERROR"));
    EXPECT_EQ(0, getLogCountByLevel("DEBUG"));
}

// 更多WebSocket相关测试...
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}