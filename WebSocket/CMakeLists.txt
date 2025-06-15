add_library(WebSocket_Lib 
    WebSocket.cpp 
    WebSocketApiHandlers.cpp 
    WebSocketServer.cpp
    ../Util/ConfigManager.cpp
    ../Util/SharedConfigManager.cpp
    ../Util/EnvConfig.cpp
)

target_include_directories(WebSocket_Lib
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/WebSocket
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollServer
    ${CMAKE_CURRENT_SOURCE_DIR}/Util
    ${CMAKE_CURRENT_SOURCE_DIR}/MySQL
    /usr/include/jsoncpp
)

target_link_libraries(WebSocket_Lib
    PUBLIC
    pthread
    mysqlclient
    ${OPENSSL_LIBRARIES}
    jsoncpp
)

# 添加编译选项
target_compile_options(WebSocket_Lib PRIVATE 
    ${JSONCPP_CFLAGS_OTHER}
)