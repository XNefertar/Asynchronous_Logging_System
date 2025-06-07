// test_log_latency.cpp
#include <iostream>
#include <chrono>
#include <algorithm>
#include "AsyncDBWriter.hpp"
using namespace AsyncDBWriterSpace;

// 测量单条日志从提交到完成的延迟
void test_latency(bool async_mode, int iterations) {
    std::vector<long long> latencies;
    
    for (int i = 0; i < iterations; i++) {
        DBWriteTask task("INFO", "192.168.1.1", 8080, 
                               "Latency test message " + std::to_string(i));
        
        auto start = std::chrono::high_resolution_clock::now();
        
        if (async_mode) {
            // 由于异步，我们需要特别标记以知道何时完成
            // 这里使用一个特殊的回调机制或共享状态来检测
            // 这里简化为直接测量提交时间
            AsyncDBWriter::getInstance().addTask(task);
        } else {
            SyncDBWriter::writeLog(task);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        latencies.push_back(duration);
        
        // 添加一些间隔，避免连续测试
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 计算平均、最小、最大、P99延迟
    long long sum = 0;
    long long min_latency = latencies[0];
    long long max_latency = latencies[0];
    
    for (auto latency : latencies) {
        sum += latency;
        min_latency = std::min(min_latency, latency);
        max_latency = std::max(max_latency, latency);
    }
    
    std::sort(latencies.begin(), latencies.end());
    long long p99 = latencies[static_cast<size_t>(iterations * 0.99)];
    
    std::cout << "平均延迟: " << (sum / iterations) << " us" << std::endl;
    std::cout << "最小延迟: " << min_latency << " us" << std::endl;
    std::cout << "最大延迟: " << max_latency << " us" << std::endl;
    std::cout << "P99延迟: " << p99 << " us" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [async|sync] <iterations>" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    int iterations = std::stoi(argv[2]);
    
    if (mode == "async") {
        AsyncDBWriter::getInstance().start(4);
        test_latency(true, iterations);
        AsyncDBWriter::getInstance().shutdown();
    } else {
        test_latency(false, iterations);
    }
    
    return 0;
}