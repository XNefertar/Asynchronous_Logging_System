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
#include <filesystem>
#include <regex>
#include <chrono>
#include "../Logger.hpp"
#include "../LogMessage/LogMessage.hpp"


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
        std::string option;
        while (true) {
            std::ifstream file("/tmp/option.flag");
            if (file.good()) {
                file >> option;
                std::remove("/tmp/option.flag"); // 删除文件
                break; // 读取到选项后退出循环
                // 处理 option
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "option = " << option << std::endl;
        switch(stoi(option)){
            case 0:
            {
                for(;;) {
                    // 每次循环重新打开文件
                    std::string path = std::filesystem::current_path().string() + "/log.txt";
                    file.open(path);
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
        
                            std::string path = std::filesystem::current_path().string() + "/Log/Client.txt";
                            LogMessage::setDefaultLogPath(path);
                            LogMessage::logMessage(INFO, "接收服务器响应: %s", buffer);
                            
                            // 简单解析JSON (不使用外部库)
                            // std::string response(buffer);
                            // auto extractValue = [&response](const std::string& key) -> std::string {
                            //     size_t keyPos = response.find("\"" + key + "\":");
                            //     if (keyPos == std::string::npos) return "未找到";
                                
                            //     size_t valueStart = response.find("\"", keyPos + key.length() + 2);
                            //     if (valueStart == std::string::npos) {
                            //         // 可能是数字值
                            //         valueStart = response.find_first_not_of(" \t\n:", keyPos + key.length() + 1);
                            //         size_t valueEnd = response.find_first_of(",\n}", valueStart);
                            //         if (valueEnd == std::string::npos) return "解析错误";
                            //         return response.substr(valueStart, valueEnd - valueStart);
                            //     } else {
                            //         size_t valueEnd = response.find("\"", valueStart + 1);
                            //         if (valueEnd == std::string::npos) return "解析错误";
                            //         return response.substr(valueStart + 1, valueEnd - valueStart - 1);
                            //     }
                            // };
        
                            std::string response(buffer);
                            auto extractValue = [&response](const std::string& key) -> std::string {
                                size_t keyPos = response.find("\"" + key + "\":");
                                if (keyPos == std::string::npos) return "未找到";
                        
                                size_t valueStart = response.find_first_not_of(" \t\n:", keyPos + key.length() + 2);
                                if (valueStart == std::string::npos) return "解析错误";
                        
                                // 处理字符串
                                if (response[valueStart] == '"') {
                                    size_t valueEnd = response.find("\"", valueStart + 1);
                                    if (valueEnd == std::string::npos) return "解析错误";
                                    return response.substr(valueStart + 1, valueEnd - valueStart - 1);
                                }
                        
                                // 处理数值、布尔值、null
                                size_t valueEnd = response.find_first_of(",\n}", valueStart);
                                if (valueEnd == std::string::npos) return "解析错误";
                        
                                std::string result = response.substr(valueStart, valueEnd - valueStart);
                                if (result == "null") return "空值";
                                if (result == "true") return "真";
                                if (result == "false") return "假";
                        
                                return result;
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
            case 1:
            {
                std::string log_path = std::filesystem::current_path().string() + "/Log/Client.txt";
                LogMessage::setDefaultLogPath(log_path);
                std::string path = std::filesystem::current_path().string() + "/log.html";
                // std::cout << "path = " << path << std::endl;
                // LogMessage::setDefaultLogPath(path);

                file.open(path);
                if(!file.is_open()) {
                    std::cerr << "Error opening file" << std::endl;
                    exit(1);
                }

                
                // 不同于TXT文件，HTML文件需要解析
                // 考虑使用正则表达式进行解析
                std::regex pattern(R"(<div\s+class=['"]log\s+(\w+)['"]>\[(.*?)\]\s+(.+?)\s+-\s+(.*?)<\/div>)");
                std::smatch match;
                std::string line;
                // 跳过文件开头的HTML标签
                // 读取到第一个<div>标签
                // 使用了seekg和tellg来定位到第一个<div>标签
                // 这样可以避免读取整个文件
                // 优化处理时间
                while (std::getline(file, line)) {
                    size_t pos = line.find("<div");
                    if (pos != std::string::npos) {
                        file.seekg(file.tellg() - std::streamoff(line.size() - pos)); // 定位到第一个<div>标签
                        break;
                    }
                }
                while (std::getline(file, line)) {
                    if (std::regex_search(line, match, pattern)) {
                        std::string level = match[1];
                        std::string timestamp = match[2];
                        std::string ip_port = match[3];
                        std::string message = match[4];
                    
                        // 发送解析后的内容
                        std::string new_content = "[" + level + "] " + timestamp + " - " + ip_port + " > " + message;
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
                        std::string response(buffer);
                        auto extractValue = [&response](const std::string& key) -> std::string {
                            size_t keyPos = response.find("\"" + key + "\":");
                            if (keyPos == std::string::npos) return "未找到";

                            size_t valueStart = response.find_first_not_of(" \t\n:", keyPos + key.length() + 2);
                            if (valueStart == std::string::npos) return "解析错误";

                            // 处理字符串
                            if (response[valueStart] == '"') {
                                size_t valueEnd = response.find("\"", valueStart + 1);
                                if (valueEnd == std::string::npos) return "解析错误";
                                return response.substr(valueStart + 1, valueEnd - valueStart - 1);
                            }

                            // 处理数值、布尔值、null
                            size_t valueEnd = response.find_first_of(",\n}", valueStart);
                            if (valueEnd == std::string::npos) return "解析错误";

                            std::string result = response.substr(valueStart, valueEnd - valueStart);
                            if (result == "null") return "空值";
                            if (result == "true") return "真";
                            if (result == "false") return "假";

                            return result;
                        };

                        // 提取并打印关键信息
                        std::cout << "  状态: \033[1;32m" << extractValue("status") << "\033[0m" << std::endl;
                        std::cout << "  时间戳: " << extractValue("timestamp") << std::endl;
                        std::cout << "  消息大小: " << extractValue("message_size") << " 字节" << std::endl;
                        std::cout << "  服务器ID: " << extractValue("server_id") << std::endl;
                        std::cout << "-----------------------------" << std::endl;
        
                        LogMessage::logMessage(INFO, "接收服务器响应: %s", buffer);
                        
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                file.close(); // 关闭文件
            }
            default:
                std::cerr << "Invalid log option" << std::endl;
                exit(1);
        }
        
    }
};




#endif // __CLIENT_HPP__