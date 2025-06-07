#!/bin/bash
# test_resources.sh

# 参数检查
if [ $# -lt 4 ]; then
    echo "用法: $0 [async|sync] <线程数> <每线程日志数> <结果输出文件>"
    exit 1
fi

MODE=$1
THREADS=$2
LOGS_PER_THREAD=$3
OUTPUT_FILE=$4

# 创建输出目录
mkdir -p ./test_results

# 启动资源监控
vmstat 1 > ./test_results/${OUTPUT_FILE}_vmstat.txt &
VMSTAT_PID=$!

iostat -xm 1 > ./test_results/${OUTPUT_FILE}_iostat.txt &
IOSTAT_PID=$!

# 运行测试程序
echo "开始 $MODE 模式测试, 线程数: $THREADS, 每线程日志数: $LOGS_PER_THREAD"
./test_log_throughput $MODE $THREADS $LOGS_PER_THREAD > ./test_results/${OUTPUT_FILE}_throughput.txt

# 停止资源监控
kill $VMSTAT_PID
kill $IOSTAT_PID

echo "测试完成，结果已保存到 ./test_results/${OUTPUT_FILE}_*.txt"