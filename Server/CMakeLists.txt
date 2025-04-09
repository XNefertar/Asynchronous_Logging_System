add_library(Server_Lib 
    main.cpp
    Server.cpp
    ../MySQL/SqlConnPool.cpp
    ../Util/SessionManager.cpp 
    ../Util/LogTemplates.cpp 
    ../LogMessage/LogMessage.cpp
)

target_include_directories(Server_Lib 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Server
    ${CMAKE_CURRENT_SOURCE_DIR}/MySQL
)

target_link_libraries(Server_Lib
    PUBLIC
    pthread
    mysqlclient
)