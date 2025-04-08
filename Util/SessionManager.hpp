#ifndef __SESSION_MANAGER_HPP__
#define __SESSION_MANAGER_HPP__

#include <unordered_map>
#include <string>
#include <mutex>

struct ClientSessionInfo {
    std::string ip;
    uint16_t port;
    std::string table_name;
    std::string user_name;
    time_t connect_time;
    size_t total_bytes;
    size_t message_count;

    ClientSessionInfo(const std::string& ip = "unknown", uint16_t port = 0, const std::string& table_name = "default_table",
                      const std::string& user_name = "default_user", time_t connect_time = 0, size_t total_bytes = 0, size_t message_count = 0)
        : ip(ip), port(port), table_name(table_name), user_name(user_name), connect_time(connect_time), total_bytes(total_bytes), message_count(message_count) {}
    ClientSessionInfo(const ClientSessionInfo& other)
        : ip(other.ip), port(other.port), table_name(other.table_name), user_name(other.user_name),
          connect_time(other.connect_time), total_bytes(other.total_bytes), message_count(other.message_count) {}
    ClientSessionInfo& operator=(const ClientSessionInfo& other) {
        if (this != &other) {
            ip = other.ip;
            port = other.port;
            table_name = other.table_name;
            user_name = other.user_name;
            connect_time = other.connect_time;
            total_bytes = other.total_bytes;
            message_count = other.message_count;
        }
        return *this;
    }
    bool operator==(const ClientSessionInfo& other) const {
        return ip == other.ip && port == other.port && table_name == other.table_name &&
               user_name == other.user_name && connect_time == other.connect_time &&
               total_bytes == other.total_bytes && message_count == other.message_count;
    }
};

class SessionManager {
private:
    static SessionManager* _instance;
    static std::mutex _mutex;
    
    std::unordered_map<int, ClientSessionInfo> _sessions;
    
    SessionManager() {}
    
public:
    static SessionManager* getInstance();
    
    void addSession(int sockfd, const ClientSessionInfo& session);
    
    void removeSession(int sockfd);
    
    ClientSessionInfo getSession(int sockfd);
    ClientSessionInfo getRandomSession();

    bool modifySession(int sockfd, const ClientSessionInfo& session);

    void addMessageCount(int sockfd, size_t count);
    void addTotalBytes(int sockfd, size_t bytes);

    // 更新现有会话信息
    bool updateSession(int sockfd, const ClientSessionInfo& info);

    // 获取当前会话总数
    size_t getSessionCount() const;
};

#endif