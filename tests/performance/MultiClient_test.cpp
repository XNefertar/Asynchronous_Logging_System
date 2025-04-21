#include <gtest/gtest.h>
#include <benchmark/benchmark.h>
#include "../../EpollServer/EpollServer.hpp"
#include "../../WebSocket/WebSocket.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>

using namespace EpollServerSpace;

// WebSocket客户端模拟
class SimpleWebSocketClient {
public:
    SimpleWebSocketClient(const std::string& host, int port) 
        : host_(host), port_(port), sockfd_(-1), handshakeCompleted_(false) {}
    
    bool connect() {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) return false;
        
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port_);
        serverAddr.sin_addr.s_addr = inet_addr(host_.c_str());
        
        int result = ::connect(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        return result == 0;
    }
    
    bool performWebSocketHandshake() {
        // 生成随机key
        std::string key = generateRandomKey();
        
        // 创建握手请求
        std::string request = 
            "GET /ws HTTP/1.1\r\n"
            "Host: " + host_ + ":" + std::to_string(port_) + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: " + key + "\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
        
        // 发送握手请求
        if (send(sockfd_, request.c_str(), request.size(), 0) <= 0) return false;
        
        // 接收响应
        char buffer[1024] = {0};
        if (recv(sockfd_, buffer, sizeof(buffer) - 1, 0) <= 0) return false;
        
        // 检查握手是否成功
        std::string response(buffer);
        handshakeCompleted_ = response.find("101 Switching Protocols") != std::string::npos;
        return handshakeCompleted_;
    }
    
    bool sendMessage(const std::string& message) {
        if (!handshakeCompleted_) return false;
        
        // 创建简化的WebSocket帧
        std::vector<char> frame;
        frame.push_back(0x81); // FIN=1, Opcode=1 (text)
        
        if (message.size() < 126) {
            frame.push_back(static_cast<char>(message.size()));
        } else if (message.size() < 65536) {
            frame.push_back(126);
            frame.push_back(static_cast<char>((message.size() >> 8) & 0xFF));
            frame.push_back(static_cast<char>(message.size() & 0xFF));
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; i--) {
                frame.push_back(static_cast<char>((message.size() >> (i * 8)) & 0xFF));
            }
        }
        
        // 添加消息内容
        frame.insert(frame.end(), message.begin(), message.end());
        
        return send(sockfd_, frame.data(), frame.size(), 0) > 0;
    }
    
    std::string receiveMessage() {
        if (!handshakeCompleted_) return "";
        
        char buffer[4096] = {0};
        int bytesReceived = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) return "";
        
        // 简化的解码
        uint8_t firstByte = static_cast<uint8_t>(buffer[0]);
        uint8_t secondByte = static_cast<uint8_t>(buffer[1]);
        bool fin = (firstByte & 0x80) != 0;
        uint8_t opcode = firstByte & 0x0F;
        bool masked = (secondByte & 0x80) != 0;
        uint64_t payloadLength = secondByte & 0x7F;
        
        size_t headerOffset = 2;
        if (payloadLength == 126) {
            payloadLength = (static_cast<uint16_t>(static_cast<uint8_t>(buffer[2])) << 8) |
                             static_cast<uint8_t>(buffer[3]);
            headerOffset = 4;
        } else if (payloadLength == 127) {
            payloadLength = 0;
            for (int i = 0; i < 8; i++) {
                payloadLength = (payloadLength << 8) | static_cast<uint8_t>(buffer[2 + i]);
            }
            headerOffset = 10;
        }
        
        if (masked) {
            headerOffset += 4; // 掩码占4字节
        }
        
        if (opcode == 0x8) { // 关闭帧
            return "";
        }
        
        return std::string(buffer + headerOffset, payloadLength);
    }
    
    void disconnect() {
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
        handshakeCompleted_ = false;
    }
    
    ~SimpleWebSocketClient() {
        disconnect();
    }
    
private:
    std::string generateRandomKey() {
        std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // 固定值用于测试
        return key;
    }
    
    std::string host_;
    int port_;
    int sockfd_;
    bool handshakeCompleted_;
};

// 多WebSocket客户端测试
static void BM_WebSocketMultiClient(benchmark::State& state) {
    // 测试参数
    int numClients = state.range(0);
    
    // 创建服务器
    EpollServer server(8098, "test_user", "test_password", "test_db", "./test_log.txt", "./static");
    
    // 设置WebSocket处理器
    server.setWebSocketHandler("/ws", [&](int sockfd, const std::string& message, ClientSession& session) {
        // 简单的回显处理器
        std::string response = "Echo: " + message;
        server.sendWebSocketMessage(sockfd, response);
    });
    
    server.ServerInit();
    
    // 在单独线程中启动服务器
    std::thread serverThread([&server]() {
        server.ServerStart();
    });
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    for (auto _ : state) {
        state.PauseTiming(); // 暂停计时器
        
        // 创建客户端
        std::vector<SimpleWebSocketClient> clients;
        for (int i = 0; i < numClients; i++) {
            clients.emplace_back("127.0.0.1", 8098);
            clients.back().connect();
            clients.back().performWebSocketHandshake();
        }
        
        state.ResumeTiming(); // 恢复计时器
        
        // 每个客户端发送一条消息
        for (int i = 0; i < numClients; i++) {
            std::string message = "Hello from client " + std::to_string(i);
            clients[i].sendMessage(message);
        }
        
        // 接收所有响应
        for (int i = 0; i < numClients; i++) {
            std::string response = clients[i].receiveMessage();
            // 验证响应是否符合预期
            if (response.find("Echo: Hello from client") == std::string::npos) {
                state.SkipWithError("Invalid response received");
                break;
            }
        }
        
        state.PauseTiming(); // 暂停计时器
        
        // 关闭所有连接
        for (auto& client : clients) {
            client.disconnect();
        }
        
        state.ResumeTiming(); // 恢复计时器
    }
    
    // 停止服务器
    server.ServerStop();
    
    // 等待服务器线程结束
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

// 注册基准测试
BENCHMARK(BM_WebSocketMultiClient)
    ->Args({5})
    ->Args({20})
    ->Args({50})
    ->Unit(benchmark::kMillisecond);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    
    return 0;
}