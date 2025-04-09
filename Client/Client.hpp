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
    
    void createSocket();

    void connectToServer();

    void run();
};


#endif // __CLIENT_HPP__