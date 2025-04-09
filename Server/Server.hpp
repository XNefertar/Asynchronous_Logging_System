#ifndef __Server_hpp__
#define __Server_hpp__

#include <iostream>
#include <string>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <thread>
#include <filesystem>
#include <chrono>
#include <regex>
#include <ctime>
#include "../MySQL/SqlConnPool.hpp"
#include "../Logger.hpp"
#include "../LogMessage/LogMessage.hpp"

// 客户端从本地文件读取日志信息后传到远端服务器
// 服务器接收日志信息并返回确认信息给客户端
// 目前只支持单个客户端连接
namespace Server{

    // initialize socket
    // bind socket
    // listen socket
    void socketIO(int socket);
    int LeveltoInt(const std::string& level);
    class ServerTCP{
    private:
        int         _socketfd;
        int         _port;
        std::string _defaultUserName;
        std::string _defaultPassword;
        std::string _defaultDBName;
        std::string _defaultIPAddress;
        int         _defaultPort;
        int         _defaultMaxConn;

    public:
        ServerTCP(uint64_t port
                , std::string defaultUserName
                , std::string defaultPassword
                , std::string defaultDBName
                , uint64_t    defaultPort = 3306
                , std::string defaultIPAddress = "localhost"
                , int defaultMaxConn = 10);
        ~ServerTCP();
        void init();
        void run();
    };
}

#endif // __Server_hpp__