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

class ClientTCP
{
private:
    int _socketfd;
    int _port;
    std::string _address;
    std::thread _reader;

public:
    ClientTCP(const std::string& address, int port, int socketfd = -1)
        : _address(address),
          _port(port),
          _socketfd(socketfd)
    {}
    ~ClientTCP() { close(_socketfd); }
    
    void createSocket()
    {
        _socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_socketfd < 0)
        {
            std::cerr << "Error creating socket" << std::endl;
            exit(1);
        }
        std::cout << "socket " << _socketfd << " created" << std::endl;
    }

    void connectToServer()
    {
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_port);
        server_addr.sin_addr.s_addr = inet_addr(_address.c_str());

        if(connect(_socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Error connecting to server" << std::endl;
            exit(1);
        }
        std::cout << "connected to server" << std::endl;
    }

    void run(){
        std::string message;
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_port);
        server_addr.sin_addr.s_addr = inet_addr(_address.c_str());
        std::ifstream file("/home/xl/repositories/Asynchronous_Logging_System/build/log.txt");
        if (!file.is_open()) {
            // If the file does not exist or cannot be opened, create a new file
            std::ofstream new_file("/home/xl/repositories/Asynchronous_Logging_System/build/log.txt");
            if (!new_file.is_open()) {
            // If the new file cannot be created, print an error and return
            std::cerr << "Error creating file" << std::endl;
            return;
            }
            new_file.close(); // Close the newly created file
            file.open("/home/xl/repositories/Asynchronous_Logging_System/build/log.txt"); // Reopen the file for reading
        }
        if (!file.is_open()) {
            // If the file still cannot be opened after creation, print an error and return
            std::cerr << "Error opening file" << std::endl;
            return;
        }
        for(;;){
            std::stringstream tmp_buffer;
            tmp_buffer << file.rdbuf();
            message = tmp_buffer.str();
            while(message.empty()){
                std::this_thread::sleep_for(std::chrono::seconds(1));
                file.clear();
                message.clear();
                message += "empty file...";
                write(_socketfd, message.c_str(), message.size());
            }
            write(_socketfd, message.c_str(), message.size());

            char buffer[1024];
            struct sockaddr_in temp;
            socklen_t len = sizeof(temp);
            ssize_t n = recvfrom(_socketfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&temp, &len);
            if (n < 0)
            {
                std::cerr << "Failed to receive message. errno: " << errno << std::endl;
                exit(1);
            }
            buffer[n] = '\0';
            // std::cout << "服务器处理结果 # " << buffer << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};




#endif // __CLIENT_HPP__