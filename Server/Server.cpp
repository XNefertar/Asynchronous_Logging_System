#include "Server.hpp"
#include "AsyncDBWriter.hpp"
using namespace AsyncDBWriterSpace;

int Server::LeveltoInt(const std::string& level) {
    if (level == "NORMAL") return NORMAL;
    if (level == "DEBUG") return DEBUG;
    if (level == "INFO") return INFO;
    if (level == "WARNING") return WARNING;
    if (level == "ERROR") return ERROR;
    if (level == "FATAL") return FATAL;
    return -1; // 未知等级
}

void Server::broadcastLogToWebSocket(const std::string& level, const std::string& message, const std::string& timestamp) {
    // 构造JSON格式的日志数据
    std::string logJson = "{\n";
    logJson += "\"type\": \"log_update\",\n";
    logJson += "\"timestamp\": \"" + timestamp + "\",\n";
    logJson += "\"level\": \"" + level + "\",\n";
    logJson += "\"message\": \"" + message + "\"\n";
    logJson += "}";
    
    // 调用全局WebSocket广播函数
    if (g_server) {
        g_server->broadcastWebSocketMessage(logJson);
        LogMessage::logMessage(INFO, "WebSocket广播日志: %s - %s", level.c_str(), message.c_str());
    } else {
        LogMessage::logMessage(WARNING, "WebSocket服务器未初始化，无法广播日志");
    }
}

void Server::socketIO(int socket){

    std::string defaultLogPath = std::filesystem::current_path().string() + "/Log/Server.txt";
    LogMessage::setDefaultLogPath(defaultLogPath);

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
        
        std::string message_total(buffer);
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
        std::string path = std::filesystem::current_path().string() + "/Log/Server.txt";
        LogMessage::logMessage(INFO, "接收客户端消息: %s", buffer);
        // 解析客户端信息并将其插入到数据库中

        // <log info>[INFO] {Scheduled task executed - Task: process_bitmap, Status: failed, Duration: {time}ms - Exception: bitmap & BITMAP_1} at 2025-04-09 15:45:45
        // 解析 message_preview 即可
        std::regex pattern(R"(<([^>]+)>\[([^\]]+)\]\s+\{(.*?)\}\s+at\s+([\d-]+\s[\d:]+))");
        std::smatch match;
        if (std::regex_search(message_total, match, pattern)) {
            std::string logLevel = match[2];
            std::string message = match[3];
            std::string timestamp = match[4];
            LogMessage::logMessage(INFO, "解析到日志等级: %s, 消息: %s, 时间戳: %s", logLevel.c_str(), message.c_str(), timestamp.c_str());

            MYSQL* conn = nullptr;
            SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());

            if(LeveltoInt(logLevel) < 0 || LeveltoInt(logLevel) > 5){
                // 日志等级解析失败
                std::cerr << "\033[1;31m[错误]\033[0m 日志等级解析失败" << std::endl;
            }
            if(LeveltoInt(logLevel) < LeveltoInt("WARNING")){
                // 日志等级小于WARNING, 不记录到数据库
                std::cout << "\033[1;33m[日志等级]\033[0m 日志等级小于WARNING, 不记录到数据库" << std::endl;
            }
            else{
                std::cout << "\033[1;32m[日志等级]\033[0m 日志等级: " << logLevel << std::endl;
                // 记录到数据库
                std::cout << "\033[1;32m[数据库记录]\033[0m 日志记录到数据库" << std::endl;
                // TODO: message字段需要更新

                
                // MYSQL* conn = nullptr;
                // SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());


                // if(conn){
                //     // 1. 初始化预处理语句
                //     MYSQL_STMT *stmt = mysql_stmt_init(conn);
                //     if (!stmt) {
                //         LogMessage::logMessage(ERROR, "mysql_stmt_init() 失败: %s", mysql_error(conn));
                //         continue;
                //     }
                    
                //     // 2. 准备带有占位符的SQL语句
                //     const char *query = "INSERT INTO log_table (level, ip, port, message) VALUES (?, ?, ?, ?)";
                //     if (mysql_stmt_prepare(stmt, query, strlen(query))) {
                //         LogMessage::logMessage(ERROR, "mysql_stmt_prepare() 失败: %s", mysql_stmt_error(stmt));
                //         mysql_stmt_close(stmt);
                //         continue;
                //     }
                    
                //     // 3. 绑定参数
                //     MYSQL_BIND bind[4];
                //     memset(bind, 0, sizeof(bind));
                    
                //     // level 参数
                //     bind[0].buffer_type = MYSQL_TYPE_STRING;
                //     bind[0].buffer = (void*)logLevel.c_str();
                //     bind[0].buffer_length = logLevel.length();
                    
                //     // ip 参数
                //     bind[1].buffer_type = MYSQL_TYPE_STRING;
                //     bind[1].buffer = (void*)client_ip.c_str();
                //     bind[1].buffer_length = client_ip.length();
                    
                //     // port 参数
                //     unsigned int port = client_port;
                //     bind[2].buffer_type = MYSQL_TYPE_LONG;
                //     bind[2].buffer = (void*)&port;
                    
                //     // message 参数
                //     bind[3].buffer_type = MYSQL_TYPE_STRING;
                //     bind[3].buffer = (void*)message.c_str();
                //     bind[3].buffer_length = message.length();
                    
                //     // 4. 绑定参数到预处理语句
                //     if (mysql_stmt_bind_param(stmt, bind)) {
                //         LogMessage::logMessage(ERROR, "mysql_stmt_bind_param() 失败: %s", mysql_stmt_error(stmt));
                //         mysql_stmt_close(stmt);
                //         continue;
                //     }
                    
                //     // 5. 执行预处理语句
                //     if (mysql_stmt_execute(stmt)) {
                //         LogMessage::logMessage(ERROR, "mysql_stmt_execute() 失败: %s", mysql_stmt_error(stmt));
                //         mysql_stmt_close(stmt);
                //         continue;
                //     }
                    
                //     LogMessage::logMessage(INFO, "MySQL 成功插入日志: level=%s, ip=%s, port=%u", 
                //                           logLevel.c_str(), client_ip.c_str(), port);
                    
                //     // 6. 关闭预处理语句
                //     mysql_stmt_close(stmt);
                // }
                DBWriteTask task(logLevel, client_ip, client_port, message);
                AsyncDBWriter::getInstance().addTask(task);
                std::cout << "\033[1;32m[数据库记录]\033[0m 日志已提交到异步写入队列" << std::endl;

                broadcastLogToWebSocket(logLevel, message, timestamp);
            }
        }
        else{
            LogMessage::logMessage(ERROR, "日志解析失败: %s", message_preview.c_str());
            std::cerr << "\033[1;31m[错误]\033[0m 日志解析失败: " << message_preview << std::endl;
        }

        int bytes_sent = write(socket, response.c_str(), response.size());
        std::cout << "\033[1;36m[响应发送]\033[0m 响应大小: " << bytes_sent << " 字节" << std::endl;
    }
    
    close(socket); // 关闭套接字
}

Server::ServerTCP::ServerTCP(uint64_t    port
                           , std::string defaultUserName
                           , std::string defaultPassword
                           , std::string defaultDBName
                           , uint64_t    defaultPort
                           , std::string defaultIPAddress
                           , int         defaultMaxConn)
            : _port(port)
            , _defaultUserName(defaultUserName)
            , _defaultPassword(defaultPassword)
            , _defaultDBName(defaultDBName)
            , _defaultIPAddress(defaultIPAddress)
            , _defaultPort(defaultPort)
            , _defaultMaxConn(defaultMaxConn)
        {
            if(0 == _port) 
                _port = defaultPort;

            // 初始化数据库连接池
            if(!SqlConnPool::getInstance()->init(_defaultIPAddress.c_str(), _defaultUserName.c_str(), _defaultPassword.c_str(), _defaultDBName.c_str(), _defaultPort, _defaultMaxConn)){
                std::cerr << "\033[1;31m[错误]\033[0m 数据库连接池初始化失败" << std::endl;
                exit(1);
            }
            
            // 启动异步数据库写入器
            AsyncDBWriter::getInstance().start(10); // 启动2个工作线程
        }

// 修改析构函数，确保正确关闭异步写入器
Server::ServerTCP::~ServerTCP() {
    AsyncDBWriter::getInstance().shutdown();
    close(_socketfd);
}

void Server::ServerTCP::init(){
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
    std::cout << "\033[1;32m[启动]\033[0m TCP服务器已初始化, 监听端口: " << _port << std::endl;
    std::cout << "\033[1;34m[运行]\033[0m 服务器开始运行，等待连接..." << std::endl;
}

void Server::ServerTCP::run(){
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

void Server::setGlobalServerReference(EpollServerSpace::EpollServer* server) {
    g_server = server;
}