#ifndef __LOG_TEMPLATES_HPP__
#define __LOG_TEMPLATES_HPP__

#include <string>
#include <vector>
#include <random>
#include <map>  // 添加map头文件

class LogTemplates {
private:
    // 定义不同类型的模板
    static const std::vector<std::string> authTemplates;
    static const std::vector<std::string> databaseTemplates;
    static const std::vector<std::string> networkTemplates;
    static const std::vector<std::string> systemTemplates;
    
    // 随机数生成器
    static std::mt19937& getGenerator();
public:
    // 根据类型获取随机模板
    static std::string getRandomTemplate(const std::string& type);
    
    // 添加占位符替换方法
    static std::string replacePlaceholders(const std::string& templateStr, 
                                         const std::map<std::string, std::string>& values);
};
#endif // __LOG_TEMPLATES_HPP__