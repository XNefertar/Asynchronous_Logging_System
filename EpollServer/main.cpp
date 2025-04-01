#include "EpollServer.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <csignal>

using namespace EpollServerSpace;
static const int defaultPort = 8080;
static const int defaultEpollSize = 1024;
static const int defaultValue = -1;

static void usage(const char *proc)
{
    std::cout << "Usage: " << proc << " <port>" << std::endl;
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);  // 忽略SIGPIPE信号
    signal(SIGINT, signalHandler);  // 捕获Ctrl+C信号
    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }

    EpollServer *epollServer = new EpollServer(atoi(argv[1]));
    epollServer->ServerInit();
    epollServer->ServerStart();

    return 0;
}