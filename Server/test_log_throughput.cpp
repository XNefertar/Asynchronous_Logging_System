// test_log_throughput.cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include "AsyncDBWriter.hpp"

using namespace AsyncDBWriterSpace;
std::atomic<int> completed(0);

void async_writer_test(int log_count) {
    for (int i = 0; i < log_count; i++) {
        DBWriteTask task("INFO", "192.168.1.1", 8080, 
                                "Test log message " + std::to_string(i));
        AsyncDBWriter::getInstance().addTask(task);
    }
    completed++;
}

void sync_writer_test(int log_count) {
    for (int i = 0; i < log_count; i++) {
        DBWriteTask task("INFO", "192.168.1.1", 8080, 
                               "Test log message " + std::to_string(i));
        SyncDBWriter::writeLog(task);
    }
    completed++;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " [async|sync] <threads> <logs_per_thread>" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    int thread_count = std::stoi(argv[2]);
    int logs_per_thread = std::stoi(argv[3]);
    
    std::vector<std::thread> threads;
    
    if (mode == "async") {
        AsyncDBWriter::getInstance().start(40); // 启动4个工作线程
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 创建多个生产者线程
        for (int i = 0; i < thread_count; i++) {
            threads.emplace_back(async_writer_test, logs_per_thread);
        }
        
        // 等待所有线程完成
        for (auto& t : threads) {
            t.join();
        }
        
        // 确保所有日志都被处理完
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 通过检查队列大小判断是否处理完成
            if (AsyncDBWriter::getInstance().getQueueSize() == 0) {
                break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "异步模式: 处理 " << thread_count * logs_per_thread << " 条日志用时 " 
                  << duration << " ms" << std::endl;
        std::cout << "吞吐量: " << (thread_count * logs_per_thread * 1000.0 / duration) 
                  << " 条/秒" << std::endl;
                  
        AsyncDBWriter::getInstance().shutdown();
    }
    else if (mode == "sync") {
        auto start = std::chrono::high_resolution_clock::now();
        
        // 创建多个线程同步写入
        for (int i = 0; i < thread_count; i++) {
            threads.emplace_back(sync_writer_test, logs_per_thread);
        }
        
        // 等待所有线程完成
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "同步模式: 处理 " << thread_count * logs_per_thread << " 条日志用时 " 
                  << duration << " ms" << std::endl;
        std::cout << "吞吐量: " << (thread_count * logs_per_thread * 1000.0 / duration) 
                  << " 条/秒" << std::endl;
    }
    
    return 0;
}