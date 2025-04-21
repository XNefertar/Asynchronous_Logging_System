#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../../EpollServer/EpollServer.hpp"

using namespace EpollServerSpace;

// 模拟客户端会话
class MockClientSession : public ClientSession {
public:
    MockClientSession() : ClientSession() {}
};

class EpollServerTest : public ::testing::Test {
protected:
    EpollServer* server;
    
    void SetUp() override {
        // 使用测试专用的端口和配置
        server = new EpollServer(8089, "test_user", "test_password", "test_db", "./test_log.txt", "test_dir");
    }
    
    void TearDown() override {
        delete server;
    }
};

// 测试HTTP请求解析
TEST_F(EpollServerTest, ParseHttpRequest) {
    std::string requestStr = 
        "GET /api/logs?limit=10 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "\r\n";
    
    HttpRequest request;
    bool result = server->parseHttpRequest(requestStr, request);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(HttpMethod::GET, request.method);
    EXPECT_EQ("/api/logs", request.path);
    EXPECT_EQ("HTTP/1.1", request.version);
    EXPECT_EQ("10", request.queryParams["limit"]);
    EXPECT_EQ("localhost:8080", request.headers["Host"]);
    EXPECT_EQ("keep-alive", request.headers["Connection"]);
    EXPECT_EQ("Mozilla/5.0", request.headers["User-Agent"]);
}

// 测试HTTP响应序列化
TEST_F(EpollServerTest, SerializeHttpResponse) {
    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    response.headers["Connection"] = "keep-alive";
    response.body = "{\"status\":\"success\"}";
    
    std::string responseStr = server->serializeHttpResponse(response);
    
    // 验证响应格式
    EXPECT_TRUE(responseStr.find("HTTP/1.1 200 OK\r\n") != std::string::npos);
    EXPECT_TRUE(responseStr.find("Content-Type: application/json\r\n") != std::string::npos);
    EXPECT_TRUE(responseStr.find("Connection: keep-alive\r\n") != std::string::npos);
    EXPECT_TRUE(responseStr.find("\r\n\r\n{\"status\":\"success\"}") != std::string::npos);
}

// 测试MIME类型判断
TEST_F(EpollServerTest, GetMimeType) {
    EXPECT_EQ("text/html", server->getMimeType("index.html"));
    EXPECT_EQ("text/css", server->getMimeType("styles.css"));
    EXPECT_EQ("application/javascript", server->getMimeType("script.js"));
    EXPECT_EQ("application/json", server->getMimeType("data.json"));
    EXPECT_EQ("image/png", server->getMimeType("image.png"));
    EXPECT_EQ("application/octet-stream", server->getMimeType("unknown.xyz"));
}

// 测试WebSocket帧创建
TEST_F(EpollServerTest, CreateWebSocketFrame) {
    std::string message = "Hello WebSocket";
    std::vector<char> frame = server->createWebSocketFrame(message);
    
    // 验证帧格式
    EXPECT_GT(frame.size(), message.size());
    EXPECT_EQ(0x81, static_cast<uint8_t>(frame[0])); // 文本帧 + FIN位
    
    // 负载长度测试逻辑
    uint8_t secondByte = static_cast<uint8_t>(frame[1]);
    size_t payloadLength = secondByte & 0x7F;
    
    if (payloadLength < 126) {
        EXPECT_EQ(message.size(), payloadLength);
    } else if (payloadLength == 126) {
        uint16_t length = (static_cast<uint16_t>(static_cast<uint8_t>(frame[2])) << 8) | 
                         static_cast<uint8_t>(frame[3]);
        EXPECT_EQ(message.size(), length);
    }
}

// 更多EpollServer的测试...

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}