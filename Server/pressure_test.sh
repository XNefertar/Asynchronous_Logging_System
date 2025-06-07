#!/bin/bash
# pressure_test.sh

# 创建结果目录
mkdir -p ./pressure_test_results

# 测试不同线程配置
THREAD_COUNTS=(1 2 4 8 16 32 64)
LOGS_PER_THREAD=10000

echo "=== 异步模式压力测试 ==="
for threads in "${THREAD_COUNTS[@]}"; do
    echo "测试 $threads 并发线程..."
    ./test_log_throughput async $threads $LOGS_PER_THREAD > ./pressure_test_results/async_${threads}_threads.txt
    sleep 5  # 缓冲时间
done

echo "=== 同步模式压力测试 ==="
for threads in "${THREAD_COUNTS[@]}"; do
    echo "测试 $threads 并发线程..."
    ./test_log_throughput sync $threads $LOGS_PER_THREAD > ./pressure_test_results/sync_${threads}_threads.txt
    sleep 5  # 缓冲时间
done

echo "压力测试完成，结果已保存到 ./pressure_test_results/ 目录"