#include "LogMessage.hpp"

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

void LogMessage::logMessage(LOG_LEVEL level, const char *message, ...)
{
    va_list args;
    va_start(args, message);

    char buffer[1024]{};
    sprintf(buffer, "[%s][%ld][%d] ", to_log(level).c_str(), time(nullptr), getpid());

    char response[1024]{};
    vsprintf(response, message, args);

    int fd = open(default_log_path.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
    write(fd, buffer, strlen(buffer));
    write(fd, response, strlen(response));
    write(fd, "\n", 1);
    close(fd);

    va_end(args);
}

void LogMessage::setDefaultLogPath(const std::string &path)
{
    default_log_path = path;
}

std::string LogMessage::default_log_path = "";