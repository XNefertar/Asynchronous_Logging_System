#include "SqlConnPool.hpp"

SqlConnPool* SqlConnPool::getInstance(){
    // C++11 线程安全的单例模式
    // C++11 之后，局部静态变量在多线程中是线程安全的
    // 只会被初始化一次
    static SqlConnPool instance;
    return &instance;
}

void SqlConnPool::init(const char *host, const char *user, const char *password, const char *dbName, int port, int connSize = 10)
{
    // TODO
    // 这里需要检查参数的合法性
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++)
    {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if (!conn)
        {
            LogMessage::logMessage(ERROR, "MySql init error!");
            assert(conn);
        }
        conn = mysql_real_connect(conn, host, user, password, dbName, port, nullptr, 0);
        if (!conn)
        {
            LogMessage::logMessage(ERROR, "MySql Connect error!");
        }
        _connQueue.emplace(conn);
    }
    _maxConn = connSize;
    sem_init(&_semID, 0, _maxConn);
}

MYSQL* SqlConnPool::getConnection(){
    // 条件变量是先上锁后判断的
    // 这里使用信号量来控制连接数
    if(_connQueue.empty()){
        LogMessage::logMessage(ERROR, "SqlConnPool: No connection available");
        return nullptr;
    }
    sem_wait(&_semID);
    std::unique_lock<std::mutex> lock(_mutex);
    MYSQL* conn = _connQueue.front();
    _connQueue.pop();
    return conn;
}

void SqlConnPool::releaseConnection(MYSQL* conn) {
    assert(conn);
    std::lock_guard<std::mutex> locker(_mutex);
    _connQueue.push(conn);
    sem_post(&_semID);  // +1
}

void SqlConnPool::destroyPool() {
    std::lock_guard<std::mutex> locker(_mutex);
    while(!_connQueue.empty()) {
        auto conn = _connQueue.front();
        _connQueue.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

int SqlConnPool::getReleaseConnCount() {
    std::lock_guard<std::mutex> locker(_mutex);
    return _connQueue.size();
}

SqlConnPool::~SqlConnPool() {
    destroyPool();
}