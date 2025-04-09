# 异步日志系统 (Asynchronous Logging System)

## 项目概览

异步日志系统是一个高性能、多功能的分布式日志收集与存储解决方案。该系统采用多层架构设计，支持日志生成、传输、存储和展示四个核心环节，实现了日志数据的高效采集和实时处理。

## 系统架构

### 核心组件

- **日志生成器**：生成结构化日志并写入本地文件
- **日志模板引擎**：提供多种类型的日志模板，支持占位符替换
- **客户端监控器**：监控日志文件变化并通过网络发送
- **EpollServer**：基于Linux Epoll的高性能TCP/WebSocket服务器
- **数据库连接池**：优化数据库连接管理，提高性能
- **会话管理器**：管理客户端连接会话状态
- **前端展示系统**：实时展示日志数据和统计信息

### 数据流

1. **日志生成**：main.cpp→ Logger → log.txt/log.html
2. **日志传输**：Client → TCP/WebSocket → Server
3. **数据处理**：EpollServer → 数据解析 → 数据库插入
4. **数据查询**：前端 → API请求 → 数据库查询 → 前端展示

## 功能特点

### 1. 日志级别支持

系统支持六种日志级别，从低到高排列：

- **NORMAL**：普通信息
- **DEBUG**：调试信息
- **INFO**：一般信息
- **WARNING**：警告信息（及以上级别存入数据库）
- **ERROR**：错误信息
- **FATAL**：致命错误

### 2. 多渠道存储

- 文件存储：所有日志保存到文件系统
- 数据库存储：WARNING及以上级别日志存入MySQL数据库

### 3. 日志模板系统

提供四种类型的日志模板，使日志内容更加结构化：

- **认证类(auth)**：用户登录、密码重置等认证相关日志
- **数据库类(database)**：数据库查询、事务操作等数据库操作日志
- **网络类(network)**：请求接收、响应发送等网络通信日志
- **系统类(system)**：系统资源使用、配置加载等系统操作日志

### 4. 多种日志格式支持

- **文本日志**：简洁的文本格式，适合命令行查看
- **HTML日志**：带CSS样式的HTML格式，方便在浏览器中查看
- **结构化消息**：使用模板系统生成结构化日志内容

### 5. 多种通信协议

- **原生TCP**：高性能、低延迟的TCP通信
- **WebSocket**：支持实时双向通信，适合前端交互
- **HTTP**：支持RESTful API接口，方便系统集成

### 6. 客户端会话管理

- 跟踪每个客户端的连接状态
- 统计消息数量、总字节数、会话持续时间
- 提供会话上下文信息用于日志增强

### 7. 安全特性

- 使用MySQL预处理语句，防止SQL注入攻击
- 错误处理与日志记录
- 内存安全管理

## 安装与使用

### 依赖项

- GCC 9.4.0+（支持C++17）
- MySQL 8.0+
- OpenSSL 1.1.1+
- CMake 3.16+

### 编译步骤

```bash
# 克隆仓库
git clone git@github.com:XNefertar/Asynchronous_Logging_System.git
cd Asynchronous_Logging_System

# 编译项目
mkdir -p build && cd build
cmake ..
make
```

### 运行系统

#### 1. 准备数据库

```sql
CREATE DATABASE logs_db;
USE logs_db;

CREATE TABLE IF NOT EXISTS log_table (
    id INT AUTO_INCREMENT PRIMARY KEY,
    level VARCHAR(20) NOT NULL,
    message TEXT NOT NULL,
    timestamp DATETIME NOT NULL,
    INDEX idx_timestamp (timestamp),
    INDEX idx_level (level)
) ENGINE=InnoDB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci COMMENT='系统日志表';
```

#### 2. 启动各组件

```bash
# 启动日志生成器
./build/main
# 选择日志格式（0=文本，1=HTML）

# 启动服务器
./build/webserver

# 启动客户端
./build/client 127.0.0.1 8080 log.html
```

#### 3. 访问前端界面

在浏览器中打开 `http://localhost:8080` 查看日志展示页面。

## 项目结构

```txt
Asynchronous_Logging_System/
├── main.cpp                 # 主程序入口，日志生成器
├── Logger.hpp               # 日志记录器头文件
├── LogQueue.hpp             # 日志队列实现
├── Client/                  # 客户端模块
│   ├── Client.hpp           # 客户端头文件
│   └── Client.cpp           # 客户端实现
├── EpollServer/             # Epoll服务器模块
│   ├── EpollServer.hpp      # Epoll服务器头文件
│   ├── EpollServer.cpp      # Epoll服务器实现
├── WebSocket/               # WebSocket模块
│   ├── WebSocket.hpp        # WebSocket基础头文件
│   ├── WebSocket.cpp        # WebSocket基础实现
│   ├── WebSocketServer.cpp  # WebSocket服务器实现
│   ├── WebSocketApiHandlers.hpp # API处理器头文件
│   └── WebSocketApiHandlers.cpp # API处理器实现
├── MySQL/                   # 数据库连接模块
│   ├── SqlConnPool.hpp      # 数据库连接池头文件
│   └── SqlConnPool.cpp      # 数据库连接池实现
├── Util/                    # 工具类
│   ├── SessionManager.hpp   # 会话管理器头文件
│   ├── SessionManager.cpp   # 会话管理器实现
│   ├── LogTemplates.hpp     # 日志模板头文件
│   └── LogTemplates.cpp     # 日志模板实现
└── static/                  # 静态文件目录
```

## 代码示例

### 日志模板使用

```c++
// 使用日志模板系统生成结构化日志
std::string tmpl = LogTemplates::getRandomTemplate("database");
std::map<std::string, std::string> values = {
    {"table", "users"},
    {"condition", "id > 1000"},
    {"time", "15"}
};
std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
logger.log_in_html(WARNING, logMsg + " at " + time_buffer);
```

### 会话管理

```c++
// 获取一个会话信息
ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();

// 使用会话信息构造日志上下文
std::map<std::string, std::string> values = {
    {"client", session.ip},
    {"port", std::to_string(session.port)},
    {"duration", std::to_string(time(nullptr) - session.connect_time)}
};
```

### WebSocket通信

```c++
// 广播消息给所有WebSocket客户端
std::string message = "{\"level\":\"ERROR\",\"message\":\"System failure\"}";
broadcastWebSocketMessage(message);
```

## 安全性考虑

系统使用MySQL预处理语句防止SQL注入攻击：

```c++
// 使用占位符(?)，而非直接字符串拼接
const char *query = "INSERT INTO log_table VALUES (?, ?, ?, ?)";
// 参数绑定处理
MYSQL_BIND bind[4];
// ...绑定参数...
mysql_stmt_execute(stmt);
```

## 未来计划

- 日志轮转和归档功能
- 分布式日志收集架构
- 更丰富的前端可视化
- AI辅助日志分析
- 更多数据库后端支持

## 许可证

本项目采用MIT许可证。详细信息请参见LICENSE文件。

## 贡献

欢迎提交Pull Request或Issue来帮助改进项目。在提交代码前，请确保您的代码符合项目的编码规范。

## 联系方式

如有问题，请通过GitHub Issues联系我们。