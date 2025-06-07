add_library(Server_Lib 
    main.cpp
    Server.cpp
    AsyncDBWriter.cpp
    ../MySQL/SqlConnPool.cpp
    ../Util/SessionManager.cpp 
    ../Util/LogTemplates.cpp 
    ../LogMessage/LogMessage.cpp
    ../LogMessage/AsyncLogBuffer.cpp
    ../WebSocket/WebSocketServer.cpp
    ../EpollServer/EpollServer.cpp
)

find_package(OpenSSL REQUIRED)

target_include_directories(Server_Lib 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Server
    ${CMAKE_CURRENT_SOURCE_DIR}/../MySQL
    ${CMAKE_CURRENT_SOURCE_DIR}/../LogMessage
    ${CMAKE_CURRENT_SOURCE_DIR}/../Util
    ${CMAKE_CURRENT_SOURCE_DIR}/../WebSocket
    ${CMAKE_CURRENT_SOURCE_DIR}/../EpollServer
    ${CMAKE_CURRENT_SOURCE_DIR}/../Client
)

target_link_libraries(Server_Lib
    PUBLIC
    pthread
    mysqlclient
    ${OPENSSL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# 添加测试可执行文件
add_executable(test_log_throughput
    test_log_throughput.cpp
)

add_executable(test_log_latency
    test_log_latency.cpp
)

# 链接测试可执行文件
target_link_libraries(test_log_throughput
    PUBLIC
    Server_Lib
    pthread
    mysqlclient
)

target_link_libraries(test_log_latency
    PUBLIC
    Server_Lib
    pthread
    mysqlclient
)

# 添加自定义命令来复制脚本并设置可执行权限
add_custom_command(
    TARGET test_log_throughput POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_SOURCE_DIR}/pressure_test.sh
        ${CMAKE_CURRENT_BINARY_DIR}/pressure_test.sh
    COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/pressure_test.sh
)

add_custom_command(
    TARGET test_log_latency POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_SOURCE_DIR}/test_resources.sh
        ${CMAKE_CURRENT_BINARY_DIR}/test_resources.sh
    COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/test_resources.sh
)