#ifndef __LOG_MESSAGE_HPP__
#define __LOG_MESSAGE_HPP__

#include <iostream>
#include <string>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NORMAL 0
#define WARNING 1
#define ERROR 2
#define FATAL 3
#define DEBUG 4
#define INFO 5

#define LOG_LEVEL int

std::string to_log(LOG_LEVEL level)
{
    switch (level)
    {
    case NORMAL:
        return "NORMAL";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    case FATAL:
        return "FATAL";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    default:
        return "UNKNOWN";
    }
}

void logMessage(LOG_LEVEL level, const char *message, ...)
{
    va_list args;
    va_start(args, message);

    char buffer[1024]{};
    sprintf(buffer, "[%s][%ld][%ld] ", to_log(level).c_str(), time(nullptr), getpid());


    char response[1024]{};
    vsprintf(response, message, args);

    int fd = open("./log_message.txt", O_CREAT | O_WRONLY | O_APPEND, 0666);
    write(fd, buffer, strlen(buffer));
    write(fd, response, strlen(response));
    write(fd, "\n", 1);
    close(fd);

    va_end(args);
}

#endif // __LOG_MESSAGE_HPP__
