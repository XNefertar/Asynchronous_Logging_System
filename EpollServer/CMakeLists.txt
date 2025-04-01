add_library(EpollServer_Lib EpollServer.cpp main.cpp)
target_include_directories(EpollServer_Lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/EpollServer)