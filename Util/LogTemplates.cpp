#include "LogTemplates.hpp"

std::string LogTemplates::getRandomTemplate(const std::string& type) {
    const std::vector<std::string>* templates = &systemTemplates;
    
    if (type == "auth") templates = &authTemplates;
    else if (type == "database") templates = &databaseTemplates;
    else if (type == "network") templates = &networkTemplates;
    
    std::uniform_int_distribution<> dis(0, templates->size() - 1);
    return (*templates)[dis(getGenerator())];
}

std::mt19937& LogTemplates::getGenerator() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

// 在文件末尾添加

std::string LogTemplates::replacePlaceholders(const std::string& templateStr,
                                            const std::map<std::string, std::string>& values) {
    std::string result = templateStr;
    for (const auto& pair : values) {
        // 检查键和值是否为空
        if (pair.first.empty() || pair.second.empty()) continue;
        
        std::string placeholder = "{" + pair.first + "}";
        size_t pos = result.find(placeholder);
        while (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), pair.second);
            pos = result.find(placeholder, pos + pair.second.length());
        }
    }
    return result;
}


// Define template contents
const std::vector<std::string> LogTemplates::authTemplates = {
    "User {user} login attempt [IP: {ip}]",
    "Password reset request - User ID: {userId}",
};

const std::vector<std::string> LogTemplates::databaseTemplates = {
    "Database query executed - Table: {table}, Condition: {condition}, Duration: {time}ms",
    "Record updated - Table: {table}, ID: {id}, Fields: {fields}",
    "Transaction started - ID: {txId}, Isolation level: {isolation}",
    "Transaction committed - ID: {txId}, Affected rows: {rows}",
    "Connection pool status - Active: {active}, Idle: {idle}, Waiting: {waiting}"
};

const std::vector<std::string> LogTemplates::networkTemplates = {
    "Request received - Endpoint: {endpoint}, Method: {method}, Size: {size} bytes",
    "Response sent - Endpoint: {endpoint}, Status code: {status}, Duration: {time}ms",
    "Connection established - Client: {client}, Protocol: {protocol}, Connection ID: {connId}",
    "Data received - Source: {source}, Size: {size} bytes, Type: {type}",
    "Connection closed - Client: {client}, Reason: {reason}, Duration: {duration} seconds"
};

const std::vector<std::string> LogTemplates::systemTemplates = {
    "System resource usage - CPU: {cpu}%, Memory: {memory}MB, Disk IO: {io}KB/s",
    "Configuration loaded - File: {file}, Items: {items}",
    "Scheduled task executed - Task: {task}, Status: {status}, Duration: {time}ms",
    "Component initialization - Name: {name}, Status: {status}, Dependencies: {dependencies}",
    "System state changed - From: {oldState}, To: {newState}, Trigger: {trigger}"
};