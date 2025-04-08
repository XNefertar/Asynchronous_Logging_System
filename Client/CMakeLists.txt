add_library(Client_Lib Client.cpp)

target_include_directories(Client_Lib 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Client
)