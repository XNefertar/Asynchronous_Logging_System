#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include "LogQueue.hpp"

class Logger{
private:
    LogQueue m_queue;
    std::thread m_thread; // TODO: use a threadpool instead of a single thread
    std::ofstream m_file;
    // bool m_exit = false;

public:
    Logger(const std::string& filename){
        m_file.open(filename, std::ios::out | std::ios::app);
        if(!m_file.is_open()){
            throw std::runtime_error("Failed to open log file");
        }
        m_thread = std::thread([this](){
            process();
        });
    }

    ~Logger(){
        m_queue.stop();
        if(m_thread.joinable()){
            m_thread.join();
        }
        if(m_file.is_open()){
            m_file.close();
        }
    }

    template <typename ...Args>
    void log(const std::string& format, Args ...args){
        m_queue.push(process_string(format, args...));
    }


private:
    void process(){
        std::string msg;
        while(m_queue.pop(msg)){
            m_file << msg << std::endl;
        }
    }

    template <typename ...Args>
    std::string process_string(const std::string& format, Args ...args){
        std::vector<std::string> args_str = {to_string(args)...};
        std::ostringstream os;
        int args_index = 0, last_position = 0;
        int placeholder_index = format.find("{}");
        // "Hello {} {}", C++, World
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
        os << format.substr(last_position);
        while(args_index < args_str.size()){
            os << args_str[args_index++];
        }

        return os.str();
    }

};


#endif // __LOGGER_HPP__