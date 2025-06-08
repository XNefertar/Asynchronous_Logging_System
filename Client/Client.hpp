#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <iostream>
#include <thread>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <chrono>
#include "../Logger.hpp"
#include "../Util/ConfigManager.hpp"
#include "../Util/SharedConfigManager.hpp"
#include "../LogMessage/LogMessage.hpp"


class ClientTCP
{
private:
    int _socketfd;
    int _port;
    std::string _address;
    std::thread _reader;

public:
    ClientTCP(const std::string& address, int port, int socketfd = -1);
    ~ClientTCP();
    
    int getSocketfd() const;
    int getPort() const;
    std::string getHost() const;
    void createSocket();

    void connectToServer();
    
    // 新增测试所需方法
    bool Connect();  // 返回连接是否成功
    ssize_t sendData(const std::string& data);  // 返回发送的字节数
    std::string receiveData();  // 接收并返回数据
    void disConnect();  // 断开连接

    void run();
};


#endif // __CLIENT_HPP__