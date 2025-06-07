#ifndef __LOGQUEUE_HPP__
#define __LOGQUEUE_HPP__

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <exception>
#include <condition_variable>

// 辅助函数，将单个参数转换为字符串
template <typename T>
std::string to_string(T value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

class LogQueue{
private:
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_bStop = false;

public:
    void push(const std::string& msg){
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(msg);
        m_cv.notify_one();
    }

    bool pop(std::string& outmsg){
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this](){
            return !m_queue.empty() || m_bStop;
        });

        if(m_bStop && m_queue.empty()){
            return false;
        }

        // while(!m_queue.empty() && m_bStop){
        //     std::cout << m_queue.front() << std::endl;
        //     m_queue.pop();
        // }

        outmsg = m_queue.front();
        m_queue.pop();
        return true;
    }

    void stop(){
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bStop = true;
        m_cv.notify_all();
    }
};

#endif // __LOGQUEUE_HPP__