#include "SqlConnPool.hpp"

SqlConnPool* SqlConnPool::getInstance(){
    // C++11 线程安全的单例模式
    // C++11 之后，局部静态变量在多线程中是线程安全的
    // 只会被初始化一次
    static SqlConnPool instance;
    return &instance;
}

void SqlConnPool::createDatabase(std::string dbName)
{
    assert(!dbName.empty());
    MYSQL* conn = nullptr;
    SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
    if (conn){
        std::string sql = "CREATE DATABASE IF NOT EXISTS " + dbName;
        if (mysql_query(conn, sql.c_str())){
            LogMessage::logMessage(ERROR, "MySql create database error!");
        }
        else{
            LogMessage::logMessage(INFO, "MySql create database success!");
        }
    }
    else{
        LogMessage::logMessage(ERROR, "MySql connection error!");
    }
}

bool SqlConnPool::init(const char *host, const char *user, const char *password, const char *dbName, int port, int connSize) {
    std::string path = std::filesystem::current_path().string() + "/Log/SqlConnPool.txt";
    LogMessage::setDefaultLogPath(path);

    // 1. 创建临时连接，不指定数据库
    MYSQL* temp = mysql_init(nullptr);
    if (!temp) {
        LogMessage::logMessage(ERROR, "Failed to initialize MySQL connection");
        return false;
    }

    // 连接到服务器，不指定数据库
    if (!mysql_real_connect(temp, host, user, password, nullptr, port, nullptr, 0)) {
        LogMessage::logMessage(ERROR, "Failed to connect to MySQL server: %s", mysql_error(temp));
        mysql_close(temp);
        return false;
    }

    // 2. 创建数据库
    std::string sql = "CREATE DATABASE IF NOT EXISTS ";
    sql += dbName;
    if (mysql_query(temp, sql.c_str()) != 0) {
        LogMessage::logMessage(ERROR, "Failed to create database: %s", mysql_error(temp));
        mysql_close(temp);
        return false;
    }

    // 选择刚创建的数据库
    if (mysql_select_db(temp, dbName) != 0) {
        LogMessage::logMessage(ERROR, "Failed to select database: %s", mysql_error(temp));
        mysql_close(temp);
        return false;
    }
    LogMessage::logMessage(INFO, "Database %s selected successfully", dbName);

    // 3. 创建表结构
    // TODO: 加入日志等级
    sql = "CREATE TABLE IF NOT EXISTS log_table (id BIGINT AUTO_INCREMENT PRIMARY KEY, ip VARCHAR(32) NOT NULL, port INT NOT NULL, message TEXT, timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";
    if (mysql_query(temp, sql.c_str()) != 0) {
        LogMessage::logMessage(ERROR, "Failed to create table: %s", mysql_error(temp));
        mysql_close(temp);
        return false;
    }
    LogMessage::logMessage(INFO, "Database and table created successfully");

    mysql_close(temp);

    // 4. 初始化连接池
    for (int i = 0; i < connSize; i++) {
        MYSQL *conn = mysql_init(nullptr);
        if (!conn) {
            LogMessage::logMessage(ERROR, "MySQL init error!");
            continue;
        }

        conn = mysql_real_connect(conn, host, user, password, dbName, port, nullptr, 0);
        if (!conn) {
            LogMessage::logMessage(ERROR, "MySQL Connect error: %s", mysql_error(conn));
            continue;
        }

        _connQueue.emplace(conn);
    }

    _maxConn = _connQueue.size();
    sem_init(&_semID, 0, _maxConn);

    return true;
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
