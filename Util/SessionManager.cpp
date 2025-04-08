#include "SessionManager.hpp"

SessionManager* SessionManager::getInstance() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_instance == nullptr) {
        _instance = new SessionManager();
    }
    return _instance;
}

void SessionManager::addSession(int sockfd, const ClientSessionInfo& session) {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions[sockfd] = session;
}

void SessionManager::removeSession(int sockfd) {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(sockfd);
}

ClientSessionInfo SessionManager::getSession(int sockfd) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_sessions.find(sockfd) != _sessions.end()) {
        return _sessions[sockfd];
    }
    return {"unknown", 0, 0, 0, 0};
}

void SessionManager::addMessageCount(int sockfd, size_t count) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(sockfd);
    if (it != _sessions.end()) {
        it->second.message_count += count;
    }
}

void SessionManager::addTotalBytes(int sockfd, size_t bytes) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(sockfd);
    if (it != _sessions.end()) {
        it->second.total_bytes += bytes;
    }
}

bool SessionManager::modifySession(int sockfd, const ClientSessionInfo& session) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(sockfd);
    if (it != _sessions.end()) {
        it->second = session;
        return true;
    }
    return false;
}

ClientSessionInfo SessionManager::getRandomSession() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_sessions.empty()) {
        // 使用有效的默认值
        return {"localhost", 12345, "default_table", "default_user", time(nullptr), 0, 0};
    }
    auto it = _sessions.begin();
    
    // 验证返回的会话数据有效性
    ClientSessionInfo session = it->second;
    if (session.ip.empty()) session.ip = "localhost";
    if (session.table_name.empty()) session.table_name = "default_table";
    if (session.user_name.empty()) session.user_name = "default_user";
    
    return session;
}

bool SessionManager::updateSession(int sockfd, const ClientSessionInfo& session) {
    return modifySession(sockfd, session);  // 使用现有的 modifySession 方法
}

size_t SessionManager::getSessionCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _sessions.size();
}

// 初始化静态成员
SessionManager* SessionManager::_instance = nullptr;
std::mutex SessionManager::_mutex;
