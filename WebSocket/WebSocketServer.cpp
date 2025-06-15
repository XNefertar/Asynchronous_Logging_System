#include "WebSocket.hpp"
#include "WebSocketApiHandlers.hpp"
#include "../EpollServer/EpollServer.hpp"
#include "../Util/Daemon.hpp"
#include "../Util/EnvConfig.hpp"
#include <signal.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <termios.h>

std::ofstream _log_file;
using namespace EpollServerSpace;

// 全局服务器实例
EpollServer* g_server = nullptr;

// 安全读取密码函数，实现类似 read -s 的功能
std::string securePasswordInput(const std::string& prompt) {
    std::string password;
    struct termios old_settings, new_settings;
    
    // 获取当前终端设置
    tcgetattr(STDIN_FILENO, &old_settings);
    new_settings = old_settings;
    
    // 关闭回显显示
    new_settings.c_lflag &= ~ECHO;
    
    // 应用新设置
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    
    // 提示用户输入
    std::cout << prompt;
    std::getline(std::cin, password);
    std::cout << std::endl; // 输入完成后换行
    
    // 恢复终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
    
    return password;
}


// 信号处理函数
void signalHandler(int signum) {
    if (g_server) {
        std::cout << "接收到信号 " << signum << ", 关闭中..." << std::endl;
        delete g_server;
        exit(signum);
    }
}

int main(int argc, char* argv[]) {
    // 注册信号处理器
    signal(SIGINT, ::signalHandler);
    signal(SIGTERM, ::signalHandler);
    
    // 首先检查是否在容器环境中运行（通过环境变量判断）
    bool useEnvConfig = (std::getenv("DB_HOST") != nullptr);
    
    // 默认值
    uint64_t port = 8080;
    std::string logPath = "./logs/server.log";
    std::string staticDir = "./static";
    std::string username, password, dbname;
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoul(argv[++i]);
        } else if (arg == "--log" && i + 1 < argc) {
            logPath = argv[++i];
        } else if (arg == "--static" && i + 1 < argc) {
            staticDir = argv[++i];
        } else if (arg == "--env") {
            useEnvConfig = true;  // 强制使用环境变量
        } else if (arg == "--help") {
            std::cout << "用法: " << argv[0] << " [选项]\n"
                      << "选项:\n"
                      << "  --port 端口     服务器端口 (默认: 8080)\n"
                      << "  --log 路径      日志文件路径 (默认: ./logs/server.log)\n"
                      << "  --static 路径   静态文件目录 (默认: ./static)\n"
                      << "  --env           强制使用环境变量配置数据库\n"
                      << "  --help          显示帮助信息\n"
                      << "\n环境变量:\n"
                      << "  DB_HOST         数据库主机地址\n"
                      << "  DB_PORT         数据库端口 (默认: 3306)\n"
                      << "  DB_USER         数据库用户名\n"
                      << "  DB_PASSWORD     数据库密码\n"
                      << "  DB_NAME         数据库名称\n"
                      << "  APP_PORT        应用服务端口 (默认: 8080)\n";
            return 0;
        }
    }
    
    // 获取数据库信息
    if (useEnvConfig) {
        std::cout << "[信息] 使用环境变量配置数据库连接..." << std::endl;
        
        // 验证环境变量
        if (!EnvConfig::validateRequiredEnvVars()) {
            std::cerr << "[错误] 环境变量验证失败，请设置必需的环境变量" << std::endl;
            return 1;
        }
        
        // 从环境变量获取配置
        username = EnvConfig::getDBUser();
        password = EnvConfig::getDBPassword();
        dbname = EnvConfig::getDBName();
        port = EnvConfig::getAppPort();  // 如果环境变量中有端口配置
        
        // 打印配置信息
        EnvConfig::printConfig();
    } else {
        std::cout << "[信息] 使用交互式输入配置数据库连接..." << std::endl;
        
        std::cout << "请输入数据库用户名: ";
        std::getline(std::cin, username);
        
        password = securePasswordInput("请输入数据库密码: ");
        
        std::cout << "请输入数据库名称: ";
        std::getline(std::cin, dbname);
    }
    
    // 创建并初始化服务器
    try {
        g_server = new EpollServer(port, username, password, dbname, logPath);
        setGlobalServerReference(g_server);
        
        // 设置静态文件目录
        g_server->setStaticFilesDir(staticDir);
        
        // 添加 API 端点处理器
        g_server->addGetHandler("/api/logs", handleLogsApi);
        g_server->addGetHandler("/api/stats", handleStatsApi);
        g_server->addGetHandler("/api/download-log", handleLogFileDownload);

        // 设置 WebSocket 处理器
        g_server->setWebSocketHandler("/ws", handleWebSocketMessage);
        
        // 初始化并启动服务器
        g_server->ServerInit();
        
        std::cout << "[成功] 服务器启动成功!" << std::endl;
        std::cout << "  - 服务器端口: " << port << std::endl;
        std::cout << "  - 静态文件目录: " << staticDir << std::endl;
        std::cout << "  - 日志文件: " << logPath << std::endl;
        
        g_server->ServerStart();  // 阻塞，直到服务器停止
        
    } catch (const std::exception& e) {
        std::cerr << "[错误] 服务器启动失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}