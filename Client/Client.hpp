#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <iostream>
#include <thread>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include <fstream>
#include <sstream>

class ClientTCP
{
private:
    int _socketfd;
    int _port;
    std::string _address;
    std::thread _reader;

public:
    ClientTCP(const std::string& address, int port, int socketfd = -1)
        : _address(address),
          _port(port),
          _socketfd(socketfd)
    {}
    ~ClientTCP() { close(_socketfd); }
    
    void createSocket()
    {
        _socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_socketfd < 0)
        {
            std::cerr << "Error creating socket" << std::endl;
            exit(1);
        }
        std::cout << "socket " << _socketfd << " created" << std::endl;
    }

    void connectToServer()
    {
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_port);
        server_addr.sin_addr.s_addr = inet_addr(_address.c_str());

        if(connect(_socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Error connecting to server" << std::endl;
            exit(1);
        }
        std::cout << "connected to server" << std::endl;
    }

    void run() {
        std::string last_content = ""; // 保存上次读取到的内容
        std::ifstream file;
        
        for(;;) {
            // 每次循环重新打开文件
            file.open("/home/xl/repositories/Asynchronous_Logging_System/build/log.txt");
            if (!file.is_open()) {
                std::cerr << "Error opening file" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            
            // 读取整个文件
            std::stringstream tmp_buffer;
            tmp_buffer << file.rdbuf();
            file.close(); // 关闭文件
            
            std::string current_content = tmp_buffer.str();
            
            // 文件内容有变化
            if (!current_content.empty() && current_content != last_content) {
                // 只发送新增的内容
                std::string new_content;
                if (last_content.empty()) {
                    new_content = current_content; // 首次读取，发送全部内容
                } else {
                    // 找出新增内容
                    if (current_content.length() > last_content.length() && 
                        current_content.substr(0, last_content.length()) == last_content) {
                        new_content = current_content.substr(last_content.length());
                    } else {
                        // 文件内容完全变化，发送全部
                        new_content = current_content;
                    }
                }
                
                // 发送新内容
                if (!new_content.empty()) {
                    write(_socketfd, new_content.c_str(), new_content.size());
                    
                    // 接收服务器响应
                    char buffer[1024] = {0};
                    ssize_t n = recv(_socketfd, buffer, sizeof(buffer) - 1, 0);
                    if (n < 0) {
                        std::cerr << "接收服务器响应失败. 错误码: " << errno << std::endl;
                        exit(1);
                    } else if (n == 0) {
                        std::cerr << "服务器已关闭连接" << std::endl;
                        exit(1);
                    }
                    buffer[n] = '\0';
                    
                    // 解析并显示JSON响应
                    std::cout << "服务器响应: " << std::endl;
                    
                    // 简单解析JSON (不使用外部库)
                    std::string response(buffer);
                    auto extractValue = [&response](const std::string& key) -> std::string {
                        size_t keyPos = response.find("\"" + key + "\":");
                        if (keyPos == std::string::npos) return "未找到";
                        
                        size_t valueStart = response.find("\"", keyPos + key.length() + 2);
                        if (valueStart == std::string::npos) {
                            // 可能是数字值
                            valueStart = response.find_first_not_of(" \t\n:", keyPos + key.length() + 1);
                            size_t valueEnd = response.find_first_of(",\n}", valueStart);
                            if (valueEnd == std::string::npos) return "解析错误";
                            return response.substr(valueStart, valueEnd - valueStart);
                        } else {
                            size_t valueEnd = response.find("\"", valueStart + 1);
                            if (valueEnd == std::string::npos) return "解析错误";
                            return response.substr(valueStart + 1, valueEnd - valueStart - 1);
                        }
                    };
                    
                    // 提取并打印关键信息
                    std::cout << "  状态: \033[1;32m" << extractValue("status") << "\033[0m" << std::endl;
                    std::cout << "  时间戳: " << extractValue("timestamp") << std::endl;
                    std::cout << "  消息大小: " << extractValue("message_size") << " 字节" << std::endl;
                    std::cout << "  服务器ID: " << extractValue("server_id") << std::endl;
                    std::cout << "-----------------------------" << std::endl;
                }
                
                last_content = current_content; // 更新上次内容
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};




#endif // __CLIENT_HPP__