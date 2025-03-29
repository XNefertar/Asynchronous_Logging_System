#include "EpollServer.hpp"

using namespace EpollServerSpace;
static void usage(const char *proc)
{
    printf("Usage: %s  [local_port]\n", proc);
}

int main(int argc, char *argv[])
{
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