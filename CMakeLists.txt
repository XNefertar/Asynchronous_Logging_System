# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(AsynchronousLoggingSystem)

# Set -g flag for debugging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(test main.cpp)