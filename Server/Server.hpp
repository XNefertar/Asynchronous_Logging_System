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
#include "../Logger.hpp"

// 客户端从本地文件读取日志信息后传到远端服务器
// 服务器接收日志信息并返回确认信息给客户端
// 目前只支持单个客户端连接
namespace Server{
    
    void socketIO(int socket){
        char buffer[1024] = {0}; // 初始化缓冲区
        for (;;)
        {
            memset(buffer, 0, sizeof(buffer)); // 每次读取前清空缓冲区

            int valread = read(socket, buffer, sizeof(buffer));
            if (valread == 0)
            {
                std::cout << "Client disconnected" << std::endl;
                break;
            }
            else if (valread == -1)
            {
                std::cerr << "Read error. errno: " << errno << std::endl;
                break;
            }

            std::cout << "Client # " << buffer << std::endl;

            std::string message = std::string(buffer) + " server[received]";
            write(socket, message.c_str(), message.size());
            std::this_thread::sleep_for(std::chrono::seconds(1));
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