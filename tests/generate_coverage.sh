#!/bin/bash

# 切换到测试目录
cd "$(dirname "$0")"

# 运行测试
./run_tests.sh

# 切换到构建目录
cd build

# 生成覆盖率数据
gcovr -r ../../ --html --html-details -o ../coverage/coverage.html

# 打印覆盖率摘要
gcovr -r ../../ --txt -o ../coverage/summary.txt
cat ../coverage/summary.txt

# 返回测试目录
cd ..

echo "Coverage report generated in coverage/coverage.html"