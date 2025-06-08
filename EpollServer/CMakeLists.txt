add_library(EpollServer_Lib 
    EpollServer.cpp 
    main.cpp 
    ../MySQL/SqlConnPool.cpp
    ../Util/SharedConfigManager.cpp
    ../Server/Server.cpp
)

find_package(OpenSSL REQUIRED)

target_include_directories(EpollServer_Lib
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/EpollServer
    ${CMAKE_CURRENT_SOURCE_DIR}/MySQL
    ${CMAKE_CURRENT_SOURCE_DIR}/Server
)

target_link_libraries(EpollServer_Lib
    PUBLIC 
    pthread
    mysqlclient
    OpenSSL::SSL
    OpenSSL::Crypto
)