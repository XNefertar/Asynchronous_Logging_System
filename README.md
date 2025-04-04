# 异步日志系统 (Asynchronous Logging System)

## 项目概览

异步日志系统是一个高性能、多功能的分布式日志收集与存储解决方案。该系统采用Epoll事件驱动模型和MySQL数据库存储，支持日志级别过滤、防SQL注入、数据持久化等功能。

## 系统架构

### 核心组件

- **Epoll服务器**：基于Linux Epoll的高性能事件驱动服务器
- **MySQL连接池**：数据库连接池，提高数据库操作效率
- **日志过滤器**：根据日志级别过滤需要存储的日志
- **安全处理**：SQL预处理机制防止注入攻击

### 技术栈

- C++11/17
- MySQL数据库
- Linux Epoll
- JSON响应格式（仿JSON格式构造响应并解析，未调用外部库）

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

### 3. 客户端会话管理

- 跟踪每个客户端的连接状态
- 统计消息数量、总字节数、会话持续时间

### 4. 安全特性

- 使用MySQL预处理语句，防止SQL注入攻击
- 错误处理与日志记录

### 5. 数据结构优化

- 针对日志场景优化的数据库表设计
- 使用索引提高查询效率

## 安装与使用

### 依赖项

- GCC 7.0+（支持C++17）
- MySQL 5.7+
- MySQL Connector/C++

### 编译步骤

```bash
# 克隆仓库
git clone git@github.com:XNefertar/Asynchronous_Logging_System.git
cd Asynchronous_Logging_System

# 编译项目
mkdir build && cd build
cmake ..
make
# 或者使用第二种方式(依然是依赖 cmake)
cmake -B build
cmake --build build
```

### 运行服务器

```bash
# 启动服务器
./build/epoll_server port[your_port]
```

## 数据库结构

```mysql
CREATE TABLE IF NOT EXISTS log_table (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, 
    level ENUM('TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'FATAL') NOT NULL, 
    ip VARCHAR(45) NOT NULL,  -- 支持完整的IPv6地址
    port SMALLINT UNSIGNED NOT NULL, 
    message TEXT, 
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_timestamp (timestamp),
    INDEX idx_level (level)
) ENGINE=InnoDB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci COMMENT='系统日志表';
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

## TODO

- 添加日志压缩功能
- 实现分布式日志收集
- 增加Web管理界面
- 支持更多数据库后端

## 许可证

### MIT License