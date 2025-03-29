cmake_minimum_required(VERSION 3.10)

project(AsynchronousLoggingSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(epoll_server EpollServer.cpp EpollServer.hpp)