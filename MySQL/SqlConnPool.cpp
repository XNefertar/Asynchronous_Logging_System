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

#include "SqlConnPool.hpp"
#include "../Util/EnvConfig.hpp"  // 添加环境配置头文件

// ...existing code...

bool SqlConnPool::init(const char *host, const char *user, const char *password, const char *dbName, int port, int connSize) {
    // 如果传入的参数为空或默认值，尝试从环境变量获取
    std::string actualHost = (host && strlen(host) > 0) ? host : EnvConfig::getDBHost();
    std::string actualUser = (user && strlen(user) > 0) ? user : EnvConfig::getDBUser();
    std::string actualPassword = (password && strlen(password) > 0) ? password : EnvConfig::getDBPassword();
    std::string actualDBName = (dbName && strlen(dbName) > 0) ? dbName : EnvConfig::getDBName();
    int actualPort = (port > 0) ? port : EnvConfig::getDBPort();
    
    std::string path = std::filesystem::current_path().string() + "/Log/SqlConnPool.txt";
    LogMessage::setDefaultLogPath(path);

    std::cout << "[数据库] 使用配置:" << std::endl;
    std::cout << "  - 主机: " << actualHost << std::endl;
    std::cout << "  - 端口: " << actualPort << std::endl;
    std::cout << "  - 用户: " << actualUser << std::endl;
    std::cout << "  - 数据库: " << actualDBName << std::endl;
    std::cout << "  - 连接数: " << connSize << std::endl;

    // 1. 创建临时连接，不指定数据库
    MYSQL* temp = mysql_init(nullptr);
    if (!temp) {
        LogMessage::logMessage(ERROR, "Failed to initialize MySQL connection");
        return false;
    }

    // 连接到服务器，不指定数据库
    if (!mysql_real_connect(temp, actualHost.c_str(), actualUser.c_str(), 
                            actualPassword.c_str(), nullptr, actualPort, nullptr, 0)) {
        LogMessage::logMessage(ERROR, "Failed to connect to MySQL server: %s", mysql_error(temp));
        mysql_close(temp);
        return false;
    }

    // 2. 创建数据库
    std::string sql = "CREATE DATABASE IF NOT EXISTS " + actualDBName;
    if (mysql_query(temp, sql.c_str()) != 0) {
        LogMessage::logMessage(ERROR, "Failed to create database: %s", mysql_error(temp));
        mysql_close(temp);
        return false;
    }

    // 选择刚创建的数据库
    if (mysql_select_db(temp, actualDBName.c_str()) != 0) {
        LogMessage::logMessage(ERROR, "Failed to select database: %s", mysql_error(temp));
        mysql_close(temp);
        return false;
    }
    LogMessage::logMessage(INFO, "Database %s selected successfully", actualDBName.c_str());

    // 3. 创建表结构
    sql = R"(
        CREATE TABLE IF NOT EXISTS log_table (
        id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, 
        level ENUM('TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'FATAL') NOT NULL, 
        ip VARCHAR(45) NOT NULL,
        port SMALLINT UNSIGNED NOT NULL, 
        message TEXT, 
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_timestamp (timestamp),
        INDEX idx_level (level)
        ) ENGINE=InnoDB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci COMMENT='系统日志表';
    )";
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

        conn = mysql_real_connect(conn, actualHost.c_str(), actualUser.c_str(), 
                                  actualPassword.c_str(), actualDBName.c_str(), 
                                  actualPort, nullptr, 0);
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
