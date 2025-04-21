#!/bin/bash

# 切换到测试目录
cd "$(dirname "$0")"

# 创建构建目录
mkdir -p build
cd build

# 配置并构建测试
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# 运行单元测试
echo "Running unit tests..."
./EpollServer_test
./WebSocket_test
./SqlConnPool_test
./Client_test

# 运行集成测试
echo "Running integration tests..."
./ServerClient_test
./WebSocketComm_test

# 运行性能测试
echo "Running performance tests..."
./HighLoad_test
./MultiClient_test

# 返回测试目录
cd ..