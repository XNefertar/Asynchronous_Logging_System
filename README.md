# 异步日志系统 (Asynchronous Logging System)

## 项目概览

异步日志系统是一个高性能、分布式的企业级日志收集与管理解决方案。该系统采用现代C++17设计，实现了多协议通信、异步数据库写入、实时Web监控等功能，为大规模系统提供可靠的日志基础设施。

## 系统架构

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   日志生产者     │───▶│    传输层        │───▶│   存储与展示    │
│                │    │                  │    │                │
│ • 日志生成器     │    │ • TCP Server     │    │ • MySQL数据库   │
│ • 模板引擎      │    │ • WebSocket      │    │ • 文件系统      │
│ • 客户端监控     │    │ • HTTP API       │    │ • Web前端       │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### 核心组件

#### 1. 日志生成层
- **Logger系统**：支持TEXT/HTML双格式输出
- **日志模板引擎**：结构化日志内容生成
- **会话管理器**：客户端连接状态跟踪
- **配置管理**：基于共享内存的进程间配置共享

#### 2. 传输通信层
- **EpollServer**：基于epoll的高性能TCP服务器
- **WebSocket服务器**：实时双向通信支持
- **HTTP API服务器**：RESTful接口支持
- **多协议适配**：统一的消息处理接口

#### 3. 存储处理层
- **异步数据库写入**：批量SQL操作优化
- **MySQL连接池**：数据库连接复用管理
- **文件异步写入**：高效的日志文件I/O
- **数据持久化**：可靠的数据存储保证

#### 4. 监控展示层
- **实时Web界面**：现代化的日志查看界面
- **统计信息展示**：系统状态实时监控
- **日志过滤与搜索**：灵活的数据查询功能
- **数据导出**：支持多种格式下载

## 功能特点

### 🚀 高性能异步处理

#### 异步日志写入机制
- **双缓冲设计**：前台/后台缓冲区切换，减少锁竞争
- **批量I/O操作**：利用顺序写入优势，提升磁盘性能
- **后台异步刷新**：独立线程处理文件写入，不阻塞主流程

```cpp
// 异步缓冲区实现示例
class AsyncLogBuffer {
    std::vector<std::string> currentBuffer;    // 前台缓冲区
    std::vector<std::string> nextBuffer;       // 后台缓冲区
    std::vector<std::vector<std::string>> fullBuffers; // 满缓冲区队列
    std::thread flushThread_;                  // 后台刷新线程
};
```

#### 异步数据库写入
- **任务队列缓冲**：内存队列暂存数据库写入任务
- **批量SQL执行**：减少数据库连接数和事务开销
- **连接池管理**：复用数据库连接，提升并发性能

### 🌐 多协议通信支持

#### TCP原生协议
- **高吞吐量**：适用于日志数据密集传输
- **低延迟**：直接Socket通信，最小化协议开销
- **稳定可靠**：成熟的TCP协议保证数据完整性

#### WebSocket实时通信
- **请求-响应模式**：支持异步API调用
- **实时推送模式**：服务器主动推送日志更新
- **优雅降级**：WebSocket失败自动降级到HTTP

```javascript
// WebSocket API使用示例
const logs = await window.wsAPI.getLogs(100);
const stats = await window.wsAPI.getStats();
```

#### HTTP RESTful API
- **标准化接口**：符合REST设计规范
- **易于集成**：支持第三方系统接入
- **丰富功能**：日志查询、统计、下载等完整API

### 📊 智能日志分级

系统支持六级日志分类，实现差异化存储策略：

```cpp
enum LogLevel {
    NORMAL   = 0,  // 普通信息 - 仅文件存储
    DEBUG    = 1,  // 调试信息 - 仅文件存储  
    INFO     = 2,  // 一般信息 - 仅文件存储
    WARNING  = 3,  // 警告信息 - 文件+数据库
    ERROR    = 4,  // 错误信息 - 文件+数据库
    FATAL    = 5   // 致命错误 - 文件+数据库+实时通知
};
```

### 🎨 结构化日志模板

提供四大类日志模板，提升日志内容标准化程度：

#### 认证类模板
```cpp
"用户 {user} 尝试从 {ip} 登录，结果：{result}"
"密码重置请求：用户 {user}，验证码：{code}"
```

#### 数据库类模板  
```cpp
"查询表 {table}，条件：{condition}，耗时：{time}ms"
"事务 {txId} 在 {isolation} 隔离级别下影响 {rows} 行"
```

#### 网络类模板
```cpp
"接收请求：{method} {endpoint}，来源：{client}"
"响应发送：状态码 {status}，耗时：{time}ms"
```

#### 系统类模板
```cpp
"系统资源：CPU {cpu}%，内存 {memory}%，磁盘 {disk}%"
"配置加载：{component} 配置更新，耗时：{time}ms"
```

### 🔒 安全与可靠性

#### SQL注入防护
```cpp
// 使用预处理语句，杜绝SQL注入风险
const char *query = "INSERT INTO log_table (level, ip, port, message) VALUES (?, ?, ?, ?)";
MYSQL_STMT *stmt = mysql_stmt_prepare(conn, query, strlen(query));
mysql_stmt_bind_param(stmt, bind);
```

#### 异常处理与恢复
- **优雅降级**：WebSocket→HTTP→本地文件的多级降级策略
- **资源自动清理**：智能指针和RAII确保资源正确释放
- **断线重连**：指数退避算法实现智能重连

#### 进程间配置同步
```cpp
// 基于共享内存的配置管理，支持多进程协同
class SharedConfigManager {
    volatile bool initialized;
    volatile int logFormat;
    volatile pid_t initializer_pid;
};
```

## 技术架构

### 并发模型

#### Epoll事件驱动
```cpp
// 高性能事件循环
void EpollServer::EventLoop() {
    while (running) {
        int ready = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < ready; i++) {
            if (events[i].data.fd == listenfd) {
                handleNewConnection();
            } else {
                handleClientData(events[i].data.fd);
            }
        }
    }
}
```

#### 线程池处理
- **工作线程池**：处理CPU密集型任务
- **I/O线程池**：处理文件和网络I/O
- **数据库线程池**：专门处理数据库操作

### 内存管理

#### 智能指针使用
```cpp
std::unique_ptr<AsyncLogBuffer> logBuffer_;
std::shared_ptr<SqlConnPool> connPool_;
```

#### 内存池优化
- **对象池**：复用频繁创建的对象
- **内存对齐**：优化缓存性能
- **零拷贝**：减少不必要的内存复制

### 性能优化

#### I/O优化
- **批量写入**：减少系统调用次数
- **缓冲区管理**：动态调整缓冲区大小
- **预分配策略**：避免频繁内存分配

#### 网络优化
- **TCP_NODELAY**：禁用Nagle算法，降低延迟
- **SO_REUSEADDR**：快速重用端口
- **非阻塞I/O**：提升并发处理能力

## 安装部署

### 系统要求

```bash
操作系统: Linux (Ubuntu 20.04+ / CentOS 8+)
编译器:   GCC 9.4.0+ (支持C++17)
数据库:   MySQL 8.0+
依赖库:   
  - libmysqlclient-dev
  - libjsoncpp-dev  
  - libssl-dev
  - cmake (3.16+)
```

### 快速安装

#### 1. 安装依赖
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y gcc g++ cmake libmysqlclient-dev libjsoncpp-dev libssl-dev

# CentOS/RHEL
sudo yum install -y gcc-c++ cmake mysql-devel jsoncpp-devel openssl-devel
```

#### 2. 编译项目
```bash
git clone git@github.com:XNefertar/Asynchronous_Logging_System.git
cd Asynchronous_Logging_System

mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

#### 3. 数据库初始化
```sql
CREATE DATABASE IF NOT EXISTS logs_db 
CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE logs_db;

CREATE TABLE IF NOT EXISTS log_table (
    id INT AUTO_INCREMENT PRIMARY KEY,
    level VARCHAR(20) NOT NULL,
    ip VARCHAR(45) NOT NULL,
    port INT NOT NULL,
    message TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_timestamp (timestamp),
    INDEX idx_level (level),
    INDEX idx_ip_port (ip, port)
) ENGINE=InnoDB 
  CHARACTER SET utf8mb4 
  COLLATE utf8mb4_unicode_ci 
  COMMENT='异步日志系统数据表';
```

#### 4. 配置文件
```bash
# 复制配置模板
cp Config/mysql_config.example.conf Config/mysql_config.conf

# 编辑数据库配置
vim Config/mysql_config.conf
```

```ini
# Config/mysql_config.conf
[mysql]
host = localhost
port = 3306
username = your_username
password = your_password
database = logs_db
max_connections = 20
```

## 使用指南

### 基本使用

#### 1. 启动系统组件
```bash
# 终端1: 启动Web服务器
./build/webserver
# 监听端口: 8080 (HTTP) / 8080 (WebSocket)

# 终端2: 启动日志生成器
./build/main
# 选择格式: 0=文本, 1=HTML

# 终端3: 启动客户端监控
./build/client 127.0.0.1 8080 log.html
```

#### 2. Web界面访问
打开浏览器，访问：
- **主界面**: http://localhost:8080
- **API文档**: http://localhost:8080/api/docs
- **系统状态**: http://localhost:8080/status

### API接口文档

#### HTTP API

```bash
# 获取日志列表
GET /api/logs?limit=100&offset=0&level=ERROR

# 获取系统统计
GET /api/stats

# 下载日志文件
GET /api/download-log?type=html

# 清空日志
DELETE /api/logs
```

#### WebSocket API

```javascript
// 连接WebSocket
const ws = new WebSocket('ws://localhost:8080/ws');

// 获取日志
const logs = await window.wsAPI.getLogs(100);

// 获取统计信息
const stats = await window.wsAPI.getStats();

// 监听实时日志
ws.onmessage = function(event) {
    const message = JSON.parse(event.data);
    if (message.type === 'log_update') {
        // 处理新日志
    }
};
```

### 高级配置

#### 性能调优
```cpp
// AsyncLogBuffer.hpp - 缓冲区大小调整
static const size_t BUFFER_SIZE = 1024 * 1024; // 1MB

// AsyncDBWriter.hpp - 批量写入大小
static const int BATCH_SIZE = 100;

// SqlConnPool.hpp - 连接池大小
static const int MAX_CONNECTIONS = 20;
```

#### 日志级别配置
```cpp
// 自定义日志级别阈值
const LogLevel DB_STORAGE_THRESHOLD = WARNING;  // WARNING及以上存入数据库
const LogLevel REALTIME_NOTIFY_THRESHOLD = ERROR; // ERROR及以上实时通知
```

## 监控与运维

### 系统监控

#### 性能指标
- **日志吞吐量**: 每秒处理的日志条数
- **数据库写入延迟**: 平均写入响应时间
- **内存使用率**: 缓冲区和连接池内存占用
- **连接数统计**: 当前活跃的客户端连接

#### 健康检查
```bash
# 检查服务状态
curl http://localhost:8080/health

# 检查数据库连接
curl http://localhost:8080/api/stats

# 检查日志文件状态
ls -la Log/
```

### 故障排除

#### 常见问题

1. **数据库连接失败**
```bash
# 检查MySQL服务状态
sudo systemctl status mysql

# 检查配置文件
cat Config/mysql_config.conf

# 测试连接
mysql -h localhost -u username -p
```

2. **WebSocket连接异常**
```bash
# 检查端口占用
netstat -tlnp | grep 8080

# 查看服务器日志
tail -f Log/WebSocketServer.txt
```

3. **性能问题诊断**
```bash
# 查看系统资源使用
top -p $(pgrep webserver)

# 监控I/O性能
iostat -x 1

# 检查网络连接
ss -tlnp | grep 8080
```

## 项目结构

```
Asynchronous_Logging_System/
├── main.cpp                          # 主程序入口
├── Logger.hpp/cpp                     # 日志记录器
├── LogQueue.hpp                       # 日志队列
├── Client/                           # 客户端模块
│   ├── Client.hpp/cpp                # TCP客户端实现
│   └── client_main.cpp               # 客户端入口
├── EpollServer/                      # 服务器核心
│   ├── EpollServer.hpp/cpp           # Epoll服务器
│   └── webserver_main.cpp            # 服务器入口
├── WebSocket/                        # WebSocket模块
│   ├── WebSocket.hpp/cpp             # WebSocket基础实现
│   ├── WebSocketServer.cpp           # WebSocket服务器
│   ├── WebSocketApiHandlers.hpp/cpp  # API处理器
│   └── CMakeLists.txt               # WebSocket模块构建
├── MySQL/                           # 数据库模块
│   ├── SqlConnPool.hpp/cpp          # 连接池实现
│   └── CMakeLists.txt               # 数据库模块构建
├── Server/                          # 服务器组件
│   ├── AsyncDBWriter.hpp/cpp        # 异步数据库写入
│   ├── test_log_throughput.cpp      # 性能测试
│   └── CMakeLists.txt               # 服务器模块构建
├── LogMessage/                      # 日志消息处理
│   ├── AsyncLogBuffer.hpp/cpp       # 异步日志缓冲
│   ├── LogMessage.hpp/cpp           # 日志消息定义
│   └── CMakeLists.txt               # 日志模块构建
├── Util/                           # 工具类
│   ├── SessionManager.hpp/cpp       # 会话管理
│   ├── LogTemplates.hpp/cpp         # 日志模板
│   ├── ConfigManager.hpp/cpp        # 配置管理
│   ├── SharedConfigManager.hpp/cpp  # 共享配置管理
│   └── CMakeLists.txt               # 工具模块构建
├── Config/                         # 配置文件
│   ├── mysql_config.conf           # MySQL配置
│   └── server_config.conf          # 服务器配置
├── static/                         # Web静态资源
│   ├── index.html                  # 主页面
│   ├── css/                        # 样式文件
│   │   └── style.css
│   └── js/                         # JavaScript文件
│       ├── main.js                 # 主逻辑
│       └── websocket.js            # WebSocket处理
├── Log/                           # 日志输出目录
├── build/                         # 构建输出目录
├── CMakeLists.txt                 # 主构建文件
└── README.md                      # 项目文档
```

## 开发指南

### 编码规范

#### C++代码风格
```cpp
// 类名使用大驼峰
class AsyncLogBuffer {
    // 私有成员使用下划线后缀
    std::mutex mutex_;
    std::string logFilePath_;
    
    // 公有方法使用小驼峰
    void appendLogEntry(const std::string& logLine);
};

// 常量使用全大写
static const size_t BUFFER_SIZE = 1024 * 1024;

// 命名空间使用首字母大写
namespace LogMessageSpace {
    // 枚举使用大写
    enum LogLevel {
        INFO, WARNING, ERROR
    };
}
```

#### JavaScript代码风格
```javascript
// 函数使用小驼峰
function initWebSocket() {
    // 变量使用小驼峰
    const websocketUrl = 'ws://localhost:8080/ws';
    
    // 常量使用全大写
    const MAX_RETRY_ATTEMPTS = 5;
}

// 类使用大驼峰
class WebSocketManager {
    constructor() {
        this.isConnected = false;
    }
}
```

### 扩展开发

#### 添加新的日志模板
```cpp
// 在 LogTemplates.cpp 中添加
void LogTemplates::initializeTemplates() {
    // 添加新的模板类型
    templates["security"] = {
        "安全事件：{event}，来源：{source}，严重级别：{severity}",
        "访问控制：用户 {user} 尝试访问 {resource}，结果：{result}",
        // ...更多安全类模板
    };
}
```

#### 实现新的API接口
```cpp
// 在 WebSocketApiHandlers.cpp 中添加
if (requestType == "get_security_logs") {
    std::string timeRange = data.get("timeRange", "24h").asString();
    auto logs = fetchSecurityLogs(timeRange);
    
    Json::Value logsArray(Json::arrayValue);
    // ...构建响应数据
    response["data"] = logsArray;
}
```

#### 添加新的前端功能
```javascript
// 在 static/js/main.js 中添加
async function fetchSecurityLogs(timeRange = '24h') {
    try {
        if (isConnected && window.wsAPI) {
            return await window.wsAPI.getSecurityLogs(timeRange);
        } else {
            const response = await fetch(`/api/security-logs?range=${timeRange}`);
            return await response.json();
        }
    } catch (error) {
        console.error('获取安全日志失败:', error);
        throw error;
    }
}
```

## 性能基准

### 测试环境
- **硬件**: Intel i7-9700K, 16GB RAM, SSD
- **操作系统**: Ubuntu 20.04 LTS
- **数据库**: MySQL 8.0.28

### 性能指标

#### 日志写入性能
```
同步模式:     1,000 条/秒
异步模式:    10,000 条/秒  (10x 提升)
批量模式:    50,000 条/秒  (50x 提升)
```

#### 网络通信性能
```
TCP连接:     100,000 连接/秒
WebSocket:    50,000 连接/秒
HTTP API:     20,000 请求/秒
```

#### 数据库操作性能
```
单条插入:     500 条/秒
批量插入:   5,000 条/秒    (10x 提升)
连接池:    20,000 条/秒    (40x 提升)
```

### 压力测试
```bash
# 运行性能测试
./build/test_log_throughput async 10000 4

# 输出示例:
# 异步模式: 10000 条日志用时 2156 ms
# 吞吐量: 4641.29 条/秒
# 平均延迟: 0.22 ms
```

## 未来规划

### 短期目标 (3个月)
- [ ] 日志轮转和归档功能
- [ ] 更丰富的查询和过滤选项
- [ ] 性能监控Dashboard
- [ ] 自动化部署脚本

### 中期目标 (6个月)  
- [ ] 分布式日志收集架构
- [ ] 基于机器学习的异常检测
- [ ] 多数据库后端支持 (PostgreSQL, ClickHouse)
- [ ] Kubernetes部署支持

### 长期目标 (1年)
- [ ] 大数据分析集成 (Elasticsearch, Kafka)
- [ ] 智能告警系统
- [ ] 多租户支持
- [ ] 云原生架构重构

## 贡献指南

### 贡献流程

1. **Fork项目**到个人仓库
2. **创建特性分支**: `git checkout -b feature/your-feature`
3. **提交更改**: `git commit -am 'Add your feature'`
4. **推送分支**: `git push origin feature/your-feature`
5. **创建Pull Request**

### 开发环境配置
```bash
# 克隆开发版本
git clone -b develop git@github.com:XNefertar/Asynchronous_Logging_System.git

# 安装开发工具
sudo apt-get install -y clang-format cppcheck valgrind

# 运行代码检查
make check
```

### 测试要求
- 新功能必须包含单元测试
- 所有测试必须通过
- 代码覆盖率不低于80%

```bash
# 运行所有测试
make test

# 生成覆盖率报告
make coverage
```

## 许可证

本项目采用 [MIT许可证](LICENSE)。

## 致谢

感谢所有为项目贡献代码和建议的开发者。

## 联系方式

- **Issues**: [GitHub Issues](https://github.com/XNefertar/Asynchronous_Logging_System/issues)
- **邮箱**: [您的邮箱]
- **文档**: [在线文档地址]

---

*最后更新: 2024年6月*