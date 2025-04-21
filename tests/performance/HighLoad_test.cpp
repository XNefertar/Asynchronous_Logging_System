#include <gtest/gtest.h>
#include <benchmark/benchmark.h>
#include "../../EpollServer/EpollServer.hpp"
#include "../../WebSocket/WebSocket.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace EpollServerSpace;

// 高负载测试标记
static void BM_ServerHighLoad(benchmark::State& state) {
    // 测试参数
    int numRequests = state.range(0);
    
    // 创建服务器
    EpollServer server(8097, "test_user", "test_password", "test_db", "./test_log.txt", "./static");
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
        std::vector<int> sockets(numRequests);
        for (int i = 0; i < numRequests; i++) {
            sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8097);
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            connect(sockets[i], (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        }
        
        // 创建HTTP请求
        std::string request = 
            "GET /index.html HTTP/1.1\r\n"
            "Host: localhost:8097\r\n"
            "Connection: close\r\n"
            "\r\n";
        
        state.ResumeTiming(); // 恢复计时器
        
        // 同时发送所有请求
        for (int i = 0; i < numRequests; i++) {
            send(sockets[i], request.c_str(), request.size(), 0);
        }
        
        // 接收所有响应
        for (int i = 0; i < numRequests; i++) {
            char buffer[4096] = {0};
            recv(sockets[i], buffer, sizeof(buffer) - 1, 0);
        }
        
        state.PauseTiming(); // 暂停计时器
        
        // 关闭所有连接
        for (int i = 0; i < numRequests; i++) {
            close(sockets[i]);
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
BENCHMARK(BM_ServerHighLoad)
    ->Args({10})
    ->Args({50})
    ->Args({100})
    ->Unit(benchmark::kMillisecond);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    
    return 0;
}