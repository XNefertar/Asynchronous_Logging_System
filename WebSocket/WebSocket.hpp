#ifndef __WEBSOCKET_HELPERS_HPP__
#define __WEBSOCKET_HELPERS_HPP__

#include <vector>
#include <map>
#include <string>
#include "../MySQL/SqlConnPool.hpp"
#include "../EpollServer/EpollServer.hpp"

using namespace EpollServerSpace;

// 从数据库获取日志数据
std::vector<std::map<std::string, std::string>> fetchLogsFromDatabase(int limit = 100, int offset = 0, const std::string& levelFilter = "");

// 获取日志总数
int getTotalLogsCount();

// 获取特定级别日志数
int getLogCountByLevel(const std::string& level);

// 在外部创建WebSocket帧的封装
std::vector<char> createWebSocketFrameWrapper(const std::string& message);

// 广播WebSocket消息的封装
void broadcastWebSocketMessageWrapper(const std::string& message);

// 设置全局服务器实例引用
void setGlobalServerReference(EpollServer* server);

extern EpollServer* g_server; // 声明外部全局服务器变量

#endif // __WEBSOCKET_HELPERS_HPP__