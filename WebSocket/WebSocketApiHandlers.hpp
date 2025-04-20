#ifndef __WEBSOCKET_API_HANDLERS_HPP__
#define __WEBSOCKET_API_HANDLERS_HPP__

#include "../EpollServer/EpollServer.hpp"

using namespace EpollServerSpace;

// API 处理函数
HttpResponse handleLogsApi(const HttpRequest& request, ClientSession& session);
HttpResponse handleStatsApi(const HttpRequest& request, ClientSession& session);

// WebSocket 消息处理函数
void handleWebSocketMessage(int sockfd, const std::string& message, ClientSession& session);

// 广播日志更新
void broadcastLogUpdate(const std::string& level, const std::string& message);

// 数据库插入日志功能
void insertWebSocketMessage(int sockfd, const std::string& message, ClientSession& session);

HttpResponse handleLogFileDownload(const HttpRequest& request, ClientSession& session);

#endif // __WEBSOCKET_API_HANDLERS_HPP__