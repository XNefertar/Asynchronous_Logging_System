# EpollServer 组件简介



## 基础知识

### 什么是 Epoll 模式

epoll，是IO多路转接的一种模式，属于五种常用的IO模式的一种，它们分别是：

> 1. 阻塞式IO
> 2. 非阻塞式IO
> 3. IO多路复用
> 4. 信号驱动式IO
> 5. 异步IO

除去第五种是真正的异步IO，其他的都是建立在同步IO基础上实现的IO模型，而IO多路转接模型可以认为是在同步IO模型下资源利用率较大的一种机制，它可以同时等待多个文件描述符的就绪状态，而并非串行等待；

而其中的epoll模型则是IO多路转接模型下效率最高的。

### Reactor 模式和 Proactor 模式

> **Reactor 模式：应用程序主动去处理 I/O 事件。**
> **Proactor 模式：操作系统处理 I/O，应用程序只管处理结果。**

简单来说二者是同步（Reactor）和异步（Proactor）的区别，而异步编程的实现难度较高，需要调用特殊的异步IO的接口，所以我们采用同步（Reactor）模式实现；

### Reactor 模式的分类

__单Reactor 单进程/线程__

![image1](./PNG/Single_Reactor_Single_Process.png)

__单Reactor 多进程/线程__

![image2](./PNG/Single_Reactor_Multi_Process.png)

__多Reactor 多进程/多线程__

![image3](./PNG/Multi_Reactor_Multi_Process.png)

## 组件介绍

这里的EpollServer在最开始实现时，目的是同Client端建立连接，并处理从Client端发来的数据，返回对应的回显消息；

但是后面做了升级，加入了网页端的内容，主要由WebServer（其内部依赖websocket协议）负责，之后EpollServer就变成了一个类似接口的类了，主要用来实现WebServer的调用接口，所以这里主要讲解EpollServer作为接口的作用；

### 关于 HTTP 方面的实现

我们知道，一个网页端的实现需要HTTP参与，即使是websocket协议，在一开始升级协议时，也需要通过HTPP发送升级请求，所以我们需要实现一个HTTP处理系统，包括HTTP报文的解析和响应报文的构建两方面。

**HTTP报文格式**
HTTP报文分为两种，一个是请求报文，一个是响应报文：

1. __请求报文__

   请求报文：请求行 + 请求头部字段 + 空行(\r\n) + [请求主体]

   - 请求行：请求方法 + 请求资源路径 + HTTP版本
   - 请求头部字段：一般是由键值对构成，包含了客户端的基本信息等
   - 空行：即\r\n，用于将请求主体和上面的内容分隔开
   - 请求主体：一般GET请求不带请求主体，而POST/PUT/PUTCH请求会带请求主体

2. __响应报文__

   响应报文：状态行 + 响应头部字段 + 响应主体

   - 状态行：HTTP版本 + 状态码 + 状态信息
   - 响应头部字段：一般是由键值对构成，包含了服务端的基本信息等
   - 空行：即\r\n，用于将请求主体和上面的内容分隔开
   - 响应主体：是服务端返回客户端的实际数据内容，通常是客户端请求资源的**核心内容部分**，常见的格式有：
     - 网页HTML源码
     - JSON数据
     - 图片、音频、视频等二进制数据
     - 文件内容（如 PDF、ZIP 等）

**具体的解析和构建方式**

1. #### 解析HTTP请求：`parseHttpRequest(const std::string& requestStr, HttpRequest& request)`函数

   __解析请求行__

   > 我们可以使用`std::getline`函数读取一整行的字符串（这里就是请求行），但是读取后如何按照空格进行分隔呢？
   >
   > 可以使用`std::istringstream`对字符串按照空格进行分隔：
   > `std::istringstream`允许像处理文件流一样处理字符串，主要用于从字符串中提取有格式的数据。它将一个字符串包装成一个流对象，然后可以使用 `>>` 运算符或其他流操作从中读取数据，常用于将按照空格分隔的字符串拆分成多个部分；

   ```C++
   // parseHttpRequest 函数对请求行进行处理的部分
   bool parseHttpRequest(const std::string& requestStr, HttpRequest& request){
       std::istringstream requestStream(requestStr);
       std::string line;
       if(std::getline(requestStream, line)){
            // 删除请求行中的回车符
   		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
           std::istringstream requestLineStream(line);
           std::string method, resource, version;
           requestLineStream >> method >> resource >> version;
           
           // 剩余部分
           // ...
       }
   }
   ```

   为了更方便的表示HTTP各部分结构，我们定义了一个结构体，用来存储每一个HTTP请求的相关信息：

   ```c++
   struct HttpRequest {
           HttpMethod method; 								// 请求方法
           std::string path;								// 请求资源路径 
           std::string version;							// HTTP版本
           std::map<std::string, std::string> headers;		  // string -> string, 请求头部信息, 键值对映射表
           std::string body;								// 请求主体 
           std::map<std::string, std::string> queryParams;	   // 用于解析资源请求路径中的查询参数
       												  // 一般以?开始, &分隔不同键值对, =连接键值对
   };
   ```

   解析路径和查询参数（即构建std::map<string, string> queryParams的映射关系）：

   ```c++
   size_t queryPos = path.find('?');
   if (queryPos != std::string::npos) {
       std::string queryString = path.substr(queryPos + 1);
       path = path.substr(0, queryPos);
       
       std::istringstream queryStream(queryString);
       std::string param;
       while (std::getline(queryStream, param, '&')) {
           size_t equalPos = param.find('=');
           if (equalPos != std::string::npos) {
               std::string key = param.substr(0, equalPos);
               std::string value = param.substr(equalPos + 1);
               request.queryParams[key] = value;
           }
       }
   }
   ```

   完整的解析请求行的代码：

   ```c++
   std::istringstream requestStream(requestStr);
       std::string line;
       
       // 解析请求行
       if (std::getline(requestStream, line)) {
           line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
           std::istringstream requestLineStream(line);
           std::string method, path, version;
           requestLineStream >> method >> path >> version;
           
           // 设置请求方法
           if (method == "GET") request.method = HttpMethod::GET;
           else if (method == "POST") request.method = HttpMethod::POST;
           else if (method == "PUT") request.method = HttpMethod::PUT;
           else if (method == "DELETE") request.method = HttpMethod::DELETE;
           else if (method == "OPTIONS") request.method = HttpMethod::OPTIONS;
           else request.method = HttpMethod::UNKNOWN;
           
           // 解析路径和查询参数
           size_t queryPos = path.find('?');
           if (queryPos != std::string::npos) {
               std::string queryString = path.substr(queryPos + 1);
               path = path.substr(0, queryPos);
               
               std::istringstream queryStream(queryString);
               std::string param;
               while (std::getline(queryStream, param, '&')) {
                   size_t equalPos = param.find('=');
                   if (equalPos != std::string::npos) {
                       std::string key = param.substr(0, equalPos);
                       std::string value = param.substr(equalPos + 1);
                       request.queryParams[key] = value;
                   }
               }
           }
           
           request.path = path;
           request.version = version;
       } else {
           return false;
       }
   	// ...
   }
   ```

   __解析请求头__

   请求头的键值对之间用\r\n隔开，而单个的键值对间使用:进行分隔，处理时注意找到对应标志即可，

   请求头的解析比请求行简单，需要注意前导空格的处理；

   ```c++
   // 解析请求头
   while (std::getline(requestStream, line) && !line.empty() && line != "\r") {
       line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
       size_t colonPos = line.find(':');
       if (colonPos != std::string::npos) {
           std::string key = line.substr(0, colonPos);
           std::string value = line.substr(colonPos + 1);
           // 去除前导空格
           value.erase(0, value.find_first_not_of(" \t"));
           // 构建 key-value 映射关系
           request.headers[key] = value;
       }
   }
   ```

   __解析请求体__

   使用与`std::istringstream`对应的`std::ostringstream`即可

   ```c++
   // 解析请求体
   std::ostringstream bodyStream;
   while (std::getline(requestStream, line)) {
       bodyStream << line << "\n";
   }
   request.body = bodyStream.str();
   ```

2. #### 处理HTTP请求报文

   在完成上面对HTTP的解析后，就需要对解析后的请求报文进行处理，返回客户端请求的对应资源：

   对于不同的请求方法来说，有着不同的处理方式，所以需要建立一个请求方法和对应的回调函数的映射关系：

   ```c++
   std::map<std::string, RequestHandler> _getHandlers;   // GET 请求处理器
   std::map<std::string, RequestHandler> _postHandlers;  // POST 请求处理器
   ```

   对应的有设置回调的函数：

   ```c++
   void addGetHandler(const std::string& path, RequestHandler handler);
   void addPostHandler(const std::string& path, RequestHandler handler);
   ```

   但是这里的EpollServer只作为一个接口类存在，所以没有对应的设置回调逻辑，所有对回调函数的处理都在WebServer中执行

   // TODO: 建立EpollServer -> WebServer的链接

   当回调函数处理完成后，如果直接向客户端发送处理后的内容，客户端时无法正确解析的，因为所有的HTTP报文都需要遵循响应的格式，所以还需要实现一个序列化函数，将处理结果序列化生成一个标准格式的HTTP响应报文

   ```c++
   std::string EpollServer::serializeHttpResponse(const HttpResponse& response) {
       std::ostringstream responseStream;
       
       // 状态行
       responseStream << "HTTP/1.1 " << response.statusCode << " " << response.statusText << "\r\n";
       
       // 响应头
       for (const auto& header : response.headers) {
           responseStream << header.first << ": " << header.second << "\r\n";
       }
       
       // 空行分隔
       responseStream << "\r\n";
       
       // 响应体
       responseStream << response.body;
       
       return responseStream.str();
   }
   
   ```

   序列化之后的响应报文就可以被客户端正确解析，这样我们就完成了基于HTTP协议的服务端-客户端网络通信功能（这里的客户端就是浏览器，其具有解析HTTP响应报文 [格式正确] 的能力）

### 关于websocket协议的升级

我们的网页端是基于websocket协议实现的，而websocket协议需要在原有HTTP协议上进行升级操作，具体的升级请求报文也需要进行特殊处理，所以就需要一个专门的模块实现升级操作；

#### 什么是WebSocket协议

首先我们简要的介绍一下什么是WebSocket协议：
WebSocket协议是一种==全双工==通信协议，用于在客户端（通常是浏览器）和服务器之间建立持久连接，而HTTP/1.x版本只支持半双工通信（尽管其底层是基于TCP实现，TCP是一个全双工协议）；

不仅如此，HTTP协议的局限还有：

- 通信只能由一端发起（客户端，服务端不能主动向客户端发送通知）
- 连接时间短，实时性差（短链接），在HTTP/1.1版本中，尽管支持长连接，但是资源消耗会增大，浪费带宽资源
- 每次请求都需要完整的请求头信息，多个连接并发需要建立多个TCP连接，浪费资源和带宽

基于上面的问题，WebSocket协议应运而生，WebSocket协议会在客户端和服务器之间建立一个持久连接，而且支持双向通信：对于HTTP协议来说，只有当客户端发出请求时，服务器才会根据请求内容推送对应的资源，这是一种低效的半双工通信方式，而WebSocket下，服务器可以主动的向客户端推送消息，无需客户端发送请求。
而且，虽然WebSocket是基于TCP实现的，但是该协议具有低延迟特性: 我们知道TCP性能问题的主要原因是因为以下几点: 
- 连接建立过程中的三次握手
- 断开连接的四次挥手
- 以及数据传输中的拥塞控制机制

然而WebSocket设计时的几大低延迟优势则完美的解决了可能带来的性能开销问题: 
1. 持久化连接，从而减少握手的连接建立开销
2. 极小的帧头开销
    HTTP每次请求都需要完整的头部信息，随着连接数量的增多，带来的资源开销是巨大的，而WebSocket头部精简，资源占用相较于HTTP会小很多
3. 双向通信消除轮询延迟，传统的HTTP会进行轮询检测服务端数据是否发生更新(因为HTTP是半双工通信，服务端无法主动通知客户端)，而WebSocket因其特殊的双向通信机制，服务端可以主动告知客户端数据发生更新，从而避免了因为多次轮询带来的性能损耗
4. WebSocket数据帧还支持二进制数据通信，从而避免了对数据的编码解码带来的性能损耗

#### 如何升级
WebSocket是基于HTTP的协议，具体就体现在WebSocket需要在HTTP连接的基础上进行升级： 
__WebSocket握手方式__
1. 客户端发送升级请求:
```http
GET /chat HTTP/1.1
Host: server.example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13
```
2. 服务器响应: 
```http
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```
> 解释一下这里的升级报文和响应报文:
> __升级报文__
> - 请求行，方法 + URL + HTTP版本
> - 请求头部字段: 
>   1. `Connection`，后面是`Upgrade`，表明客户端想要升级HTTP协议
>   2. `Upgrade`关键字后则是想要升级的协议的具体名称，这里是WebSocket协议
>   3. `Sec-WebSocket-Key`: 这是客户端随机生成的密钥经过Base64编码形成的一个字符串，但是注意，这不是为了保证连接时数据传输正确性而采取的措施，而是为了防止服务端错误的将WebSocket协议解析为一个HTTP协议而采取的措施
> __响应报文__
> - 正常响应行
> - 响应头部字段: 
>   1. 和请求头部字段类似，同样有对应的关键字
>   2. `Sec-WebSocket-Accept`: 在服务端接收到客户端发来的`Sec-WebSocket-Accept`密钥后，将这个值与一个固定的、全球唯一的 GUID（全局唯一标识符）字符串拼接起来。这个 GUID 是 WebSocket 协议规范中明确规定的，其值为 258EAFA5-E914-47DA-95CA-C5AB0DC85B11，对拼接后的值进行SHA-1哈希，将结果进行Base64编码，得到对应字符串

服务器通过计算Sec-WebSocket-Key的SHA-1哈希并进行Base64编码来生成Sec-WebSocket-Accept值，这确保了握手的安全性。

#### WebSocket帧格式
与HTTP不同，WebSocket使用二进制帧进行数据传输: 
```txt
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
```
主要特点包括：

- FIN位：标识这是否是消息的最后一个分片
- 操作码(Opcode)：标识帧类型（文本、二进制、控制帧等）
- 掩码(Mask)：客户端发送到服务器的所有帧必须进行掩码处理，防止缓存投毒攻击
- 有效载荷长度：指示数据长度

__WebSocket的控制帧__
WebSocket协议定义了三种控制帧：

1. 关闭帧(0x8)：用于终止连接
2. Ping帧(0x9)：检测连接活跃性
3. Pong帧(0xA)：对Ping的响应

这些控制帧确保了连接的可靠性和资源管理。

__适用场景__
WebSocket特别适合以下应用场景：

- 实时通信应用：聊天应用、在线游戏、协作工具
- 数据流推送：股票市场数据、体育比分、新闻快讯
- IoT设备监控：设备状态实时监控、传感器数据推送
- 实时分析面板：业务指标可视化、系统监控

### 关于客户端连接请求的处理（TODO）

一个服务端想要运行，必须要能够接受客户端的访问请求，所以我们基于上面介绍的Reactor + epoll多路复用机制实现客户端连接等请求处理的机制；

### 其他的优化机制（TODO）

通过上面的实现，我们就可以完成一个简易的网站搭建和请求处理业务了，但是性能实在单一，功能也残缺不全，所以就引出了我们对于上述实现的进一步优化机制，常见的有session机制，cookie机制等

