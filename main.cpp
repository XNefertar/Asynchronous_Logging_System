#include "Logger.hpp"
#include "LogQueue.hpp"


int main(){
    Logger logger("log.txt");
    logger.log("Hello {} {}", "C++", "World");
    logger.log("Hello {} {} {} {}", "C++", "World");
    logger.log("Hello {} {} ", "C++", "World", "Hello ", "World");

    return 0;
}