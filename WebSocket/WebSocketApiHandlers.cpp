#include "WebSocketApiHandlers.hpp"
#include "WebSocket.hpp"
#include "../MySQL/SqlConnPool.hpp"
#include "../Util/SessionManager.hpp"
#include <chrono>
#include <mysql/mysql.h>


#include <iostream>
#include <filesystem>
std::string __log_file_path = std::filesystem::current_path().string() + "/Log/WebSocketServer.txt";
std::ofstream __log_file(__log_file_path, std::ios::app);

// ������־API����
HttpResponse handleLogsApi(const HttpRequest& request, ClientSession& session) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    
    // ��ѯ��������
    int limit = 100;  // Ĭ������
    int offset = 0;   // Ĭ��ƫ��
    std::string levelFilter;
    
    for (const auto& param : request.queryParams) {
        if (param.first == "limit") {
            limit = std::stoi(param.second);
        } else if (param.first == "offset") {
            offset = std::stoi(param.second);
        } else if (param.first == "level") {
            levelFilter = param.second;
        }
    }
    
    // �����ݿ��л�ȡ��־����
    std::vector<std::map<std::string, std::string>> logs = fetchLogsFromDatabase(limit, offset, levelFilter);
    
    // TODO
    // ʹ��רҵJSON��ʵ��
    // ����־����ת��ΪJSON��ʽ
    std::string json = "[";
    for (size_t i = 0; i < logs.size(); ++i) {
        json += "{\n";
        json += "\"timestamp\": \"" + logs[i]["timestamp"] + "\",\n";
        json += "\"level\": \"" + logs[i]["level"] + "\",\n";
        json += "\"message\": \"" + logs[i]["message"] + "\"\n";
        json += "}";
        
        if (i < logs.size() - 1) {
            json += ",";
        }
    }
    json += "]";
    
    response.body = json;
    response.headers["Content-Type"] = "application/json";
    response.headers["Content-Length"] = std::to_string(json.size());
    
    return response;
}

// ����ͳ��API����
HttpResponse handleStatsApi(const HttpRequest& request, ClientSession& session) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    
    // ��ȡͳ����Ϣ
    int totalLogs = getTotalLogsCount();
    int warningCount = getLogCountByLevel("WARNING");
    int errorCount = getLogCountByLevel("ERROR") + getLogCountByLevel("FATAL");
    int clientCount = SessionManager::getInstance()->getSessionCount();
    
    // ����JSON��Ӧ
    std::string json = "{\n";
    json += "\"totalLogs\": " + std::to_string(totalLogs) + ",\n";
    json += "\"warningCount\": " + std::to_string(warningCount) + ",\n";
    json += "\"errorCount\": " + std::to_string(errorCount) + ",\n";
    json += "\"clientCount\": " + std::to_string(clientCount) + "\n";
    json += "}";
    
    response.body = json;
    response.headers["Content-Type"] = "application/json";
    response.headers["Content-Length"] = std::to_string(json.size());
    
    return response;
}

// WebSocket��Ϣ������
void handleWebSocketMessage(int sockfd, const std::string& message, ClientSession& session) {
    // �������ݿ���뺯��
    insertWebSocketMessage(sockfd, message, session);

    // ����ӿͻ��˽��յ� WebSocket ��Ϣ
    std::string response = "{\"status\": \"ok\", \"message\": \"Received: " + message + "\"}";
    std::cout << "handleWebSocketMessage: " << message << std::endl;
    __log_file << "[INFO] ����WebSocket��Ϣ: " << message << std::endl;
    auto frame = createWebSocketFrameWrapper(response);
    send(sockfd, frame.data(), frame.size(), 0);
}

// ������־���µ����� WebSocket �ͻ���
void broadcastLogUpdate(const std::string& level, const std::string& message) {
    // ������־ JSON
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char time_buffer[100];
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
    
    std::string logJson = "{\n";
    logJson += "\"timestamp\": \"" + std::string(time_buffer) + "\",\n";
    logJson += "\"level\": \"" + level + "\",\n";
    logJson += "\"message\": \"" + message + "\"\n";
    logJson += "}";
    
    // �㲥������ WebSocket �ͻ���
    broadcastWebSocketMessageWrapper(logJson);
}

// ����WebSocket��Ϣ���������ݿ�
void insertWebSocketMessage(int sockfd, const std::string& message, ClientSession& session) {
    // ���Խ���JSON��ʽ����־��Ϣ
    try {
        // �򵥵�JSON����(ʵ����Ŀ�п�����Ҫʹ��JSON��)
        std::string level, logMessage, timestamp;
        
        // ��ȡlevel�ֶ�
        size_t levelPos = message.find("\"level\":");
        if (levelPos != std::string::npos) {
            size_t valueStart = message.find("\"", levelPos + 8) + 1;
            size_t valueEnd = message.find("\"", valueStart);
            level = message.substr(valueStart, valueEnd - valueStart);
        }
        __log_file << "[INFO] ��������־����: " << level << std::endl;
        
        // ��ȡmessage�ֶ�
        size_t messagePos = message.find("\"message\":");
        if (messagePos != std::string::npos) {
            size_t valueStart = message.find("\"", messagePos + 10) + 1;
            size_t valueEnd = message.find("\"", valueStart);
            logMessage = message.substr(valueStart, valueEnd - valueStart);
        }
        __log_file << "[INFO] ��������־��Ϣ: " << logMessage << std::endl;
        
        // ��ȡtimestamp�ֶ�
        size_t timestampPos = message.find("\"timestamp\":");
        if (timestampPos != std::string::npos) {
            size_t valueStart = message.find("\"", timestampPos + 12) + 1;
            size_t valueEnd = message.find("\"", valueStart);
            timestamp = message.substr(valueStart, valueEnd - valueStart);
        }
        __log_file << "[INFO] ������ʱ���: " << timestamp << std::endl;
        
        // ��ȡ���ݿ����Ӳ�������־
        MYSQL* conn = nullptr;
        SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
        
        if (conn) {
            // ׼��SQL���
            std::string sql = "INSERT INTO log_table (level, message, timestamp) VALUES (?, ?, ?)";
            
            // ʹ��Ԥ��������ֹSQLע��
            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            if (stmt) {
                if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) == 0) {
                    MYSQL_BIND bind[3];
                    memset(bind, 0, sizeof(bind));
                    
                    // ��level����
                    bind[0].buffer_type = MYSQL_TYPE_STRING;
                    bind[0].buffer = (void*)level.c_str();
                    bind[0].buffer_length = level.length();
                    
                    // ��message����
                    bind[1].buffer_type = MYSQL_TYPE_STRING;
                    bind[1].buffer = (void*)logMessage.c_str();
                    bind[1].buffer_length = logMessage.length();
                    
                    // ��timestamp����
                    bind[2].buffer_type = MYSQL_TYPE_STRING;
                    bind[2].buffer = (void*)timestamp.c_str();
                    bind[2].buffer_length = timestamp.length();
                    
                    if (mysql_stmt_bind_param(stmt, bind) == 0) {
                        if (mysql_stmt_execute(stmt) == 0) {
                            // �ɹ��������ݿ�
                            std::string response = "{\"status\": \"ok\", \"message\": \"Log saved to database\"}";
                            auto frame = createWebSocketFrameWrapper(response);
                            send(sockfd, frame.data(), frame.size(), 0);
                        } else {
                            // ִ��ʧ��
                            std::string error = mysql_stmt_error(stmt);
                            std::string response = "{\"status\": \"error\", \"message\": \"Database error: " + error + "\"}";
                            auto frame = createWebSocketFrameWrapper(response);
                            send(sockfd, frame.data(), frame.size(), 0);
                        }
                    }   

                    __log_file << "[INFO] �󶨲����ɹ�" << std::endl;
                    
                    mysql_stmt_close(stmt);
                }
            }
        } else {
            // �޷���ȡ���ݿ�����
            std::string response = "{\"status\": \"error\", \"message\": \"Database connection failed\"}";
            auto frame = createWebSocketFrameWrapper(response);
            send(sockfd, frame.data(), frame.size(), 0);
        }
    } catch (const std::exception& e) {
        // �����쳣
        std::string response = "{\"status\": \"error\", \"message\": \"Error processing log: " + std::string(e.what()) + "\"}";
        auto frame = createWebSocketFrameWrapper(response);
        send(sockfd, frame.data(), frame.size(), 0);
    }
}

HttpResponse handleLogFileDownload(const HttpRequest& request, ClientSession& session) {
    HttpResponse response;
    
    // ��ȡ��ѯ������ָ�����ļ�����
    std::string fileType = "html"; // Ĭ��html
    if (request.queryParams.find("type") != request.queryParams.end()) {
        fileType = request.queryParams.at("type");
    }
    
    std::string filePath = std::filesystem::current_path().string() + "/log." + fileType;
    
    // ����ļ��Ƿ����
    if (!std::filesystem::exists(filePath)) {
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = "Log file not found";
        return response;
    }
    
    // ��ȡ�ļ�����
    std::ifstream file(filePath, std::ios::binary);
    std::ostringstream contentStream;
    contentStream << file.rdbuf();
    response.body = contentStream.str();
    
    // ������Ӧͷ
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = (fileType == "html") ? "text/html" : "text/plain";
    response.headers["Content-Disposition"] = "attachment; filename=\"log." + fileType + "\"";
    response.headers["Content-Length"] = std::to_string(response.body.size());
    
    return response;
}