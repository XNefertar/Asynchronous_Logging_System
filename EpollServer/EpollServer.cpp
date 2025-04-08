#include "EpollServer.hpp"
#include <openssl/sha.h>  // 需要 OpenSSL 库
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

using namespace EpollServerSpace;

void EpollServerSpace::signalHandler(int signum)
{
    std::cout << "\033[1;31m[错误]\033[0m 捕获到信号 " << signum << "，服务器正在关闭..." << std::endl;
    exit(0);
}

EpollServer::EpollServer( uint64_t port
                        , std::string defaultUserName
                        , std::string defaultPassword
                        , std::string defaultDBName
                        , std::string path
                        , std::string defaultIPAddress
                        , int defaultPort
                        , int defaultMaxConn
                        , std::string staticFilesDir)
    : _port(port)
    , _defaultUserName(defaultUserName)
    , _defaultPassword(defaultPassword)
    , _defaultDBName(defaultDBName)
    , _listenfd(defaultValue)
    , _epollfd(defaultValue)
    , _log_file(path)
    , _events(nullptr)
    , _defaultIPAddress(defaultIPAddress)
    , _defaultPort(defaultPort)
    , _defaultMaxConn(defaultMaxConn)
    , _staticFilesDir(staticFilesDir)
    , _wsPath("/ws")  // 默认WebSocket路径
{
    if (port == 0)
        _port = defaultPort;

    _events = new struct epoll_event[defaultEpollSize];
    if(!SqlConnPool::getInstance()->init(_defaultIPAddress.c_str(), _defaultUserName.c_str(), _defaultPassword.c_str(), _defaultDBName.c_str(), _defaultPort, _defaultMaxConn)){
        std::cerr << "\033[1;31m[错误]\033[0m 数据库连接池初始化失败" << std::endl;
        exit(1);
    }

    // 确保静态文件目录存在
    if (!_staticFilesDir.empty() && !std::filesystem::exists(_staticFilesDir)) {
        try {
            std::filesystem::create_directories(_staticFilesDir);
        } catch (const std::exception& e) {
            std::cerr << "\033[1;31m[错误]\033[0m 创建静态文件目录失败: " << e.what() << std::endl;
        }
    }
}

int EpollServer::LeveltoInt(const std::string& level) {
    if (level == "NORMAL") return NORMAL;
    if (level == "DEBUG") return DEBUG;
    if (level == "INFO") return INFO;
    if (level == "WARNING") return WARNING;
    if (level == "ERROR") return ERROR;
    if (level == "FATAL") return FATAL;
    return -1; // 未知等级
}

EpollServer::~EpollServer()
{
    if (_events != nullptr)
        delete[] _events;

    if (_listenfd != defaultValue)
        close(_listenfd);

    if (_epollfd != defaultValue)
        close(_epollfd);
}

void EpollServer::ServerInit(){
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
    
    std::cout << "\033[1;32m[启动]\033[0m Epoll服务器已初始化, 监听端口: " << _port << std::endl;
}

void EpollServer::ServerStart(){
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

void EpollServer::HandleEvents(int ReadyNum) {
    for(int i = 0; i < ReadyNum; ++i) {
        int sockfd = _events[i].data.fd;
        if(sockfd == _listenfd && (_events[i].events & EPOLLIN)) {
            // 新客户端连接
            std::string ip;
            uint16_t port;
            int connfd = Sock::Accept(_listenfd, ip, port);
            if(connfd < 0) {
                _log_file << "[ERROR] Accept connection failed: " << strerror(errno) << std::endl;
                continue;
            }
            
            // 创建新的客户端会话记录
            ClientSessionInfo sessionInfo = {ip, port, _defaultDBName, _defaultUserName, time(nullptr), 0, 0};
            SessionManager::getInstance()->addSession(connfd, sessionInfo);
            
            // 输出连接信息
            _log_file << "[INFO] Client " << ip << ":" << port 
                      << " connected, socket: " << connfd << std::endl;

            struct epoll_event ev;
            ev.data.fd = connfd;
            ev.events = EPOLLIN;
            if(epoll_ctl(_epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
                _log_file << "[ERROR] Failed to add client to epoll: " << strerror(errno) << std::endl;
                close(connfd);
                SessionManager::getInstance()->removeSession(connfd);
                continue;
            }
        }
        else if(_events[i].events & EPOLLIN) {
            // 处理已连接客户端的数据
            char buffer[4096] = {0};
            ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            
            if (n <= 0) {
                // 客户端断开连接或错误
                if (n < 0) {
                    _log_file << "[ERROR] Recv error: " << strerror(errno) << std::endl;
                }
                
                // 关闭连接并清理
                close(sockfd);
                epoll_ctl(_epollfd, EPOLL_CTL_DEL, sockfd, NULL);
                _wsConnections.erase(sockfd);
                SessionManager::getInstance()->removeSession(sockfd);
                _log_file << "[INFO] Client disconnected, socket: " << sockfd << std::endl;
                continue;
            }
            
            // 获取会话信息
            ClientSessionInfo sessionInfo = SessionManager::getInstance()->getSession(sockfd);
            ClientSession session(sessionInfo); // 转换为 ClientSession 类型
            session.type = ClientType::RAW_TCP; // 设置默认类型
            sessionInfo.total_bytes += n;
            sessionInfo.message_count++;
            SessionManager::getInstance()->updateSession(sockfd, sessionInfo);
            
            // 检查是否是HTTP请求
            if (strncmp(buffer, "GET", 3) == 0 || strncmp(buffer, "POST", 4) == 0) {
                // 解析HTTP请求
                HttpRequest request;
                if (parseHttpRequest(std::string(buffer, n), request)) {
                    // 检查是否是WebSocket升级请求
                    auto connectionIt = request.headers.find("Connection");
                    auto upgradeIt = request.headers.find("Upgrade");
                    
                    if (connectionIt != request.headers.end() && 
                        upgradeIt != request.headers.end() && 
                        connectionIt->second.find("Upgrade") != std::string::npos &&
                        upgradeIt->second == "websocket" &&
                        request.path == _wsPath) {
                        // WebSocket握手
                        if (handleWebSocketHandshake(sockfd, request, session)) {
                            _log_file << "[INFO] WebSocket handshake successful for client: " 
                                     << session.ip << ":" << session.port << std::endl;
                        } else {
                            _log_file << "[ERROR] WebSocket handshake failed for client: " 
                                     << session.ip << ":" << session.port << std::endl;
                        }
                    } else {
                        // 处理普通HTTP请求
                        handleHttpRequest(sockfd, request, session);
                    }
                } else {
                    // 无效的HTTP请求
                    HttpResponse response;
                    response.statusCode = 400;
                    response.statusText = "Bad Request";
                    response.body = "Invalid HTTP request";
                    response.headers["Content-Type"] = "text/plain";
                    response.headers["Content-Length"] = std::to_string(response.body.size());
                    
                    std::string responseStr = serializeHttpResponse(response);
                    send(sockfd, responseStr.c_str(), responseStr.size(), 0);
                }
            } else {
                // 检查是否是WebSocket帧
                if (_wsConnections.find(sockfd) != _wsConnections.end()) {
                    // 转换为vector以便处理
                    std::vector<char> frameData(buffer, buffer + n);
                    handleWebSocketFrame(sockfd, frameData, session);
                } else {
                    // 普通TCP数据，这里可以处理您原有的协议
                    // ...
                }
            }
        }
    }
}

// 解析 HTTP 请求
bool EpollServer::parseHttpRequest(const std::string& requestStr, HttpRequest& request) {
    std::istringstream requestStream(requestStr);
    std::string line;
    
    // 解析请求行
    if (std::getline(requestStream, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        std::istringstream requestLineStream(line);
        std::string method, path, version;
        requestLineStream >> method >> path >> version;
        
        // 设置请求方法
        if (method == "GET") request.method = HttpMethod::GET;
        else if (method == "POST") request.method = HttpMethod::POST;
        else if (method == "PUT") request.method = HttpMethod::PUT;
        else if (method == "DELETE") request.method = HttpMethod::DELETE;
        else if (method == "OPTIONS") request.method = HttpMethod::OPTIONS;
        else request.method = HttpMethod::UNKNOWN;
        
        // 解析路径和查询参数
        size_t queryPos = path.find('?');
        if (queryPos != std::string::npos) {
            std::string queryString = path.substr(queryPos + 1);
            path = path.substr(0, queryPos);
            
            std::istringstream queryStream(queryString);
            std::string param;
            while (std::getline(queryStream, param, '&')) {
                size_t equalPos = param.find('=');
                if (equalPos != std::string::npos) {
                    std::string key = param.substr(0, equalPos);
                    std::string value = param.substr(equalPos + 1);
                    request.queryParams[key] = value;
                }
            }
        }
        
        request.path = path;
        request.version = version;
    } else {
        return false;
    }
    
    // 解析请求头
    while (std::getline(requestStream, line) && !line.empty() && line != "\r") {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // 去除前导空格
            value.erase(0, value.find_first_not_of(" \t"));
            request.headers[key] = value;
        }
    }
    
    // 解析请求体
    std::ostringstream bodyStream;
    while (std::getline(requestStream, line)) {
        bodyStream << line << "\n";
    }
    request.body = bodyStream.str();
    
    return true;
}

// 序列化 HTTP 响应
std::string EpollServer::serializeHttpResponse(const HttpResponse& response) {
    std::ostringstream responseStream;
    
    // 状态行
    responseStream << "HTTP/1.1 " << response.statusCode << " " << response.statusText << "\r\n";
    
    // 响应头
    for (const auto& header : response.headers) {
        responseStream << header.first << ": " << header.second << "\r\n";
    }
    
    // 空行分隔
    responseStream << "\r\n";
    
    // 响应体
    responseStream << response.body;
    
    return responseStream.str();
}

// 获取文件的 MIME 类型
std::string EpollServer::getMimeType(const std::string& path) {
    std::string extension = path.substr(path.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "html" || extension == "htm") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "json") return "application/json";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "svg") return "image/svg+xml";
    if (extension == "ico") return "image/x-icon";
    
    return "application/octet-stream";  // 默认二进制类型
}

// 提供静态文件服务
HttpResponse EpollServer::serveStaticFile(const std::string& path) {
    HttpResponse response;
    
    // 拼接完整文件路径
    std::string fullPath = _staticFilesDir + path;
    
    // 如果路径是目录，则尝试提供 index.html
    if (fullPath.back() == '/') {
        fullPath += "index.html";
    }
    
    // 打开文件
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = "404 - File not found";
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    
    // 读取文件内容
    std::ostringstream contentStream;
    contentStream << file.rdbuf();
    response.body = contentStream.str();
    
    // 设置响应头
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = getMimeType(fullPath);
    response.headers["Content-Length"] = std::to_string(response.body.size());
    
    return response;
}

// Base64 编码，用于WebSocket握手
std::string base64Encode(const unsigned char* input, int length) {
    BIO* bio, *b64;
    BUF_MEM* bufferPtr;
    
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    
    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
}

// 生成 WebSocket 握手响应的 Accept Key
std::string EpollServer::generateWebSocketAcceptKey(const std::string& key) {
    const std::string magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concatenated = key + magicString;
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(concatenated.c_str()), concatenated.size(), hash);
    
    return base64Encode(hash, SHA_DIGEST_LENGTH);
}

// 处理 WebSocket 握手
bool EpollServer::handleWebSocketHandshake(int sockfd, const HttpRequest& request, ClientSession& session) {
    auto it = request.headers.find("Sec-WebSocket-Key");
    if (it == request.headers.end()) {
        return false;
    }
    
    std::string acceptKey = generateWebSocketAcceptKey(it->second);
    
    HttpResponse response;
    response.statusCode = 101;
    response.statusText = "Switching Protocols";
    response.headers["Upgrade"] = "websocket";
    response.headers["Connection"] = "Upgrade";
    response.headers["Sec-WebSocket-Accept"] = acceptKey;
    
    std::string responseStr = serializeHttpResponse(response);
    
    if (send(sockfd, responseStr.c_str(), responseStr.size(), 0) < 0) {
        return false;
    }
    
    // 更新会话信息
    session.type = ClientType::WEBSOCKET;
    session.wsConnection.sockfd = sockfd;
    session.wsConnection.state = WebSocketState::OPEN;
    session.wsConnection.handshakeKey = it->second;
    
    // 添加到 WebSocket 连接列表
    _wsConnections.insert(sockfd);
    
    return true;
}

// 创建 WebSocket 数据帧
std::vector<char> EpollServer::createWebSocketFrame(const std::string& message, WebSocketOpcode opcode) {
    std::vector<char> frame;
    
    // 帧头第一个字节 (FIN + opcode)
    frame.push_back(0x80 | static_cast<char>(opcode));  // FIN 位设为 1，表示这是最后一个帧
    
    // 帧头第二个字节起 (mask + payload length)
    size_t payloadLength = message.size();
    
    if (payloadLength < 126) {
        // 如果负载长度小于 126，直接在第二个字节表示长度
        frame.push_back(static_cast<char>(payloadLength));
    } else if (payloadLength <= 0xFFFF) {
        // 如果负载长度在 126 到 65535 之间，使用 2 字节表示长度
        frame.push_back(126);
        frame.push_back((payloadLength >> 8) & 0xFF);
        frame.push_back(payloadLength & 0xFF);
    } else {
        // 如果负载长度大于 65535，使用 8 字节表示长度
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((payloadLength >> (i * 8)) & 0xFF);
        }
    }
    
    // 添加消息负载
    for (char c : message) {
        frame.push_back(c);
    }
    
    return frame;
}

// 处理 WebSocket 数据帧
void EpollServer::handleWebSocketFrame(int sockfd, const std::vector<char>& frame, ClientSession& session) {
    if (frame.size() < 2) return;
    
    // 解析帧头
    bool fin = (frame[0] & 0x80) != 0;
    uint8_t opcode = frame[0] & 0x0F;
    bool masked = (frame[1] & 0x80) != 0;
    uint64_t payloadLength = frame[1] & 0x7F;
    
    size_t headerLength = 2;
    
    // 确定负载长度
    if (payloadLength == 126) {
        if (frame.size() < 4) return;
        payloadLength = ((static_cast<uint16_t>(frame[2]) << 8) | static_cast<uint16_t>(frame[3]));
        headerLength += 2;
    } else if (payloadLength == 127) {
        if (frame.size() < 10) return;
        payloadLength = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLength = (payloadLength << 8) | static_cast<uint8_t>(frame[2 + i]);
        }
        headerLength += 8;
    }
    
    // 解析掩码
    uint8_t mask[4] = {0};
    if (masked) {
        if (frame.size() < headerLength + 4) return;
        for (int i = 0; i < 4; ++i) {
            mask[i] = frame[headerLength + i];
        }
        headerLength += 4;
    }
    
    // 确保帧完整
    if (frame.size() < headerLength + payloadLength) return;
    
    // 提取并解码负载
    std::string payload;
    for (size_t i = 0; i < payloadLength; ++i) {
        char c = frame[headerLength + i];
        if (masked) {
            c ^= mask[i % 4];
        }
        payload.push_back(c);
    }
    
    // 根据 opcode 处理不同类型的帧
    switch (static_cast<WebSocketOpcode>(opcode)) {
        case WebSocketOpcode::TEXT:
            // 处理文本消息，调用 WebSocket 处理器
            if (_wsHandler) {
                _wsHandler(sockfd, payload, session);
            }
            break;
            
        case WebSocketOpcode::BINARY:
            // 暂不处理二进制消息
            break;
            
        case WebSocketOpcode::PING:
            // 响应 Ping 消息
            {
                auto pongFrame = createWebSocketFrame(payload, WebSocketOpcode::PONG);
                send(sockfd, pongFrame.data(), pongFrame.size(), 0);
            }
            break;
            
        case WebSocketOpcode::CLOSE:
            // 处理关闭连接请求
            {
                auto closeFrame = createWebSocketFrame("", WebSocketOpcode::CLOSE);
                send(sockfd, closeFrame.data(), closeFrame.size(), 0);
                
                // 更新会话状态
                session.wsConnection.state = WebSocketState::CLOSED;
                _wsConnections.erase(sockfd);
                
                // 关闭连接
                close(sockfd);
            }
            break;
            
        default:
            // 忽略其他类型的帧
            break;
    }
}

// 广播 WebSocket 消息到所有连接
void EpollServer::broadcastWebSocketMessage(const std::string& message) {
    auto frame = createWebSocketFrame(message, WebSocketOpcode::TEXT);
    for (int sockfd : _wsConnections) {
        send(sockfd, frame.data(), frame.size(), 0);
    }
}

// 处理 HTTP 请求
void EpollServer::handleHttpRequest(int sockfd, const HttpRequest& request, ClientSession& session) {
    HttpResponse response;
    bool handled = false;
    
    // 根据请求方法选择不同的处理器映射
    switch(request.method) {
        case HttpMethod::GET:
        {
            // 首先检查是否是 API 请求
            auto handler = _getHandlers.find(request.path);
            if (handler != _getHandlers.end()) {
                response = handler->second(request, session);
                handled = true;
            }
            break;
        }
        case HttpMethod::POST:
        {
            auto handler = _postHandlers.find(request.path);
            if (handler != _postHandlers.end()) {
                response = handler->second(request, session);
                handled = true;
            }
            break;
        }
        default:
            // 不支持的方法
            response.statusCode = 405;
            response.statusText = "Method Not Allowed";
            response.body = "Method not supported";
            response.headers["Content-Type"] = "text/plain";
            response.headers["Content-Length"] = std::to_string(response.body.size());
            handled = true;
            break;
    }
    
    // 如果没有处理，则尝试提供静态文件
    if (!handled) {
        response = serveStaticFile(request.path);
    }
    
    // 发送响应
    std::string responseStr = serializeHttpResponse(response);
    send(sockfd, responseStr.c_str(), responseStr.size(), 0);
    
    // 记录请求
    _log_file << "[INFO] " << session.ip << ":" << session.port 
              << " " << (request.method == HttpMethod::GET ? "GET" : "POST") 
              << " " << request.path
              << " " << response.statusCode << std::endl;
}

// 添加 GET 请求处理器
void EpollServer::addGetHandler(const std::string& path, RequestHandler handler) {
    _getHandlers[path] = handler;
}

// 添加 POST 请求处理器
void EpollServer::addPostHandler(const std::string& path, RequestHandler handler) {
    _postHandlers[path] = handler;
}

// 设置 WebSocket 处理器
void EpollServer::setWebSocketHandler(const std::string& path, WebSocketHandler handler) {
    _wsPath = path;
    _wsHandler = handler;
}

// 设置静态文件目录
void EpollServer::setStaticFilesDir(const std::string& dir) {
    _staticFilesDir = dir;
    
    // 确保目录存在
    if (!std::filesystem::exists(_staticFilesDir)) {
        std::filesystem::create_directories(_staticFilesDir);
    }
}