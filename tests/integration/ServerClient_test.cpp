#include <gtest/gtest.h>
#include "../../EpollServer/EpollServer.hpp"
#include "../../Client/Client.hpp"
#include <thread>
#include <future>

using namespace EpollServerSpace;

class ServerClientIntegrationTest : public ::testing::Test {
protected:
    EpollServer* server;
    std::thread serverThread;
    
    void SetUp() override {
        // 使用测试专用端口和配置
        server = new EpollServer(8090, "test_user", "test_password", "test_db", "./test_log.txt", "./static");
        
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

// 测试HTTP GET请求
TEST_F(ServerClientIntegrationTest, HttpGetRequest) {
    // 创建一个基本HTTP GET请求
    std::string request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8090\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    // 使用socket直接发送请求
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(sockfd, 0);
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8090);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    int result = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    ASSERT_EQ(0, result);
    
    result = send(sockfd, request.c_str(), request.size(), 0);
    ASSERT_GT(result, 0);
    
    // 接收响应
    char buffer[4096] = {0};
    result = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    ASSERT_GT(result, 0);
    
    // 验证响应是HTTP响应
    std::string response(buffer);
    EXPECT_TRUE(response.find("HTTP/1.1") != std::string::npos);
    
    close(sockfd);
}

// 测试客户端和服务器的连接
TEST_F(ServerClientIntegrationTest, ClientServerConnection) {
    ClientTCP client("localhost", 8090);
    
    // 模拟客户端连接和发送消息
    bool connected = client.Connect();
    EXPECT_TRUE(connected);
    
    // 发送一个测试日志消息
    std::string testMessage = "[INFO]{Test integration message}";
    ssize_t bytesSent = client.sendData(testMessage);
    EXPECT_GT(bytesSent, 0);
    
    // 接收服务器响应
    std::string response = client.receiveData();
    EXPECT_FALSE(response.empty());
    
    // 验证响应中包含成功状态
    EXPECT_TRUE(response.find("success") != std::string::npos || 
                response.find("200") != std::string::npos);
    
    client.disConnect();
}

// 更多集成测试...

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}