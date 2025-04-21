#include <gtest/gtest.h>
#include "../../MySQL/SqlConnPool.hpp"
#include <thread>
#include <vector>
#include <future>

class SqlConnPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 确保连接池已初始化并清空
        SqlConnPool::getInstance()->destroyPool();
    }
    
    void TearDown() override {
        SqlConnPool::getInstance()->destroyPool();
    }
};

// 测试连接池初始化
TEST_F(SqlConnPoolTest, InitPool) {
    auto pool = SqlConnPool::getInstance();
    bool result = pool->init("localhost", "test_user", "test_password", "test_db", 3306, 5);
    EXPECT_TRUE(result);
    
    // 验证连接池正常工作
    MYSQL* conn = pool->getConnection();
    EXPECT_NE(nullptr, conn);
    
    // 释放连接
    pool->releaseConnection(conn);
}

// 测试连接的获取与释放
TEST_F(SqlConnPoolTest, GetAndReleaseConnection) {
    auto pool = SqlConnPool::getInstance();
    pool->init("localhost", "test_user", "test_password", "test_db", 3306, 5);
    
    std::vector<MYSQL*> connections;
    
    // 获取5个连接
    for (int i = 0; i < 5; i++) {
        MYSQL* conn = pool->getConnection();
        EXPECT_NE(nullptr, conn);
        connections.push_back(conn);
    }
    
    // 应该没有更多连接可用
    MYSQL* extraConn = pool->getConnection();
    EXPECT_EQ(nullptr, extraConn);
    
    // 释放一个连接
    pool->releaseConnection(connections.back());
    connections.pop_back();
    
    // 现在应该可以获取一个连接
    MYSQL* newConn = pool->getConnection();
    EXPECT_NE(nullptr, newConn);
    connections.push_back(newConn);
    
    // 释放所有连接
    for (auto conn : connections) {
        pool->releaseConnection(conn);
    }
}

// 测试多线程环境下的连接池行为
TEST_F(SqlConnPoolTest, MultithreadedUsage) {
    auto pool = SqlConnPool::getInstance();
    pool->init("localhost", "test_user", "test_password", "test_db", 3306, 5);
    
    constexpr int numThreads = 10;
    constexpr int operationsPerThread = 20;
    
    auto threadFunc = [pool]() {
        int successfulOperations = 0;
        
        for (int i = 0; i < operationsPerThread; i++) {
            MYSQL* conn = pool->getConnection();
            if (conn) {
                // 模拟数据库操作
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                pool->releaseConnection(conn);
                successfulOperations++;
            } else {
                // 连接不可用时等待一会儿
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        
        return successfulOperations;
    };
    
    std::vector<std::future<int>> futures;
    for (int i = 0; i < numThreads; i++) {
        futures.push_back(std::async(std::launch::async, threadFunc));
    }
    
    int totalSuccessful = 0;
    for (auto& future : futures) {
        totalSuccessful += future.get();
    }
    
    // 预期所有操作都能成功，因为每个操作都会释放连接
    EXPECT_EQ(numThreads * operationsPerThread, totalSuccessful);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}