#ifndef __Sock_hpp__
#define __Sock_hpp__

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>

class Sock
{
    const static int backlog = 5;
public:
    Sock() = default;
    ~Sock() = default;

    static int Socket()
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            std::cerr << "socket error" << std::endl;
            exit(1);
        }
        std::cout << "Socket fd: " << fd << std::endl;
        // 设置端口复用
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        return fd;
    }

    static void Bind(int fd, int port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            std::cerr << "bind error" << std::endl;
            exit(1);
        }
        std::cout << "Bind fd: " << fd << std::endl;
    }

    static void Listen(int fd)
    {
        if (listen(fd, backlog) < 0)
        {
            std::cerr << "listen error" << std::endl;
            exit(1);
        }
        std::cout << "Listen fd: " << fd << std::endl;
    }

    static int Accept(int sock, std::string& ip, uint16_t& port)
    {
        struct  sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = accept(sock, (struct sockaddr*)&addr, &len);
        if(fd < 0)
        {
            std::cerr << "accept error" << std::endl;
            exit(1);
        }
        ip = inet_ntoa(addr.sin_addr);
        port = ntohs(addr.sin_port);
        std::cout << "Accept fd: " << fd << std::endl;
        return fd;
    }
};

#endif // __Sock_hpp__