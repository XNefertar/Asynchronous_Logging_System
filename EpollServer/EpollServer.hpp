#ifndef __EPOLLSERVER_HPP__
#define __EPOLLSERVER_HPP__

#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <map>
#include "../Util/Sock.hpp"

namespace EpollServerSpace{

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
        uint64_t _port;
        struct epoll_event* _events;
        int _epollfd;
        int _listenfd;
        std::map<int, ClientSession> _sessions; // 存储客户端会话信息
    
    public:
        EpollServer(uint64_t port)
            : _port(port)
            , _epollfd(-1)
            , _listenfd(-1) 
        {
            _events = new epoll_event[defaultEpollSize];
        }
    
        ~EpollServer() 
        {
            if(_listenfd != defaultValue)
                close(_listenfd);
            if(_epollfd != defaultValue)
                close(_epollfd);
            if(_events != nullptr)
                delete[] _events;
        }
    
        void ServerInit(){
            _listenfd = Sock::Socket();
            Sock::Bind(_listenfd, _port);
            Sock::Listen(_listenfd);
    
            _epollfd = epoll_create(1);
            if(_epollfd < 0){
                std::cerr << "\033[1;31m[错误]\033[0m epoll_create 失败: " << strerror(errno) << std::endl;
                exit(1);
            }
    
            struct epoll_event ev;
            ev.data.fd = _listenfd;
            ev.events = EPOLLIN;
            if(epoll_ctl(_epollfd, EPOLL_CTL_ADD, _listenfd, &ev) < 0){
                std::cerr << "\033[1;31m[错误]\033[0m epoll_ctl 添加监听套接字失败: " << strerror(errno) << std::endl;
                exit(1);
            }
            
            std::cout << "\033[1;32m[启动]\033[0m Epoll服务器已初始化，监听端口: " << _port << std::endl;
        }
    
        void ServerStart(){
            std::cout << "\033[1;34m[运行]\033[0m 服务器开始运行，等待连接..." << std::endl;
            for(;;){
                int ReadyNum = epoll_wait(_epollfd, _events, defaultEpollSize, timeout);
                switch(ReadyNum){
                    case -1:
                        std::cerr << "\033[1;31m[错误]\033[0m epoll_wait 失败: " << strerror(errno) << std::endl;
                        break;
                    case 0:
                        // timeout, 正常情况，什么都不做
                        break;
                    default:
                        HandleEvents(ReadyNum);
                }
            }
        }
    
        void HandleEvents(int ReadyNum){
            for(int i = 0; i < ReadyNum; ++i){
                int sockfd = _events[i].data.fd;
                if(sockfd == _listenfd && (_events[i].events & EPOLLIN)){
                    // 新客户端连接
                    std::string ip;
                    uint16_t port;
                    int connfd = Sock::Accept(_listenfd, ip, port);
                    if(connfd < 0) {
                        std::cerr << "\033[1;31m[错误]\033[0m 接受连接失败: " << strerror(errno) << std::endl;
                        continue;
                    }
                    
                    // 创建新的客户端会话记录
                    ClientSession session = {ip, port, time(nullptr), 0, 0};
                    _sessions[connfd] = session;
                    
                    // 输出连接信息
                    std::cout << "\033[1;34m[连接建立]\033[0m 客户端 " << ip << ":" << port 
                              << " 已连接，分配套接字: " << connfd << std::endl;
    
                    struct epoll_event ev;
                    ev.data.fd = connfd;
                    ev.events = EPOLLIN;
                    if(epoll_ctl(_epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0){
                        std::cerr << "\033[1;31m[错误]\033[0m 添加客户端到 epoll 失败: " << strerror(errno) << std::endl;
                        close(connfd);
                        _sessions.erase(connfd);
                    }
                }
                else if(_events[i].events & EPOLLIN){
                    // 处理已连接客户端的数据
                    char buffer[1024] = {0};
                    ssize_t n = read(sockfd, buffer, sizeof(buffer) - 1);
                    
                    if(n < 0){
                        std::cerr << "\033[1;31m[错误]\033[0m 客户端 " << _sessions[sockfd].ip << ":" << _sessions[sockfd].port
                                  << " 读取失败: " << strerror(errno) << std::endl;
                        close(sockfd);
                        epoll_ctl(_epollfd, EPOLL_CTL_DEL, sockfd, nullptr);
                        _sessions.erase(sockfd);
                    }
                    else if(n == 0){
                        // 客户端断开连接
                        time_t session_duration = time(nullptr) - _sessions[sockfd].connect_time;
                        std::cout << "\033[1;33m[连接终止]\033[0m 客户端 " << _sessions[sockfd].ip << ":" << _sessions[sockfd].port
                                  << " 断开连接" << std::endl;
                        std::cout << "\033[1;36m[会话统计]\033[0m 总接收: " 
                                  << _sessions[sockfd].total_bytes << " 字节, 消息数: " << _sessions[sockfd].message_count 
                                  << ", 持续时间: " << session_duration << " 秒" << std::endl;
                        
                        close(sockfd);
                        epoll_ctl(_epollfd, EPOLL_CTL_DEL, sockfd, nullptr);
                        _sessions.erase(sockfd);
                        continue;
                    }
                    else{
                        buffer[n] = '\0';  // 确保字符串正确终止
                        
                        // 更新会话统计
                        _sessions[sockfd].total_bytes += n;
                        _sessions[sockfd].message_count++;
                        
                        // 格式化当前时间
                        auto now = std::chrono::system_clock::now();
                        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                        char time_buffer[64];
                        std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
                        
                        // 打印收到的消息摘要
                        std::string message_preview;
                        if (n > 50) {
                            message_preview = std::string(buffer).substr(0, 47) + "...";
                        } else {
                            message_preview = std::string(buffer);
                        }
                        
                        std::cout << "\033[1;32m[消息接收]\033[0m [" << time_buffer << "] " 
                                  << _sessions[sockfd].ip << ":" << _sessions[sockfd].port << " > " << message_preview 
                                  << " (" << n << " 字节)" << std::endl;
                        
                        // 创建结构化的JSON响应
                        std::string response = "{\n";
                        response += "  \"status\": \"success\",\n";
                        response += "  \"timestamp\": \"" + std::string(time_buffer) + "\",\n";
                        response += "  \"message_size\": " + std::to_string(n) + ",\n";
                        response += "  \"server_id\": \"epoll_server_01\",\n";
                        response += "  \"client\": \"" + _sessions[sockfd].ip + ":" + std::to_string(_sessions[sockfd].port) + "\",\n";
                        response += "  \"message_number\": " + std::to_string(_sessions[sockfd].message_count) + ",\n";
                        response += "  \"total_bytes\": " + std::to_string(_sessions[sockfd].total_bytes) + "\n";
                        response += "}";
                        
                        // 发送响应
                        int bytes_sent = write(sockfd, response.c_str(), response.size());
                        if (bytes_sent < 0) {
                            std::cerr << "\033[1;31m[错误]\033[0m 发送响应失败: " << strerror(errno) << std::endl;
                        } else {
                            std::cout << "\033[1;36m[响应发送]\033[0m 响应大小: " << bytes_sent << " 字节" << std::endl;
                        }
                    }
                }
            }
        }
    };
}

#endif // __EPOLLSERVER_HPP__