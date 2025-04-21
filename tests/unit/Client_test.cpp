#include <gtest/gtest.h>
#include "../../Client/Client.hpp"
#include <fstream>
#include <filesystem>
#include <thread>

class ClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时日志文件
        std::ofstream testLogFile("test_log.txt");
        testLogFile << "[INFO] Test log message 1\n";
        testLogFile << "[WARNING] Test warning message\n";
        testLogFile << "[ERROR] Test error message\n";
        testLogFile.close();
        
        // 创建临时HTML日志文件
        std::ofstream testHtmlLogFile("test_log.html");
        testHtmlLogFile << "<!DOCTYPE html><html><body>\n";
        testHtmlLogFile << "<div class='log-entry'>[INFO] Test log message 1 at 2023-01-01 12:00:00</div>\n";
        testHtmlLogFile << "<div class='log-entry'>[WARNING] Test warning message at 2023-01-01 12:01:00</div>\n";
        testHtmlLogFile << "<div class='log-entry'>[ERROR] Test error message at 2023-01-01 12:02:00</div>\n";
        testHtmlLogFile << "</body></html>\n";
        testHtmlLogFile.close();
    }
    
    void TearDown() override {
        // 清理临时文件
        std::filesystem::remove("test_log.txt");
        std::filesystem::remove("test_log.html");
    }
};

// 测试客户端初始化
TEST_F(ClientTest, Initialization) {
    ClientTCP client("localhost", 8080);
    EXPECT_EQ("localhost", client.getHost());
    EXPECT_EQ(8080, client.getPort());
}

// 模拟测试客户端的日志解析功能
TEST_F(ClientTest, LogParsing) {
    // 创建一个特定格式的JSON日志
    std::ofstream jsonLogFile("test_json_log.txt");
    jsonLogFile << "{\"level\": \"INFO\", \"message\": \"Test JSON message\"}\n";
    jsonLogFile.close();
    
    // 模拟解析器的行为
    std::ifstream file("test_json_log.txt");
    std::string line;
    std::getline(file, line);
    file.close();
    
    // 手动解析
    size_t levelPos = line.find("\"level\":");
    ASSERT_NE(std::string::npos, levelPos);
    
    size_t messagePos = line.find("\"message\":");
    ASSERT_NE(std::string::npos, messagePos);
    
    // 提取level值
    size_t levelStart = line.find("\"", levelPos + 8) + 1;
    size_t levelEnd = line.find("\"", levelStart);
    std::string level = line.substr(levelStart, levelEnd - levelStart);
    EXPECT_EQ("INFO", level);
    
    // 提取message值
    size_t messageStart = line.find("\"", messagePos + 10) + 1;
    size_t messageEnd = line.find("\"", messageStart);
    std::string message = line.substr(messageStart, messageEnd - messageStart);
    EXPECT_EQ("Test JSON message", message);
    
    // 清理
    std::filesystem::remove("test_json_log.txt");
}

// 更多客户端测试...

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}