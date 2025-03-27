#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include "LogQueue.hpp"
#include "ThreadPool.hpp"

#define THREADS_SIZE 10

class Logger{
    friend class ThreadPool;
private:
    LogQueue m_queue;
    // std::thread m_thread; // TODO: use a threadpool instead of a single thread
    std::unique_ptr<ThreadPool> m_threads;
    std::ofstream m_file;

public:
    Logger(const std::string& filename)
        :m_threads(new ThreadPool(THREADS_SIZE))
    {
        m_file.open(filename, std::ios::out | std::ios::app);
        if(!m_file.is_open()){
            throw std::runtime_error("Failed to open log file");
        }
    }

    ~Logger(){
        m_queue.stop();
        if(m_file.is_open()){
            m_file.flush();
            m_file.close();
        }
    }

    template <typename ...Args>
    void log(const std::string& format, Args ...args){
        m_queue.push(process_string(format, args...));
        m_threads->submitTask([this](){
            process();
        });
    }


private:
    void process(){
        std::string msg;
        while(m_queue.pop(msg)){
            m_file << msg << std::endl;
            // m_file.flush();
        }
    }

    template <typename ...Args>
    std::string process_string(const std::string& format, Args ...args){
        std::vector<std::string> args_str = {to_string(args)...};
        std::ostringstream os;
        os.clear();
        int args_index = 0, last_position = 0;
        int placeholder_index = format.find("{}");
        // "Hello {} {}", C++, World
        // "Hello {} {} {} {}", "C++", "World"
        while(placeholder_index != std::string::npos){
            if(args_index >= args_str.size()){
                break;
            }
            os << format.substr(last_position, placeholder_index - last_position);
            os << args_str[args_index++];
            last_position = placeholder_index + 2;
            placeholder_index = format.find("{}", last_position);
        }

        if(!format.empty() && args_index >= args_str.size()){
            os << format.substr(last_position);
        }
        // os << format.substr(last_position);
        while(args_index < args_str.size()){
            os << args_str[args_index++];
        }

        return os.str();
    }

};


#endif // __LOGGER_HPP__