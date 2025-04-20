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
#include <set>
#include <filesystem>
#include <locale>
#include <algorithm>
#include <regex>
#include <functional>
#include <unordered_map>
#include <sstream>
#include "../Util/Sock.hpp"
#include "../MySQL/SqlConnPool.hpp"
#include "../Util/SessionManager.hpp"

namespace EpollServerSpace {

    void signalHandler(int signum);
    
    static const uint64_t defaultPort       = 8080;
    static const int      defaultEpollSize  = 1024;
    static const int      defaultValue      = -1;
    static const int      timeout           = -1;   // epoll_wait timeout，-1表示阻塞等待
    
    // 日志级别枚举
    // enum LogLevel {
    //     NORMAL = 0,
    //     DEBUG,
    //     INFO,
    //     WARNING,
    //     ERROR,
    //     FATAL
    // };
    
    // HTTP 请求方法枚举
    // class 强枚举类型
    // 防止枚举类型冲突
    enum class HttpMethod {
        GET,
        POST,
        PUT,
        DELETE,
        OPTIONS,
        UNKNOWN
    };
    
    // HTTP 请求结构
    struct HttpRequest {
        HttpMethod method;
        std::string path;
        std::string version;
        std::map<std::string, std::string> headers;
        std::string body;
        std::map<std::string, std::string> queryParams;
    };
    
    // HTTP 响应结构
    struct HttpResponse {
        int statusCode;
        std::string statusText;
        std::map<std::string, std::string> headers;
        std::string body;
    };
    
    // WebSocket 连接状态枚举
    enum class WebSocketState {
        CONNECTING,
        OPEN,
        CLOSING,
        CLOSED
    };
    
    // WebSocket 帧类型枚举
    enum class WebSocketOpcode {
        CONTINUATION = 0x0,
        TEXT = 0x1,
        BINARY = 0x2,
        CLOSE = 0x8,
        PING = 0x9,
        PONG = 0xA
    };
    
    // WebSocket 连接结构
    struct WebSocketConnection {
        int sockfd;
        WebSocketState state;
        std::string handshakeKey;
    };
    
    // 客户端连接类型枚举
    enum class ClientType {
        RAW_TCP,
        HTTP,
        WEBSOCKET
    };
    
    // 客户端会话结构 - 扩展自 ClientSessionInfo
    struct ClientSession : public ClientSessionInfo {
        ClientType type;
        HttpRequest currentRequest;
        WebSocketConnection wsConnection;
        std::vector<char> buffer;
        
        ClientSession()
            : ClientSessionInfo(), type(ClientType::RAW_TCP) {}
        
        ClientSession(const ClientSessionInfo& info) 
            : ClientSessionInfo(info), type(ClientType::RAW_TCP) {}
    };
    
    using RequestHandler = std::function<HttpResponse(const HttpRequest&, ClientSession&)>;
    using WebSocketHandler = std::function<void(int sockfd, const std::string& message, ClientSession&)>;
    
    class EpollServer {
    private:
        uint64_t                        _port;
        std::string                     _defaultUserName;
        std::string                     _defaultPassword;
        std::string                     _defaultDBName;
        int                             _listenfd;
        int                             _epollfd;
        std::ofstream                   _log_file;
        struct epoll_event*             _events;
        std::string                     _defaultIPAddress;
        int                             _defaultPort;
        int                             _defaultMaxConn;
        std::string                     _staticFilesDir;  // 静态文件目录
        
        // 路由系统
        std::map<std::string, RequestHandler> _getHandlers;   // GET 请求处理器
        std::map<std::string, RequestHandler> _postHandlers;  // POST 请求处理器
        std::string                     _wsPath;              // WebSocket 路径
        WebSocketHandler                _wsHandler;           // WebSocket 消息处理器
        std::set<int>                   _wsConnections;       // WebSocket 连接列表

    private:
        // HTTP 相关方法
        bool parseHttpRequest(const std::string& requestStr, HttpRequest& request);
        std::string serializeHttpResponse(const HttpResponse& response);
        void handleHttpRequest(int sockfd, const HttpRequest& request, ClientSession& session);
        
        // WebSocket 相关方法
        bool handleWebSocketHandshake(int sockfd, const HttpRequest& request, ClientSession& session);
        std::string generateWebSocketAcceptKey(const std::string& key);
        void handleWebSocketFrame(int sockfd, const std::vector<char>& frame, ClientSession& session);
        
        // 静态文件服务方法
        HttpResponse serveStaticFile(const std::string& path);
        std::string getMimeType(const std::string& path);

    public:
        EpollServer(uint64_t port,
                   std::string defaultUserName,
                   std::string defaultPassword,
                   std::string defaultDBName,
                   std::string path = std::filesystem::current_path().string() + "/Log/Epoll_Server.txt",
                   std::string defaultIPAddress = "localhost",
                   int defaultPort = 3306,
                   int defaultMaxConn = 10,
                   std::string staticFilesDir = "./static");
        
        ~EpollServer();
        
        void ServerInit();
        void ServerStart();
        void HandleEvents(int ReadyNum);
        int LeveltoInt(const std::string& level);
        
        // 路由设置方法
        void addGetHandler(const std::string& path, RequestHandler handler);
        void addPostHandler(const std::string& path, RequestHandler handler);
        void setWebSocketHandler(const std::string& path, WebSocketHandler handler);
        void setStaticFilesDir(const std::string& dir);
        
        // WebSocket相关公共方法
        void broadcastWebSocketMessage(const std::string& message);
        std::vector<char> createWebSocketFrame(const std::string& message, WebSocketOpcode opcode = WebSocketOpcode::TEXT);
    };
}

#endif // __EPOLLSERVER_HPP__