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
                    const std::string& path = std::filesystem::current_path().string() + "/./Log/log.txt");
    
    ~WebSocketClient();
    
    void createSocket();

    void connectToServer();
    
    // 生成Base64编码
    std::string base64Encode(const unsigned char* input, int length);

    // WebSocket握手
    bool performWebSocketHandshake();
    
    // 发送WebSocket帧
    bool sendFrame(const std::string& message);
    
    // 监听并处理从服务器接收的消息
    void startReceiving();
    
    // 设置要监控的日志文件路径
    void setLogFilePath(const std::string& path);
    
    // 发送队列中的消息
    void startSending();
    
    // 添加消息到发送队列
    void queueMessage(const std::string& message);
    
    // 解析日志文件
    void monitorLogFile();

    void run();
};

#endif // __CLIENT_HPP__