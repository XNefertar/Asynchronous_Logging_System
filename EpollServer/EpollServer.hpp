#ifndef __EPOLLSERVER_HPP__
#define __EPOLLSERVER_HPP__

#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <fstream>
#include <queue>
#include <map>
#include <filesystem>
#include "../Util/Sock.hpp"
#include "../MySQL/SqlConnPool.hpp"


namespace EpollServerSpace{

    void signalHandler(int signum);
    
    static const uint64_t defaultPort       = 8080;
    static const int      defaultEpollSize  = 1024;
    static const int      defaultValue      = -1;
    static const int      timeout           = -1;   // epoll_wait timeout，-1表示阻塞等待
    
    // 客户端会话信息结构
    struct ClientSession {
        std::string ip;
        uint16_t port;
        time_t connect_time;
        uint64_t total_bytes;
        uint64_t message_count;
    };
    
    class EpollServer{
    private:
        uint64_t                        _port;
        std::string                     _defaultUserName;
        std::string                     _defaultPassword;
        std::string                     _defaultDBName;

        int                             _listenfd;
        int                             _epollfd;
        std::ofstream                   _log_file;
        struct epoll_event*             _events;
        std::map<int, ClientSession>    _sessions; // 存储客户端会话信息
        std::string                     _defaultIPAddress;
        int                             _defaultPort;
        int                             _defaultMaxConn;


    public:
        // TODO
        // 路径硬编码问题
        EpollServer( uint64_t port
                   , std::string defaultUserName
                   , std::string defaultPassword
                   , std::string defaultDBName
                   , std::string path = std::filesystem::current_path().string() + "/Log/Epoll_Server.txt"
                   , std::string defaultIPAddress = "localhost"
                   , int defaultPort = 3306
                   , int defaultMaxConn = 10);
    
        ~EpollServer();
    
        void ServerInit();
    
        void ServerStart();
    
        void HandleEvents(int ReadyNum);

    };
}

#endif // __EPOLLSERVER_HPP__