#include "Client.hpp"

ClientTCP::ClientTCP(const std::string& address, int port, int socketfd)
        : _address(address),
          _port(port),
          _socketfd(socketfd)
    {}

ClientTCP::~ClientTCP() { close(_socketfd); }

int ClientTCP::getPort() const {return _port;}
int ClientTCP::getSocketfd() const {return _socketfd;}
std::string ClientTCP::getHost() const {return _address;}

// 连接到服务器，返回是否成功
bool ClientTCP::Connect() {
    try {
        // 如果套接字未创建，先创建套接字
        if (_socketfd < 0) {
            createSocket();
        }
        
        // 设置服务器地址
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(_port);
        serverAddr.sin_addr.s_addr = inet_addr(_address.c_str());
        
        // 连接到服务器
        if (::connect(_socketfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "连接服务器失败: " << strerror(errno) << std::endl;
            return false;
        }
        
        return true;
    } catch (std::exception& e) {
        std::cerr << "连接异常: " << e.what() << std::endl;
        return false;
    }
}

// 发送数据，返回发送的字节数
ssize_t ClientTCP::sendData(const std::string& data) {
    if (_socketfd < 0) {
        std::cerr << "发送前套接字无效" << std::endl;
        return -1;
    }
    
    ssize_t bytesSent = send(_socketfd, data.c_str(), data.length(), 0);
    if (bytesSent < 0) {
        std::cerr << "发送数据失败: " << strerror(errno) << std::endl;
    }
    
    return bytesSent;
}

// 接收数据，返回接收到的字符串
std::string ClientTCP::receiveData() {
    if (_socketfd < 0) {
        std::cerr << "接收前套接字无效" << std::endl;
        return "";
    }
    
    char buffer[4096] = {0};
    ssize_t bytesRead = recv(_socketfd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cerr << "连接已关闭" << std::endl;
        } else {
            std::cerr << "接收数据失败: " << strerror(errno) << std::endl;
        }
        return "";
    }
    
    return std::string(buffer, bytesRead);
}

// 断开连接
void ClientTCP::disConnect() {
    if (_socketfd >= 0) {
        close(_socketfd);
        _socketfd = -1;
    }
    
    // 如果读取线程正在运行，等待它结束
    if (_reader.joinable()) {
        _reader.join();
    }
}

void ClientTCP::createSocket()
{
    _socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketfd < 0)
    {
        std::cerr << "Error creating socket" << std::endl;
        exit(1);
    }
    std::cout << "socket " << _socketfd << " created" << std::endl;
}

void ClientTCP::connectToServer()
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

void ClientTCP::run() {
    std::string last_content = ""; // 保存上次读取到的内容
    std::ifstream file;
    
    // while(ConfigSpace::ConfigManager::getInstance()->isInitialized() == false){
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    //     std::cout << "等待配置初始化..." << std::endl;
    // }

    ConfigSpace::SharedConfigRAII configRAII;
    auto& configManager = configRAII.get();

    while(configManager.isInitialized() == false){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "等待配置初始化..." << std::endl;
    }
    if(configManager.isTextFormat()) {
        for(;;) {
            // 每次循环重新打开文件
            std::string path = std::filesystem::current_path().string() + "/log.txt";
            file.open(path);
            if (!file.is_open()) {
                std::cerr << "Error opening file" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            
            // 客户端读取格式要求: 只需要提供日志等级和消息内容即可
            // 格式: [日志等级]{消息内容}, 先后顺序无所谓，只需要对应的内容有对应的包围即可
            
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
                    // 解析JSON字段
                    // {"level": "INFO", "message": "This is a log message"}

                    std::string send_message(new_content);
                    auto extractValueSender = [&send_message](const std::string& key) -> std::string {
                        size_t keyPos = send_message.find("\"" + key + "\":");
                        if (keyPos == std::string::npos) return "未找到";
                        
                        size_t valueStart = send_message.find_first_not_of(" \t\n:", keyPos + key.length() + 2);
                        if (valueStart == std::string::npos) return "解析错误";
                        
                        // 处理字符串
                        if (send_message[valueStart] == '"') {
                            size_t valueEnd = send_message.find("\"", valueStart + 1);
                            if (valueEnd == std::string::npos) return "解析错误";
                            return send_message.substr(valueStart + 1, valueEnd - valueStart - 1);
                        }
                        
                        // 处理数值、布尔值、null
                        size_t valueEnd = send_message.find_first_of(",\n}", valueStart);
                        if (valueEnd == std::string::npos) return "解析错误";
                        
                        std::string result = send_message.substr(valueStart, valueEnd - valueStart);
                        if (result == "null") return "空值";
                        if (result == "true") return "真";
                        if (result == "false") return "假";
                        
                        return result;
                    };
                    std::string logLevel = extractValueSender("level");
                    std::string message_content = extractValueSender("message");

                    std::string message = "[" + logLevel + "]{" + message_content + "}";
                    std::cout << "发送消息: " << message << std::endl;
                    write(_socketfd, message.c_str(), message.size());
                    
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
                    auto extractValueResponse = [&response](const std::string& key) -> std::string {
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
                    std::cout << "  状态: \033[1;32m" << extractValueResponse("status") << "\033[0m" << std::endl;
                    std::cout << "  时间戳: " << extractValueResponse("timestamp") << std::endl;
                    std::cout << "  消息大小: " << extractValueResponse("message_size") << " 字节" << std::endl;
                    std::cout << "  服务器ID: " << extractValueResponse("server_id") << std::endl;
                    std::cout << "-----------------------------" << std::endl;
                }
                
                last_content = current_content; // 更新上次内容
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else
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

        // HTML文件解析
        // 考虑使用正则表达式进行解析
        std::regex pattern(R"(class='([^']+)'>\[(\w+)\]\s+(.*?)\s+at\s+(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}))");
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
                std::string log_class = match[1];
                std::string level = match[2];
                std::string message = match[3];
                std::string timestamp = match[4];
                LogMessage::logMessage(INFO, "解析到日志等级: %s, 消息: %s, 时间戳: %s", level.c_str(), message.c_str(), timestamp.c_str());
                std::cout << "解析到日志等级: " << log_class << ", 消息: " << message << ", 时间戳: " << timestamp << std::endl;
            
                // 发送解析后的内容
                std::string new_content = "<" + log_class + ">" + "[" + level + "] " + "{" + message + "}" + " at " + timestamp;
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
}