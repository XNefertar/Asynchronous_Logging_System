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
#include <chrono>
#include <ctime>
#include "../Logger.hpp"
#include "../LogMessage.hpp"

// 客户端从本地文件读取日志信息后传到远端服务器
// 服务器接收日志信息并返回确认信息给客户端
// 目前只支持单个客户端连接
namespace Server{
    
    void socketIO(int socket){
        char buffer[1024] = {0}; // 初始化缓冲区
        
        // 获取客户端信息
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        getpeername(socket, (struct sockaddr*)&client_addr, &addr_len);
        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port = ntohs(client_addr.sin_port);
        
        // 记录连接信息
        std::cout << "\033[1;34m[连接建立]\033[0m 客户端 " << client_ip << ":" << client_port << " 已连接" << std::endl;
        
        // 消息统计
        uint64_t total_bytes = 0;
        uint64_t message_count = 0;
        time_t start_time = time(nullptr);
        
        for (;;)
        {
            memset(buffer, 0, sizeof(buffer)); // 每次读取前清空缓冲区

            int valread = read(socket, buffer, sizeof(buffer));
            if (valread == 0)
            {
                std::cout << "\033[1;33m[连接终止]\033[0m 客户端 " << client_ip << ":" << client_port << " 断开连接" << std::endl;
                // 显示会话统计信息
                time_t session_duration = time(nullptr) - start_time;
                std::cout << "\033[1;36m[会话统计]\033[0m 总接收: " 
                          << total_bytes << " 字节, 消息数: " << message_count 
                          << ", 持续时间: " << session_duration << " 秒" << std::endl;
                break;
            }
            else if (valread == -1)
            {
                std::cerr << "\033[1;31m[错误]\033[0m 读取失败. 错误码: " << errno << " (" << strerror(errno) << ")" << std::endl;
                break;
            }

            // 更新统计信息
            total_bytes += valread;
            message_count++;
            
            // 格式化当前时间
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            char time_buffer[64];
            std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
            
            // 打印收到的消息摘要
            std::string message_preview;
            if (valread > 50) {
                message_preview = std::string(buffer).substr(0, 47) + "...";
            } else {
                message_preview = std::string(buffer);
            }
            
            std::cout << "\033[1;32m[消息接收]\033[0m [" << time_buffer << "] " 
                      << client_ip << ":" << client_port << " > " << message_preview 
                      << " (" << valread << " 字节)" << std::endl;
            
            // 创建更结构化的响应消息
            std::string response = "{\n";
            response += "  \"status\": \"success\",\n";
            response += "  \"timestamp\": \"" + std::string(time_buffer) + "\",\n";
            response += "  \"message_size\": " + std::to_string(valread) + ",\n";
            response += "  \"server_id\": \"log_server_01\",\n";
            response += "  \"client\": \"" + client_ip + ":" + std::to_string(client_port) + "\",\n";
            response += "  \"message_number\": " + std::to_string(message_count) + ",\n";
            response += "  \"total_bytes\": " + std::to_string(total_bytes) + "\n";
            response += "}";
            
            // 发送响应
            LogMessage::setDefaultLogPath("/home/xl/repositories/Asynchronous_Logging_System/Server/serverLog.txt");
            LogMessage::logMessage(INFO, "接收客户端消息: %s", buffer);
            int bytes_sent = write(socket, response.c_str(), response.size());
            std::cout << "\033[1;36m[响应发送]\033[0m 响应大小: " << bytes_sent << " 字节" << std::endl;
        }
        
        close(socket); // 关闭套接字
    }

    // initialize socket
    // bind socket
    // listen socket
    class ServerTCP{
    private:
        int _socketfd;
        int _port;

    public:
        ServerTCP(int port, int socketfd = -1)
            : _port(port) {}
        ~ServerTCP() { close(_socketfd); }

        void init(){
            _socketfd = socket(AF_INET, SOCK_STREAM, 0);
            if (_socketfd < 0)
            {
                std::cerr << "Error creating socket" << std::endl;
                exit(1);
            }

            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(_port);

            // 设置地址复用
            int opt = 1;
            if (setsockopt(_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
                std::cerr << "Error setting socket options" << std::endl;
                exit(1);
            }
            // 设置端口复用
            if (setsockopt(_socketfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0){
                std::cerr << "Error setting socket options" << std::endl;
                exit(1);
            }
            
            // bind
            if(bind(_socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
                std::cerr << "Error binding socket" << std::endl;
                exit(1);
            }

            // listen
            if(listen(_socketfd, 5) < 0){
                std::cerr << "Error listening on socket" << std::endl;
                exit(1);
            }
        }

        void run(){
            // 连接客户端
            for (;;)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);
                int client_socket = accept(_socketfd, (struct sockaddr *)&client_addr, &client_addr_size);
                if (client_socket < 0)
                {
                    std::cerr << "Error accepting client" << std::endl;
                    continue;
                }

                std::thread client_thread(socketIO, client_socket);
                client_thread.detach(); // 分离线程，允许其独立运行
            }
        }

    };

    
}



#endif // __Server_hpp__