#ifndef _DEAMON_HPP
#define _DEAMON_HPP

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// 不重定向标准I/O的守护进程
void daemon_no_redirect(const char* path = nullptr)
{
    pid_t pid = fork();

    if(pid < 0){
        std::cerr << "fork error" << std::endl;
        exit(1);
    }
    // 父进程退出
    else if(pid > 0) { exit(0); }

    // 创建新的会话
    // setsid()函数创建一个新的会话并将当前进程设置为该会话的领导进程
    // 这将使当前进程脱离控制终端，并成为新的会话的领导进程
    setsid();
    
    // 可选：更改工作目录
    if (path) chdir(path);
    
    // 可选：设置umask
    // umask(0);
    
    // 注意：这里不关闭或重定向标准I/O流
    // 这允许进程继续使用原有的标准输入输出
    
    std::cout << "\033[1;32m[守护进程]\033[0m 已启动, 保留标准I/O" << std::endl;
}

// 原始的守护进程函数保持不变，作为选择
void daemon(const char* path = nullptr)
{
    pid_t pid = fork();

    if(pid < 0){
        std::cerr << "fork error" << std::endl;
        exit(1);
    }
    else if(pid > 0) { exit(0); }

    setsid();
    if (path) chdir(path);
    
    int fd = open("/dev/null", O_RDWR);
    if(fd < 0){
        std::cerr << "open /dev/null error" << std::endl;
        exit(1);
    }
    
    // 标准守护进程会重定向标准I/O
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    
    close(fd);
}

#endif