#include "EpollServer.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <string>
#include <unistd.h>
#include <termios.h>

using namespace EpollServerSpace;
static const int defaultPort = 8080;
static const int defaultEpollSize = 1024;
static const int defaultValue = -1;

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

static void usage(const char *proc) {
    std::cout << "Usage: " << proc << " <port>" << std::endl;
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);  // 忽略SIGPIPE信号
    signal(SIGINT, signalHandler);  // 捕获Ctrl+C信号
    
    int port;
    
    // 检查是否提供了端口号
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }
    
    port = atoi(argv[1]);
    
    // 安全输入数据库连接信息
    std::string username, password, dbname;
    
    std::cout << "请输入数据库用户名: ";
    std::getline(std::cin, username);
    
    password = securePasswordInput("请输入数据库密码: ");
    
    std::cout << "请输入数据库名称: ";
    std::getline(std::cin, dbname);
    
    // 创建服务器实例
    EpollServer *epollServer = new EpollServer(port, username, password, dbname);
    epollServer->ServerInit();
    epollServer->ServerStart();
    
    // 释放资源
    delete epollServer;
    
    return 0;
}