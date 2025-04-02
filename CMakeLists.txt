# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(AsynchronousLoggingSystem)

# Set -g flag for debugging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

# find_package(MySQL REQUIRED)
# if(NOT MySQL_FOUND)
#   set(MYSQL_INCLUDE_DIR /usr/include/mysql)
#   set(MYSQL_LIBRARIES mysqlclient)
# endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Client)
add_subdirectory(EpollServer)
# add_subdirectory(Server)

# add_executable(server Server/Server.cpp)
add_executable(epoll_server 
    EpollServer/EpollServer.cpp 
    EpollServer/main.cpp 
    MySQL/SqlConnPool.cpp
    LogMessage/LogMessage.cpp
)
add_executable(client 
    Client/Client.cpp
    LogMessage/LogMessage.cpp
)
add_executable(main main.cpp)

target_link_libraries(epoll_server EpollServer_Lib)
target_link_libraries(client Client_Lib)