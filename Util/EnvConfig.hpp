#ifndef __ENV_CONFIG_HPP__
#define __ENV_CONFIG_HPP__

#include <string>

class EnvConfig {
public:
    // 获取字符串环境变量
    static std::string getEnvVar(const std::string& key, const std::string& defaultValue = "");
    
    // 获取整数环境变量
    static int getEnvVarInt(const std::string& key, int defaultValue = 0);
    
    // 数据库配置相关
    static std::string getDBHost();
    static int getDBPort();
    static std::string getDBUser();
    static std::string getDBPassword();
    static std::string getDBName();
    
    // 应用配置相关
    static int getAppPort();
    static std::string getLogLevel();
    
    // 验证必需的环境变量
    static bool validateRequiredEnvVars();
    
    // 打印配置信息（不包括密码）
    static void printConfig();
};

#endif // __ENV_CONFIG_HPP__