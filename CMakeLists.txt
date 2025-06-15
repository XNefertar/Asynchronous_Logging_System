# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(AsynchronousLoggingSystem)

# Set -g flag for debugging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

# 加入 OpenSSL 库
find_package(OpenSSL REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Client)
add_subdirectory(EpollServer)
add_subdirectory(WebSocket)
add_subdirectory(Server)

add_executable(server
    Server/main.cpp
    Server/Server.cpp
    LogMessage/LogMessage.cpp
    LogMessage/AsyncLogBuffer.cpp
    Util/SessionManager.cpp
    Util/LogTemplates.cpp
    Util/ConfigManager.cpp
    Util/SharedConfigManager.cpp
    Util/EnvConfig.cpp
    MySQL/SqlConnPool.cpp
)

# add_executable(epoll_server 
#     EpollServer/EpollServer.cpp 
#     EpollServer/main.cpp 
#     MySQL/SqlConnPool.cpp
#     LogMessage/LogMessage.cpp
#     Util/SessionManager.cpp
# )

add_executable(webserver
    WebSocket/WebSocketServer.cpp
    # WebSocket/WebSocketHelpers.cpp
    WebSocket/WebSocketApiHandlers.cpp
    Util/SessionManager.cpp
    Util/ConfigManager.cpp
    Util/SharedConfigManager.cpp
    Util/EnvConfig.cpp
    LogMessage/LogMessage.cpp
    LogMessage/AsyncLogBuffer.cpp
)

add_executable(client 
    Client/main.cpp
    Client/Client.cpp
    LogMessage/LogMessage.cpp
    LogMessage/AsyncLogBuffer.cpp
    Util/ConfigManager.cpp
    Util/SharedConfigManager.cpp
)

add_executable(main 
    main.cpp
    Util/SessionManager.cpp
    Util/LogTemplates.cpp
    Util/ConfigManager.cpp
    Util/SharedConfigManager.cpp
)

target_link_libraries(server 
    Server_Lib 
    ${OPENSSL_LIBRARIES}
    rt
)

# target_link_libraries(epoll_server 
#     EpollServer_Lib 
#     ${OPENSSL_LIBRARIES}
# )

target_link_libraries(webserver
    WebSocket_Lib
    EpollServer_Lib
    ${OPENSSL_LIBRARIES}
    jsoncpp
    rt
)

target_link_libraries(client 
    Client_Lib
    rt
)

target_link_libraries(main
    pthread
    rt
)