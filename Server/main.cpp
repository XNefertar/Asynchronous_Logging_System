#include "Server.hpp"
#include <memory>
#include <termios.h>

using namespace std;
using namespace Server;
static void Usage(string proc)
{
    cerr << "\nUsage:\n\t" << proc << " server_port\n\n";
}

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

int main(int argc, char* argv[])
{
    // 
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);

    port = atoi(argv[1]);
    
    // 安全输入数据库连接信息
    std::string username, password, dbname;
    
    std::cout << "请输入数据库用户名: ";
    std::getline(std::cin, username);
    
    password = securePasswordInput("请输入数据库密码: ");
    
    std::cout << "请输入数据库名称: ";
    std::getline(std::cin, dbname);

    std::unique_ptr<ServerTCP> tcpServer(new ServerTCP(port
        , username
        , password
        , dbname));

    tcpServer->init();
    tcpServer->run();

    return 0;
}