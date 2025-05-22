cmake_minimum_required(VERSION 3.10)
project(AsyncLogSystemTests)

# 开启测试覆盖率
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -O0")

# 查找必要的库
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(benchmark REQUIRED)  # 添加这一行
find_library(MYSQL_LIBRARY mysqlclient)
find_package(OpenSSL REQUIRED)

# 包含目录
include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/..
)

# 设置性能测试的链接库
set(PERFORMANCE_LIBRARIES
    project_lib
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${MYSQL_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    benchmark  # 添加这一行
    benchmark_main  # 可选，如果你不想自己定义main函数
    gcov
)

# 添加原项目源文件，确保包含SessionManager.cpp
set(PROJECT_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/../WebSocket/WebSocket.cpp
    ${PROJECT_SOURCE_DIR}/../WebSocket/WebSocketApiHandlers.cpp
    ${PROJECT_SOURCE_DIR}/../EpollServer/EpollServer.cpp
    # ${PROJECT_SOURCE_DIR}/../MySQL/SqlConnPool.cpp
    ${PROJECT_SOURCE_DIR}/../Client/Client.cpp
    ${PROJECT_SOURCE_DIR}/../LogMessage/LogMessage.cpp
    ${PROJECT_SOURCE_DIR}/../Util/LogTemplates.cpp
    ${PROJECT_SOURCE_DIR}/../Util/SessionManager.cpp  # 添加原始SessionManager实现
    ${PROJECT_SOURCE_DIR}/mocks/GlobalVariables.cpp  # 添加这一行
)

set(TEST_MOCK_FILES
    ${PROJECT_SOURCE_DIR}/mocks/GlobalVariables.cpp
    ${PROJECT_SOURCE_DIR}/mocks/MockSqlConnPool.cpp
)


# 创建一个静态库，只包含项目源文件
add_library(project_lib STATIC ${PROJECT_SOURCE_FILES} ${TEST_MOCK_FILES})
target_compile_definitions(project_lib PUBLIC EXCLUDE_MAIN)


# 设置通用链接库
set(COMMON_LIBRARIES
    project_lib
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${MYSQL_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    gcov
)

# 单元测试 - 保留这些定义但删除EXCLUDE_MAIN行
add_executable(EpollServer_test unit/EpollServer_test.cpp)
target_link_libraries(EpollServer_test ${COMMON_LIBRARIES})

add_executable(WebSocket_test unit/WebSocket_test.cpp)
target_link_libraries(WebSocket_test ${COMMON_LIBRARIES})

add_executable(SqlConnPool_test unit/SqlConnPool_test.cpp)
target_link_libraries(SqlConnPool_test ${COMMON_LIBRARIES})

add_executable(Client_test unit/Client_test.cpp)
target_link_libraries(Client_test ${COMMON_LIBRARIES})

# 集成测试 - 同样处理
add_executable(ServerClient_test integration/ServerClient_test.cpp)
target_link_libraries(ServerClient_test ${COMMON_LIBRARIES})

add_executable(WebSocketComm_test integration/WebSocketComm_test.cpp)
target_link_libraries(WebSocketComm_test ${COMMON_LIBRARIES})

add_executable(HighLoad_test performance/HighLoad_test.cpp)
target_link_libraries(HighLoad_test ${PERFORMANCE_LIBRARIES})  # 使用包含benchmark的库列表

add_executable(MultiClient_test performance/MultiClient_test.cpp)
target_link_libraries(MultiClient_test ${PERFORMANCE_LIBRARIES})  # 使用包含benchmark的库列表

# 测试目标
add_custom_target(run_tests
    COMMAND EpollServer_test
    COMMAND WebSocket_test
    COMMAND SqlConnPool_test
    COMMAND Client_test
    COMMAND ServerClient_test
    COMMAND WebSocketComm_test
    COMMAND HighLoad_test
    COMMAND MultiClient_test
)