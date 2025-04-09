#include "Client.hpp"
#include <memory>

int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4) {
        std::cerr << "\n用法:\n\t" << argv[0] << " 服务器IP 服务器端口 [日志文件路径]\n\n";
        exit(1);
    }
    
    std::string serverip = argv[1];
    int serverport = atoi(argv[2]);
    
    std::unique_ptr<WebSocketClient> wsClient(new WebSocketClient(serverip, serverport));
    
    // 如果提供了日志文件路径，则使用它
    if (argc == 4) {
        wsClient->setLogFilePath(argv[3]);
    }
    
    wsClient->createSocket();
    wsClient->connectToServer();
    wsClient->run();
    
    return 0;
}