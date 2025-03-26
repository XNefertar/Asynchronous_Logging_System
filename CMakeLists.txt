# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(AsynchronousLoggingSystem)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(test main.cpp)