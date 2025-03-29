#include "Logger.hpp"
#include "LogQueue.hpp"


int main(){
    Logger logger("log.txt");
    // Hello C++ World
    // Hello C++ World {} {}
    // Hello C++ WorldHello World   
    while(true){
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char time_buffer[100];
        std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
        logger.log("[%s] ", time_buffer);
        static int index = 0;
        logger.log("The %dth log message", index++);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(index >= 100000){
            std::ofstream ofs("log.txt", std::ofstream::out | std::ofstream::trunc);
            ofs.close();
        }
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}