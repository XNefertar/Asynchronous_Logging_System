#include "Client.hpp"
#include <memory>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "\nUsage:\n\t" << argv[0] << " server_ip server_port\n\n";
        exit(1);
    }
    std::string serverip = argv[1];
    int serverport = atoi(argv[2]);

    std::unique_ptr<ClientTCP> tcpClient(new ClientTCP(serverip, serverport));

    tcpClient->createSocket();
    tcpClient->connectToServer();
    tcpClient->run();
    
    return 0;
}