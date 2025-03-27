#include "Logger.hpp"
#include "LogQueue.hpp"


int main(){
    Logger logger("log.txt");
    // Hello C++ World
    // Hello C++ World {} {}
    // Hello C++ WorldHello World
    logger.log("Hello {} {}", "C++", "World");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    logger.log("Hello {} {} {} {}", "C++", "World");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    logger.log("Hello {} {} ", "C++", "World", "Hello ", "World");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}