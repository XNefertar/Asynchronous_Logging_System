#include "EnvConfig.hpp"
#include <cstdlib>
#include <iostream>

// 获取字符串环境变量
std::string EnvConfig::getEnvVar(const std::string& key, const std::string& defaultValue) {
    const char* val = std::getenv(key.c_str());
    return val ? std::string(val) : defaultValue;
}

// 获取整数环境变量
int EnvConfig::getEnvVarInt(const std::string& key, int defaultValue) {
    const char* val = std::getenv(key.c_str());
    return val ? std::atoi(val) : defaultValue;
}

// 数据库配置相关实现
std::string EnvConfig::getDBHost() {
    return getEnvVar("DB_HOST", "192.144.236.38");
}

int EnvConfig::getDBPort() {
    return getEnvVarInt("DB_PORT", 3306);
}

std::string EnvConfig::getDBUser() {
    return getEnvVar("DB_USER", "root");
}

std::string EnvConfig::getDBPassword() {
    return getEnvVar("DB_PASSWORD", "");
}

std::string EnvConfig::getDBName() {
    return getEnvVar("DB_NAME", "logging_db");
}

// 应用配置相关实现
int EnvConfig::getAppPort() {
    return getEnvVarInt("APP_PORT", 8080);
}

std::string EnvConfig::getLogLevel() {
    return getEnvVar("LOG_LEVEL", "INFO");
}

// 验证必需的环境变量
bool EnvConfig::validateRequiredEnvVars() {
    bool valid = true;
    
    if (getDBHost().empty()) {
        std::cerr << "[错误] 环境变量 DB_HOST 未设置" << std::endl;
        valid = false;
    }
    
    if (getDBUser().empty()) {
        std::cerr << "[错误] 环境变量 DB_USER 未设置" << std::endl;
        valid = false;
    }
    
    if (getDBPassword().empty()) {
        std::cerr << "[警告] 环境变量 DB_PASSWORD 未设置，将使用空密码" << std::endl;
    }
    
    if (getDBName().empty()) {
        std::cerr << "[错误] 环境变量 DB_NAME 未设置" << std::endl;
        valid = false;
    }
    
    return valid;
}

// 打印配置信息（不包括密码）
void EnvConfig::printConfig() {
    std::cout << "[配置信息] 数据库连接配置:" << std::endl;
    std::cout << "  - 主机 (DB_HOST): " << getDBHost() << std::endl;
    std::cout << "  - 端口 (DB_PORT): " << getDBPort() << std::endl;
    std::cout << "  - 用户 (DB_USER): " << getDBUser() << std::endl;
    std::cout << "  - 数据库 (DB_NAME): " << getDBName() << std::endl;
    std::cout << "  - 应用端口 (APP_PORT): " << getAppPort() << std::endl;
    std::cout << "  - 日志级别 (LOG_LEVEL): " << getLogLevel() << std::endl;
}