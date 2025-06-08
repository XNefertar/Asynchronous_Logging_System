#include "Logger.hpp"
#include "LogQueue.hpp"
#include "Util/SessionManager.hpp"
#include "Util/LogTemplates.hpp"
#include "Util/SessionManager.hpp"
#include "Util/ConfigManager.hpp"
#include "Util/SharedConfigManager.hpp"
#include <exception>
#include <random>
#include <array>
#include <string>
#include <map>


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
    // int option = 0;
    // std::cout << "请选择日志选项: " << std::endl;
    // std::cout << "0. 普通文件(.txt)" << std::endl;
    // std::cout << "1. HTML文件(.html)" << std::endl;
    // std::cin >> option;

    // std::ofstream file("/tmp/option.tmp");
    // if (file) {
    //     file << option;
    //     // 记得要 flush 刷新缓冲区
    //     // 这样才能确保数据写入文件
    //     file.flush();
    //     std::rename("/tmp/option.tmp", "/tmp/option.flag"); // 原子操作
    //     std::cout << "选项已保存到 /tmp/option.flag" << std::endl;
    //     std::cout << "option = " << option << std::endl;
    // }
    // ConfigSpace::ConfigManager::getInstance()->initFromUserInput();
    // std::cout << "ConfigManager initialized with format: " 
    //           << ConfigSpace::ConfigManager::getInstance()->getFormatString() << std::endl;
    // 初始化日志系统
    // if(ConfigSpace::ConfigManager::getInstance()->isInitialized()){

    ConfigSpace::SharedConfigManager::getInstance().initFromUserInput();

    if(ConfigSpace::SharedConfigManager::getInstance().isInitialized()){
        if(ConfigSpace::ConfigManager::getInstance()->isTextFormat()){
            Logger logger("log.txt", false);
            while(true){
                auto now = std::chrono::system_clock::now();
                std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                char time_buffer[100];
                std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
                time_buffer[sizeof(time_buffer) - 1] = '\0'; // Ensure null-termination
                
                try{
                    TEST_FUNC(getRandomBitmap());
                }catch(const std::out_of_range& e){
                    // 获取一个会话信息
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("database");
                    std::map<std::string, std::string> values = {
                        {"table", "bitmap_table"},
                        {"condition", "id = " + std::to_string(BITMAP_2)},
                        {"time", "50"},
                        {"id", std::to_string(static_cast<int>(session.connect_time))},
                        {"txId", std::to_string(static_cast<int>(session.connect_time + session.port))},
                        {"isolation", "READ_COMMITTED"},
                        {"rows", std::to_string(session.total_bytes > 0 ? session.total_bytes : 1)},
                        {"active", "5"}, {"idle", "10"}, {"waiting", "2"}, {"fields", "value,flag"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    // logger.log_in_text(WARNING, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                    logger.log_in_text(WARNING, logMsg + " - Exception: " + " at " + time_buffer);

                }catch(const std::length_error& e){
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("network");
                    std::map<std::string, std::string> values = {
                        {"endpoint", "/api/bitmap"},
                        {"method", "GET"},
                        {"size", std::to_string(sizeof(BITMAP))},
                        {"client", session.ip},
                        {"source", session.ip + ":" + std::to_string(session.port)},
                        {"status", "413"},
                        {"time", "120"},
                        {"protocol", "TCP"},
                        {"connId", std::to_string(static_cast<int>(session.connect_time))},
                        {"type", "binary"},
                        {"reason", "size_exceeded"},
                        {"duration", std::to_string(time(nullptr) - session.connect_time)}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_text(ERROR, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                }catch(const std::invalid_argument& e){
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("auth");
                    std::map<std::string, std::string> values = {
                        {"user", "system_user"},
                        {"ip", session.ip},
                        {"resource", "bitmap_flags"},
                        {"action", "modify"},
                        {"userId", std::to_string(static_cast<int>(session.connect_time))},
                        {"duration", std::to_string(time(nullptr) - session.connect_time)},
                        {"permission", "admin"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_text(ERROR, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                }catch(const std::runtime_error& e){
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("system");
                    std::map<std::string, std::string> values = {
                        {"cpu", "15"},
                        {"memory", "256"},
                        {"io", "500"},
                        {"file", "bitmap_config.json"},
                        {"items", "10"},
                        {"task", "process_bitmap"},
                        {"status", "failed"},
                        {"name", "BitmapProcessor"},
                        {"dependencies", "LogSystem,FileSystem"},
                        {"oldState", "running"},
                        {"newState", "error"},
                        {"trigger", "runtime_error"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_text(INFO, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                }catch(...){
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("system");
                    std::map<std::string, std::string> values = {
                        {"oldState", "running"},
                        {"newState", "error"},
                        {"trigger", "unknown_exception"},
                        {"name", "BitmapProcessor"},
                        {"status", "crashed"},
                        {"dependencies", "unknown"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_text(WARNING, logMsg + " - Unknown exception at " + time_buffer);
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        else
        {
            Logger logger("log.html", true);
            while(true){
                auto now = std::chrono::system_clock::now();
                std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                char time_buffer[100];
                std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
                time_buffer[sizeof(time_buffer) - 1] = '\0'; // Ensure null-termination
                
                try{
                    TEST_FUNC(getRandomBitmap());
                }catch(const std::out_of_range& e){
                    // HTML日志使用相同的模板系统
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("database");
                    std::map<std::string, std::string> values = {
                        {"table", "bitmap_table"},
                        {"condition", "id = " + std::to_string(BITMAP_2)},
                        {"time", "50"},
                        {"id", std::to_string(static_cast<int>(session.connect_time))},
                        {"txId", std::to_string(static_cast<int>(session.connect_time + session.port))},
                        {"isolation", "READ_COMMITTED"},
                        {"rows", std::to_string(session.total_bytes > 0 ? session.total_bytes : 1)},
                        {"active", "5"}, {"idle", "10"}, {"waiting", "2"}, {"fields", "value,flag"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_html(WARNING, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                }catch(const std::length_error& e){
                    // 类似的处理其他异常...
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("network");
                    std::map<std::string, std::string> values = {
                        {"endpoint", "/api/bitmap"},
                        {"method", "GET"},
                        {"size", std::to_string(sizeof(BITMAP))},
                        {"client", session.ip},
                        {"source", session.ip + ":" + std::to_string(session.port)},
                        {"status", "413"},
                        {"time", "120"},
                        {"protocol", "TCP"},
                        {"connId", std::to_string(static_cast<int>(session.connect_time))},
                        {"type", "binary"},
                        {"reason", "size_exceeded"},
                        {"duration", std::to_string(time(nullptr) - session.connect_time)}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_html(ERROR, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                }catch(const std::invalid_argument& e){
                    // 类似处理...
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("auth");
                    std::map<std::string, std::string> values = {
                        {"user", "system_user"},
                        {"ip", session.ip},
                        {"resource", "bitmap_flags"},
                        {"action", "modify"},
                        {"userId", std::to_string(static_cast<int>(session.connect_time))},
                        {"duration", std::to_string(time(nullptr) - session.connect_time)},
                        {"permission", "admin"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_html(ERROR, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                }catch(const std::runtime_error& e){
                    // 类似处理...
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("system");
                    std::map<std::string, std::string> values = {
                        {"cpu", "15"},
                        {"memory", "256"},
                        {"io", "500"},
                        {"file", "bitmap_config.json"},
                        {"items", "10"},
                        {"task", "process_bitmap"},
                        {"status", "failed"},
                        {"name", "BitmapProcessor"},
                        {"dependencies", "LogSystem,FileSystem"},
                        {"oldState", "running"},
                        {"newState", "error"},
                        {"trigger", "runtime_error"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_html(INFO, logMsg + " - Exception: " + std::string(e.what()) + " at " + time_buffer);
                }catch(...){
                    ClientSessionInfo session = SessionManager::getInstance()->getRandomSession();
                    
                    std::string tmpl = LogTemplates::getRandomTemplate("system");
                    std::map<std::string, std::string> values = {
                        {"oldState", "running"},
                        {"newState", "error"},
                        {"trigger", "unknown_exception"},
                        {"name", "BitmapProcessor"},
                        {"status", "crashed"},
                        {"dependencies", "unknown"}
                    };
                    
                    std::string logMsg = LogTemplates::replacePlaceholders(tmpl, values);
                    logger.log_in_html(WARNING, logMsg + " - Unknown exception at " + time_buffer);
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}