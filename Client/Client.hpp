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
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include "../Logger.hpp"
#include "../LogMessage/LogMessage.hpp"

enum class WebSocketState {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED
};

struct LogEntry {
    std::string level;
    std::string message;
    std::string timestamp;
};

class WebSocketClient
{
private:
    int _socketfd;
    int _port;

    std::ofstream _log_file;
    std::string _address;
    std::thread _reader;
    std::thread _sender;
    WebSocketState _state;
    
    std::queue<std::string> _messageQueue;
    std::mutex _queueMutex;
    std::condition_variable _queueCV;
    std::atomic<bool> _running;
    
    // 文件监控变量
    std::string _lastContent;
    std::string _logFilePath;

public:
    WebSocketClient(const std::string& address, int port, int socketfd = -1, 
                    const std::string& path = std::filesystem::current_path().string() + "/./Log/log.txt")
        : _address(address)
        , _port(port)
        , _socketfd(socketfd)
        , _state(WebSocketState::CLOSED)
        , _running(false)
        , _log_file(path, std::ios::app)
    {}
    
    ~WebSocketClient() { 
        _running = false;
        if (_reader.joinable()) _reader.join();
        if (_sender.joinable()) _sender.join();
        close(_socketfd); 
    }
    
    void createSocket() {
        _socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_socketfd < 0) {
            // std::cerr << "错误: 创建socket失败" << std::endl;
            _log_file << "[EORROR]创建socket失败" << std::endl;
            exit(1);
        }
        std::cout << "socket " << _socketfd << " 已创建" << std::endl;
    }

    void connectToServer() {
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_port);
        server_addr.sin_addr.s_addr = inet_addr(_address.c_str());

        if(connect(_socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            // std::cerr << "错误: 连接服务器失败" << std::endl;
            _log_file << "[EORROR]连接服务器失败" << std::endl;
            exit(1);
        }
        std::cout << "已连接到服务器 " << _address << ":" << _port << std::endl;
    }
    
    // 生成Base64编码
    std::string base64Encode(const unsigned char* input, int length) {
        BIO* bmem = BIO_new(BIO_s_mem());
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO_push(b64, bmem);
        BIO_write(b64, input, length);
        BIO_flush(b64);
        
        BUF_MEM* bptr;
        BIO_get_mem_ptr(b64, &bptr);
        
        std::string result(bptr->data, bptr->length);
        BIO_free_all(b64);
        
        return result;
    }

    // WebSocket握手
    bool performWebSocketHandshake() {
        std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // 固定key简化示例
        
        std::stringstream request;
        request << "GET /ws HTTP/1.1\r\n"
                << "Host: " << _address << ":" << _port << "\r\n"
                << "Upgrade: websocket\r\n"
                << "Connection: Upgrade\r\n"
                << "Sec-WebSocket-Key: " << key << "\r\n"
                << "Sec-WebSocket-Version: 13\r\n"
                << "\r\n";
                
        std::string requestStr = request.str();
        if (send(_socketfd, requestStr.c_str(), requestStr.length(), 0) < 0) {
            std::cerr << "错误: 发送WebSocket握手请求失败" << std::endl;
            return false;
        }
        
        char buffer[1024] = {0};
        int bytesReceived = recv(_socketfd, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cerr << "错误: 接收WebSocket握手响应失败" << std::endl;
            return false;
        }
        
        std::string response(buffer);
        if (response.find("101 Switching Protocols") != std::string::npos && 
            response.find("Upgrade: websocket") != std::string::npos) {
            _state = WebSocketState::OPEN;
            // std::cout << "WebSocket连接已建立" << std::endl;
            _log_file << "[INFO]WebSocket连接已建立" << std::endl;
            return true;
        }
        
        // std::cerr << "WebSocket握手失败. 服务器响应:\n" << response << std::endl;
        _log_file << "[EORROR]WebSocket握手失败. 服务器响应:\n" << response << std::endl;
        return false;
    }
    
    // 发送WebSocket帧
    bool sendFrame(const std::string& message) {
        if (_state != WebSocketState::OPEN) return false;
        
        // 简单的WebSocket帧构建 (未包含掩码)
        std::vector<unsigned char> frame;
        frame.push_back(0x81); // FIN=1, opcode=1 (文本)
        
        // 设置payload长度
        if (message.size() < 126) {
            frame.push_back(message.size());
        } else if (message.size() <= 0xFFFF) {
            frame.push_back(126);
            frame.push_back((message.size() >> 8) & 0xFF);
            frame.push_back(message.size() & 0xFF);
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back((message.size() >> (i * 8)) & 0xFF);
            }
        }
        
        // 添加消息内容
        for (char c : message) {
            frame.push_back(c);
        }
        
        // 发送帧
        if (send(_socketfd, frame.data(), frame.size(), 0) < 0) {
            // std::cerr << "错误: 发送WebSocket消息失败" << std::endl;
            _log_file << "[EORROR]发送WebSocket消息失败" << std::endl;
            return false;
        }
        _log_file << "[INFO]已发送WebSocket消息: " << message << std::endl;
        return true;
    }
    
    // 监听并处理从服务器接收的消息
    void startReceiving() {
        _reader = std::thread([this]() {
            _running = true;
            char buffer[4096];
            while (_running && _state == WebSocketState::OPEN) {
                memset(buffer, 0, sizeof(buffer));
                int bytesReceived = recv(_socketfd, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesReceived > 0) {
                    // 处理WebSocket帧 (简化版)
                    // 在实际应用中, 需要处理掩码、分片等
                    
                    // 简单提取消息内容
                    std::string message(buffer + 2, bytesReceived - 2);
                    std::cout << "收到服务器消息: " << message << std::endl;
                } else if (bytesReceived == 0) {
                    std::cout << "服务器关闭连接" << std::endl;
                    _state = WebSocketState::CLOSED;
                    break;
                } else {
                    std::cerr << "接收错误: " << strerror(errno) << std::endl;
                    break;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
    
    // 设置要监控的日志文件路径
    void setLogFilePath(const std::string& path) {
        _logFilePath = path;
    }
    
    // 发送队列中的消息
    void startSending() {
        _sender = std::thread([this]() {
            while (_running) {
                std::string message;
                {
                    std::unique_lock<std::mutex> lock(_queueMutex);
                    _queueCV.wait(lock, [this] { 
                        return !_messageQueue.empty() || !_running; 
                    });
                    
                    if (!_running) break;
                    
                    message = _messageQueue.front();
                    _messageQueue.pop();
                }
                
                if (sendFrame(message)) {
                    // std::cout << "已发送消息: " << message << std::endl;
                    _log_file << "[INFO]已发送消息: " << message << std::endl;
                }
            }
        });
    }
    
    // 添加消息到发送队列
    void queueMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _messageQueue.push(message);
        _queueCV.notify_one();
    }
    
    // 解析日志文件
    void monitorLogFile() {
        std::regex txtPattern(R"(\[(\w+)\]\{(.*?)\} at (.*))");
        std::regex htmlPattern(R"(class='([^']*)'>\[(\w+)\]\s+(.*?)\s+at\s+([\d:-]+\s[\d:]+))");
        
        bool isHtml = _logFilePath.find(".html") != std::string::npos;
        std::regex& pattern = isHtml ? htmlPattern : txtPattern;
        
        while (_running) {
            std::ifstream file(_logFilePath);
            if (!file.is_open()) {
                std::cerr << "无法打开日志文件: " << _logFilePath << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            file.close();
            
            // 检查文件是否有更新
            if (content != _lastContent) {
                // 处理新增内容
                std::string newContent;
                if (_lastContent.empty()) {
                    newContent = content;
                } else if (content.length() > _lastContent.length() && 
                           content.substr(0, _lastContent.length()) == _lastContent) {
                    newContent = content.substr(_lastContent.length());
                } else {
                    newContent = content;
                }
                
                // 解析日志条目
                std::smatch match;
                std::istringstream stream(newContent);
                std::string line;
                
                while (std::getline(stream, line)) {
                    if (std::regex_search(line, match, pattern)) {
                        std::string level = isHtml ? match[2] : match[1];
                        std::string message = isHtml ? match[3] : match[2];
                        std::string timestamp = isHtml ? match[4] : match[3];
                        
                        // 创建JSON格式的日志消息
                        std::string jsonLog = "{\n";
                        jsonLog += "\"level\": \"" + level + "\",\n";
                        jsonLog += "\"message\": \"" + message + "\",\n";
                        jsonLog += "\"timestamp\": \"" + timestamp + "\"\n";
                        jsonLog += "}";
                        
                        _log_file << "[INFO]解析到日志: " << jsonLog << std::endl;
                        queueMessage(jsonLog);
                    }
                }
                
                _lastContent = content;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void run() {
        // 执行WebSocket握手
        if (!performWebSocketHandshake()) {
            std::cerr << "WebSocket握手失败, 无法继续" << std::endl;
            return;
        }
        
        // // 确定日志文件路径
        // if (_logFilePath.empty()) {
        //     std::ifstream file("/tmp/option.flag");
        //     std::string option;
        //     if (file.good()) {
        //         file >> option;
        //         file.close();
                
        //         if (option == "0") {
        //             _logFilePath = std::filesystem::current_path().string() + "/log.txt";
        //         } else if (option == "1") {
        //             _logFilePath = std::filesystem::current_path().string() + "/log.html";
        //         } else {
        //             std::cerr << "无效的日志选项: " << option << std::endl;
        //             return;
        //         }
        //     } else {
        //         // 默认为文本日志
        //         _logFilePath = std::filesystem::current_path().string() + "/log.txt";
        //     }
            
        //     std::cout << "监控日志文件: " << _logFilePath << std::endl;
        // }

        _log_file << "[INFO]监控日志文件: " << _logFilePath << std::endl;
        
        // 启动接收线程
        startReceiving();
        
        // 启动发送线程
        startSending();
        
        // 监控日志文件
        _running = true;
        monitorLogFile();
    }
};

#endif // __CLIENT_HPP__