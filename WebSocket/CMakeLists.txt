add_library(WebSocket_Lib 
    WebSocket.cpp 
    WebSocketApiHandlers.cpp 
    WebSocketServer.cpp
)

target_include_directories(WebSocket_Lib
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/WebSocket
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollServer
    ${CMAKE_CURRENT_SOURCE_DIR}/Util
    ${CMAKE_CURRENT_SOURCE_DIR}/MySQL
)

target_link_libraries(WebSocket_Lib
    PUBLIC
    pthread
    mysqlclient
    ${OPENSSL_LIBRARIES}
)