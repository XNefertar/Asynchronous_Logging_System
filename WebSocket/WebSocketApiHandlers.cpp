#include "WebSocketApiHandlers.hpp"
#include "WebSocket.hpp"
#include "../MySQL/SqlConnPool.hpp"
#include "../Util/SessionManager.hpp"
#include <chrono>
#include <mysql/mysql.h>
#include <iostream>
#include <filesystem>

#include <iostream>
#include <filesystem>
std::string __log_file_path = std::filesystem::current_path().string() + "/Log/WebSocketServer.txt";
std::ofstream __log_file(__log_file_path, std::ios::app);

// 处理日志API请求
HttpResponse handleLogsApi(const HttpRequest& request, ClientSession& session) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    
    // 查询参数解析
    int limit = 100;  // 默认限制
    int offset = 0;   // 默认偏移
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
    
    // 从数据库中获取日志数据
    std::vector<std::map<std::string, std::string>> logs = fetchLogsFromDatabase(limit, offset, levelFilter);
    
    // TODO
    // 使用专业JSON库实现
    // 将日志数据转换为JSON格式
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

// 处理统计API请求
HttpResponse handleStatsApi(const HttpRequest& request, ClientSession& session) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    
    // 获取统计信息
    int totalLogs = getTotalLogsCount();
    int warningCount = getLogCountByLevel("WARNING");
    int errorCount = getLogCountByLevel("ERROR") + getLogCountByLevel("FATAL");
    int clientCount = SessionManager::getInstance()->getSessionCount();
    
    // 构建JSON响应
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

void sendWebSocketError(int sockfd, const std::string& error) {
    Json::Value errorResponse;
    errorResponse["type"] = "error";
    errorResponse["message"] = error;
    
    Json::StreamWriterBuilder builder;
    std::string responseStr = Json::writeString(builder, errorResponse);
    g_server->sendWebSocketMessage(sockfd, responseStr);
}

// WebSocket消息处理器
void handleWebSocketMessage(int sockfd, const std::string& message, ClientSession& session) {
    try {
        // 解析JSON消息
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(message, root)) {
            sendWebSocketError(sockfd, "无效的JSON消息");
            return;
        }
        
        std::string messageType = root.get("type", "").asString();
        
        if (messageType == "request") {
            handleWebSocketRequest(sockfd, root, session);
        } else {
            // 处理其他类型的消息
            insertWebSocketMessage(sockfd, message, session);
        }
        
    } catch (const std::exception& e) {
        sendWebSocketError(sockfd, std::string("处理消息失败: ") + e.what());
    }
}

void handleWebSocketRequest(int sockfd, const Json::Value& request, ClientSession& session) {
    std::string requestType = request.get("requestType", "").asString();
    int requestId = request.get("requestId", 0).asInt();
    Json::Value data = request.get("data", Json::Value());
    
    Json::Value response;
    response["type"] = "response";
    response["requestId"] = requestId;
    response["success"] = true;
    
    try {
        if (requestType == "get_logs") {
            int limit = data.get("limit", 100).asInt();
            int offset = data.get("offset", 0).asInt();
            
            auto logs = fetchLogsFromDatabase(limit, offset);
            Json::Value logsArray(Json::arrayValue);
            
            for (const auto& log : logs) {
                Json::Value logObj;
                logObj["level"] = log.at("level");
                logObj["message"] = log.at("message");
                logObj["timestamp"] = log.at("timestamp");
                logsArray.append(logObj);
            }
            
            response["data"] = logsArray;
            
        } else if (requestType == "get_stats") {
            Json::Value stats;
            stats["totalLogs"] = getTotalLogsCount();
            stats["errorCount"] = getLogCountByLevel("ERROR");
            stats["warningCount"] = getLogCountByLevel("WARNING");
            stats["infoCount"] = getLogCountByLevel("INFO");
            stats["clientCount"] = SessionManager::getInstance()->getSessionCount();
            
            response["data"] = stats;
            
        } else if (requestType == "get_logs_by_level") {
            std::string level = data.get("level", "").asString();
            auto logs = fetchLogsFromDatabase(1000, 0, level);
            
            Json::Value logsArray(Json::arrayValue);
            for (const auto& log : logs) {
                Json::Value logObj;
                logObj["level"] = log.at("level");
                logObj["message"] = log.at("message");
                logObj["timestamp"] = log.at("timestamp");
                logsArray.append(logObj);
            }
            
            response["data"] = logsArray;
            
        } else if (requestType == "download_logs") {
            std::string format = data.get("format", "txt").asString();
            
            // 生成日志文件内容
            auto logs = fetchLogsFromDatabase(1000, 0);
            std::string content;
            
            if (format == "html") {
                content = "<html><body><h1>Logs</h1><table>";
                for (const auto& log : logs) {
                    content += "<tr><td>" + log.at("timestamp") + "</td><td>" + log.at("level") + "</td><td>" + log.at("message") + "</td></tr>";
                }
                content += "</table></body></html>";
            } else {
                for (const auto& log : logs) {
                    content += log.at("timestamp") + " [" + log.at("level") + "] " + log.at("message") + "\n";
                }
            }
            
            Json::Value downloadData;
            downloadData["content"] = content;
            downloadData["filename"] = "logs_" + std::to_string(std::time(nullptr)) + "." + format;
            downloadData["mimeType"] = (format == "html") ? "text/html" : "text/plain";
            
            response["data"] = downloadData;
            
        } else {
            response["success"] = false;
            response["error"] = "未知的请求类型: " + requestType;
        }
        
    } catch (const std::exception& e) {
        response["success"] = false;
        response["error"] = std::string("处理请求失败: ") + e.what();
    }
    
    // 发送响应
    Json::StreamWriterBuilder builder;
    std::string responseStr = Json::writeString(builder, response);
    g_server->sendWebSocketMessage(sockfd, responseStr);
}

// 发送日志更新到所有 WebSocket 客户端
void broadcastLogUpdate(const std::string& level, const std::string& message) {
    // 构造日志 JSON
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char time_buffer[100];
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
    
    std::string logJson = "{\n";
    logJson += "\"timestamp\": \"" + std::string(time_buffer) + "\",\n";
    logJson += "\"level\": \"" + level + "\",\n";
    logJson += "\"message\": \"" + message + "\"\n";
    logJson += "}";
    
    // 广播到所有 WebSocket 客户端
    broadcastWebSocketMessageWrapper(logJson);
}

// 处理WebSocket消息并插入数据库
void insertWebSocketMessage(int sockfd, const std::string& message, ClientSession& session) {
    // 尝试解析JSON格式的日志消息
    try {
        // 简单的JSON解析(实际项目中可能需要使用JSON库)
        std::string level, logMessage, timestamp;
        
        // 提取level字段
        size_t levelPos = message.find("\"level\":");
        if (levelPos != std::string::npos) {
            size_t valueStart = message.find("\"", levelPos + 8) + 1;
            size_t valueEnd = message.find("\"", valueStart);
            level = message.substr(valueStart, valueEnd - valueStart);
        }
        __log_file << "[INFO] 解析到日志级别: " << level << std::endl;
        
        // 提取message字段
        size_t messagePos = message.find("\"message\":");
        if (messagePos != std::string::npos) {
            size_t valueStart = message.find("\"", messagePos + 10) + 1;
            size_t valueEnd = message.find("\"", valueStart);
            logMessage = message.substr(valueStart, valueEnd - valueStart);
        }
        __log_file << "[INFO] 解析到日志消息: " << logMessage << std::endl;
        
        // 提取timestamp字段
        size_t timestampPos = message.find("\"timestamp\":");
        if (timestampPos != std::string::npos) {
            size_t valueStart = message.find("\"", timestampPos + 12) + 1;
            size_t valueEnd = message.find("\"", valueStart);
            timestamp = message.substr(valueStart, valueEnd - valueStart);
        }
        __log_file << "[INFO] 解析到时间戳: " << timestamp << std::endl;
        
        // 获取数据库连接并插入日志
        MYSQL* conn = nullptr;
        SqlConnRAII connRAII(&conn, SqlConnPool::getInstance());
        
        if (conn) {
            // 准备SQL语句
            std::string sql = "INSERT INTO log_table (level, message, timestamp) VALUES (?, ?, ?)";
            
            // 使用预处理语句防止SQL注入
            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            if (stmt) {
                if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) == 0) {
                    MYSQL_BIND bind[3];
                    memset(bind, 0, sizeof(bind));
                    
                    // 绑定level参数
                    bind[0].buffer_type = MYSQL_TYPE_STRING;
                    bind[0].buffer = (void*)level.c_str();
                    bind[0].buffer_length = level.length();
                    
                    // 绑定message参数
                    bind[1].buffer_type = MYSQL_TYPE_STRING;
                    bind[1].buffer = (void*)logMessage.c_str();
                    bind[1].buffer_length = logMessage.length();
                    
                    // 绑定timestamp参数
                    bind[2].buffer_type = MYSQL_TYPE_STRING;
                    bind[2].buffer = (void*)timestamp.c_str();
                    bind[2].buffer_length = timestamp.length();
                    
                    if (mysql_stmt_bind_param(stmt, bind) == 0) {
                        if (mysql_stmt_execute(stmt) == 0) {
                            // 成功插入数据库
                            std::string response = "{\"status\": \"ok\", \"message\": \"Log saved to database\"}";
                            auto frame = createWebSocketFrameWrapper(response);
                            send(sockfd, frame.data(), frame.size(), 0);
                        } else {
                            // 执行失败
                            std::string error = mysql_stmt_error(stmt);
                            std::string response = "{\"status\": \"error\", \"message\": \"Database error: " + error + "\"}";
                            auto frame = createWebSocketFrameWrapper(response);
                            send(sockfd, frame.data(), frame.size(), 0);
                        }
                    }   

                    __log_file << "[INFO] 绑定参数成功" << std::endl;
                    
                    mysql_stmt_close(stmt);
                }
            }
        } else {
            // 无法获取数据库连接
            std::string response = "{\"status\": \"error\", \"message\": \"Database connection failed\"}";
            auto frame = createWebSocketFrameWrapper(response);
            send(sockfd, frame.data(), frame.size(), 0);
        }
    } catch (const std::exception& e) {
        // 处理异常
        std::string response = "{\"status\": \"error\", \"message\": \"Error processing log: " + std::string(e.what()) + "\"}";
        auto frame = createWebSocketFrameWrapper(response);
        send(sockfd, frame.data(), frame.size(), 0);
    }
}

HttpResponse handleLogFileDownload(const HttpRequest& request, ClientSession& session) {
    HttpResponse response;
    
    // 获取查询参数中指定的文件类型
    std::string fileType = "html"; // 默认html
    if (request.queryParams.find("type") != request.queryParams.end()) {
        fileType = request.queryParams.at("type");
    }
    
    std::string filePath = std::filesystem::current_path().string() + "/log." + fileType;
    
    // 检查文件是否存在
    if (!std::filesystem::exists(filePath)) {
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = "Log file not found";
        return response;
    }
    
    // 读取文件内容
    std::ifstream file(filePath, std::ios::binary);
    std::ostringstream contentStream;
    contentStream << file.rdbuf();
    response.body = contentStream.str();
    
    // 设置响应头
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = (fileType == "html") ? "text/html" : "text/plain";
    response.headers["Content-Disposition"] = "attachment; filename=\"log." + fileType + "\"";
    response.headers["Content-Length"] = std::to_string(response.body.size());
    
    return response;
}