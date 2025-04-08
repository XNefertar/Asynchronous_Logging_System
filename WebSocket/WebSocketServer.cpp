#include "WebSocket.hpp"
#include "WebSocketApiHandlers.hpp"
#include "../EpollServer/EpollServer.hpp"
#include <signal.h>
#include <iostream>
#include <string>
#include <filesystem>

std::ofstream _log_file;
using namespace EpollServerSpace;

// 全局服务器实例
EpollServer* g_server = nullptr;

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
    
    // 默认值
    uint64_t port = 8080;
    std::string logPath = "./logs/server.log";
    std::string staticDir = "./static";
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoul(argv[++i]);
        } else if (arg == "--log" && i + 1 < argc) {
            logPath = argv[++i];
        } else if (arg == "--static" && i + 1 < argc) {
            staticDir = argv[++i];
        } else if (arg == "--help") {
            std::cout << "用法: " << argv[0] << " [选项]\n"
                      << "选项:\n"
                      << "  --port 端口     服务器端口 (默认: 8080)\n"
                      << "  --log 路径      日志文件路径 (默认: ./logs/server.log)\n"
                      << "  --static 路径   静态文件目录 (默认: ./static)\n"
                      << "  --help          显示帮助信息\n";
            return 0;
        }
    }
    
    // 获取数据库信息
    std::string username, password, dbname;
    
    std::cout << "请输入数据库用户名: ";
    std::getline(std::cin, username);
    
    std::cout << "请输入数据库密码: ";
    std::getline(std::cin, password);
    
    std::cout << "请输入数据库名称: ";
    std::getline(std::cin, dbname);
    
    // 创建并初始化服务器
    g_server = new EpollServer(port, username, password, dbname, logPath);
    setGlobalServerReference(g_server);
    
    // 设置静态文件目录
    g_server->setStaticFilesDir(staticDir);
    
    // 添加 API 端点处理器
    g_server->addGetHandler("/api/logs", handleLogsApi);
    g_server->addGetHandler("/api/stats", handleStatsApi);
    
    // 设置 WebSocket 处理器
    g_server->setWebSocketHandler("/ws", handleWebSocketMessage);
    
    // 初始化并启动服务器
    g_server->ServerInit();
    
    std::cout << "服务器运行中，端口: " << port << std::endl;
    std::cout << "静态文件目录: " << staticDir << std::endl;
    std::cout << "日志文件: " << logPath << std::endl;
    
    g_server->ServerStart();  // 阻塞，直到服务器停止
    
    return 0;
}