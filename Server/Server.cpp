#include "Server.hpp"
#include <memory>

using namespace std;
using namespace Server;
static void Usage(string proc)
{
    cerr << "\nUsage:\n\t" << proc << " server_port\n\n";
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);

    std::unique_ptr<ServerTCP> tcpServer(new ServerTCP(port));

    tcpServer->init();
    tcpServer->run();

    return 0;
}