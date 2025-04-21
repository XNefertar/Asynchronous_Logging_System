#include <gtest/gtest.h>
#include "../../WebSocket/WebSocket.hpp"
#include "../../WebSocket/WebSocketApiHandlers.hpp"
#include "../../EpollServer/EpollServer.hpp"
#include <thread>
#include <future>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <vector>

using namespace EpollServerSpace;

// WebSocket客户端模拟类
class WebSocketClientSimulator {
public:
    WebSocketClientSimulator(const std::string& host, int port) : host_(host), port_(port), sockfd_(-1) {}
    
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
        std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // 固定key，仅用于测试
        
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
        
        // 验证响应是否包含WebSocket握手成功的标志
        std::string response(buffer);
        return response.find("101 Switching Protocols") != std::string::npos &&
               response.find("Upgrade: websocket") != std::string::npos;
    }
    
    bool sendWebSocketFrame(const std::string& message) {
        // 创建一个简单的WebSocket帧
        std::vector<char> frame;
        
        // 第一个字节: FIN=1, RSV1-3=0, Opcode=1 (text frame)
        frame.push_back(0x81);
        
        // 计算负载长度
        size_t length = message.size();
        
        // 第二个字节: MASK=1, Payload len=?
        if (length < 126) {
            frame.push_back(static_cast<char>(0x80 | length));
        } else if (length < 65536) {
            frame.push_back(0xFE); // 126 | 0x80
            frame.push_back(static_cast<char>((length >> 8) & 0xFF));
            frame.push_back(static_cast<char>(length & 0xFF));
        } else {
            frame.push_back(0xFF); // 127 | 0x80
            for (int i = 7; i >= 0; i--) {
                frame.push_back(static_cast<char>((length >> (i * 8)) & 0xFF));
            }
        }
        
        // 掩码 (4 bytes)
        unsigned char mask[4] = {0x12, 0x34, 0x56, 0x78};
        for (int i = 0; i < 4; i++) {
            frame.push_back(mask[i]);
        }
        
        // 加密载荷
        for (size_t i = 0; i < message.size(); i++) {
            frame.push_back(message[i] ^ mask[i % 4]);
        }
        
        // 发送帧
        return send(sockfd_, frame.data(), frame.size(), 0) > 0;
    }
    
    std::string receiveWebSocketFrame() {
        char buffer[4096] = {0};
        int bytesReceived = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) return "";
        
        // 简化的解码逻辑 (仅用于测试)
        if (bytesReceived < 2) return "";
        
        uint8_t secondByte = static_cast<uint8_t>(buffer[1]);
        uint8_t payloadLength = secondByte & 0x7F;
        
        size_t headerSize = 2;
        if (payloadLength == 126) {
            headerSize += 2;
        } else if (payloadLength == 127) {
            headerSize += 8;
        }
        
        // 服务器不会掩码数据
        std::string payload(buffer + headerSize, bytesReceived - headerSize);
        return payload;
    }
    
    void disconnect() {
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
    }
    
    ~WebSocketClientSimulator() {
        disconnect();
    }
    
private:
    std::string host_;
    int port_;
    int sockfd_;
};

class WebSocketCommTest : public ::testing::Test {
protected:
    EpollServer* server;
    std::thread serverThread;
    
    void SetUp() override {
        // 使用测试专用端口和配置
        server = new EpollServer(8095, "test_user", "test_password", "test_db", "./test_log.txt", "./static");
        
        // 设置WebSocket处理器
        server->setWebSocketHandler("/ws", [&](int sockfd, const std::string& message, ClientSession& session) {
            // 简单的回显处理器
            std::string response = "Echo: " + message;
            server->sendWebSocketMessage(sockfd, response);
        });
        
        // 初始化服务器
        server->ServerInit();
        
        // 在单独的线程中启动服务器
        serverThread = std::thread([this]() {
            server->ServerStart();
        });
        
        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    void TearDown() override {
        // 停止服务器
        server->ServerStop();
        
        // 等待服务器线程结束
        if (serverThread.joinable()) {
            serverThread.join();
        }
        
        delete server;
    }
};

// 测试WebSocket握手
TEST_F(WebSocketCommTest, WebSocketHandshake) {
    WebSocketClientSimulator client("127.0.0.1", 8095);
    
    EXPECT_TRUE(client.connect());
    EXPECT_TRUE(client.performWebSocketHandshake());
    
    client.disconnect();
}

// 测试WebSocket消息发送和接收
TEST_F(WebSocketCommTest, WebSocketMessageExchange) {
    WebSocketClientSimulator client("127.0.0.1", 8095);
    
    EXPECT_TRUE(client.connect());
    EXPECT_TRUE(client.performWebSocketHandshake());
    
    // 发送消息
    std::string testMessage = "Hello WebSocket";
    EXPECT_TRUE(client.sendWebSocketFrame(testMessage));
    
    // 接收响应
    std::string response = client.receiveWebSocketFrame();
    EXPECT_EQ("Echo: Hello WebSocket", response);
    
    client.disconnect();
}

// 更多WebSocket通信测试...

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}