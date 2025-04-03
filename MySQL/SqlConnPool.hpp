#ifndef __SQLCONNPOOL_HPP__
#define __SQLCONNPOOL_HPP__

#include <assert.h>
#include <mysql/mysql.h>
#include <thread>
#include <mutex>
#include <queue>
#include <semaphore.h>
#include <filesystem>
#include "../LogMessage/LogMessage.hpp"
// #include <condition_variable>

// 单例模式下的数据库连接池
class SqlConnPool {
private:
    std::queue<MYSQL*> _connQueue;      // 连接队列
    std::mutex _mutex;                  // 互斥锁
    sem_t _semID;                       // 信号量
    int _maxConn;                       // 最大连接数
    int _usedConn;                      // 已使用的连接数
    const char* _host;                  // 主机名
    const char* _user;                  // 用户名
    const char* _password;              // 密码
    const char* _db;                    // 数据库名
public:
    static SqlConnPool* getInstance();
    MYSQL* getConnection();
    void releaseConnection(MYSQL* conn);
    void destroyPool();
    void createDatabase(std::string dbName);
    bool init(const char* host, const char* user, const char* password, const char* dbName, int port, int maxConn);
    int getReleaseConnCount();


private:
    SqlConnPool() = default;
    ~SqlConnPool() { destroyPool(); }
    SqlConnPool(const SqlConnPool&) = delete;
    SqlConnPool& operator=(const SqlConnPool&) = delete;

};
#endif // __SQLCONNPOOL_HPP__


#ifndef __SQLCONNPOOLRAII_CPP__
#define __SQLCONNPOOLRAII_CPP__

class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** conn, SqlConnPool* connPool) : _connPool(connPool), _conn(nullptr) {
        assert(connPool);  // 添加断言确保连接池不为空
        *conn = connPool->getConnection();
        _conn = *conn;
    }
    
    ~SqlConnRAII() {
        if (_conn && _connPool) {  // 检查两个指针都不为空
            _connPool->releaseConnection(_conn);
        }
    }
    
private:
    SqlConnPool* _connPool;
    MYSQL* _conn;
};

#endif // __SQLCONNPOOLRAII_CPP__