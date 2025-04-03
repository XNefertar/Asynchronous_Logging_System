#include "Logger.hpp"
#include "LogQueue.hpp"
#include <exception>
#include <random>
#include <array>


using BITMAP = const int32_t;
BITMAP BITMAP_1 = 1; // 0001
BITMAP BITMAP_2 = 2; // 0010
BITMAP BITMAP_3 = 4; // 0100
BITMAP BITMAP_4 = 8; // 1000

BITMAP getRandomBitmap() {
    // 包含所有可能值的数组
    const std::array<BITMAP, 4> bitmaps = {BITMAP_1, BITMAP_2, BITMAP_3, BITMAP_4};
    
    // 创建随机数生成器
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // 创建均匀分布
    std::uniform_int_distribution<> dist(0, bitmaps.size() - 1);
    
    // 随机选择一个值并返回
    return bitmaps[dist(gen)];
}

void TEST_FUNC(BITMAP bitmap){
    if(bitmap & BITMAP_2){
        throw std::out_of_range("bitmap & BITMAP_2");
    }
    else if(bitmap & BITMAP_3){
        throw std::length_error("bitmap & BITMAP_3");
    }
    else if(bitmap & BITMAP_4){
        throw std::invalid_argument("bitmap & BITMAP_4");
    }
    else{
        throw std::runtime_error("bitmap & BITMAP_1");
    }
}

int main(){
    // TODO: 提供多种日志选项
    // 用户可以选择普通文件(.txt)或html文件(.html)
    Logger logger("log.txt");
    // Hello C++ World
    // Hello C++ World {} {}
    // Hello C++ WorldHello World   
    while(true){
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char time_buffer[100];
        std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
        time_buffer[sizeof(time_buffer) - 1] = '\0'; // Ensure null-termination
        static int index = 0;
        try{
            TEST_FUNC(getRandomBitmap());
        }catch(const std::out_of_range& e){
            logger.log(WARNING, "Out of range exception: " + std::string(e.what()) + " at " + time_buffer);
        }catch(const std::length_error& e){
            logger.log(ERROR, "Length error exception: " + std::string(e.what()) + " at " + time_buffer);
        }catch(const std::invalid_argument& e){
            logger.log(ERROR, "Invalid argument exception: " + std::string(e.what()) + " at " + time_buffer);
        }catch(const std::runtime_error& e){
            logger.log(INFO, "Runtime error exception: " + std::string(e.what()) + " at " + time_buffer);
        }catch(...){
            logger.log(WARNING, "Unknown exception: " + std::string("Unknown exception") + " at " + time_buffer);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}