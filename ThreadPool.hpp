#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <condition_variable>

class Logger;
using Task = std::function<void()>;

// template<typename T>
class ThreadPool {
    //std::function<void(Logger*)> f = &Logger::process;
public:
    ThreadPool(int thread_count = 4)
        :_thread_count(thread_count)
        
    {
        _threads.reserve(thread_count * 2);
        for(int i = 0; i < thread_count; ++i){
            _threads.emplace_back(std::thread([this, i](){
                while(true){
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        _cond.wait(lock, [this](){ return _isClosed || !_tasks.empty(); });
                        if(_isClosed && _tasks.empty()) return;
                        task = std::move(_tasks.front());
                        _tasks.pop();
						// std::cout << "Thread " << i << " is processing" << std::endl;
                    }
                    task();
                }
            }));
            std::cout << "Thread " << i << " created" << std::endl;
        }
    }

    ~ThreadPool(){
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _isClosed = true;
        }
        _cond.notify_all();
        for(auto& it : _threads){
            if(it.joinable()){
                it.join();
            }
        }
    }

    template<typename F>
    bool submitTask(F&& f){
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(_isClosed) return false;
            _tasks.push(std::forward<F>(f));
        }
        _cond.notify_one();
        return true;
    }

private:
    std::vector<std::thread> _threads;
    std::queue<Task> _tasks;
    int _thread_count;
    std::mutex _mutex;
    std::condition_variable _cond;
    bool _isClosed = false;

};

#endif  // __THREADPOOL_HPP__