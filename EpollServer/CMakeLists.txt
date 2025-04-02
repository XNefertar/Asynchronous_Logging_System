add_library(EpollServer_Lib EpollServer.cpp main.cpp ../MySQL/SqlConnPool.cpp)

target_include_directories(EpollServer_Lib
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollServer
    ${CMAKE_CURRENT_SOURCE_DIR}/MySQL
)

target_link_libraries(EpollServer_Lib
    PUBLIC 
    pthread
    mysqlclient
)