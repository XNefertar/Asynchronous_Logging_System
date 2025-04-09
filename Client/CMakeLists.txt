add_library(Client_Lib Client.cpp main.cpp)

target_include_directories(Client_Lib 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Client
    ${OPENSSL_INCLUDE_DIR}  # 添加OpenSSL头文件路径
)

# 链接OpenSSL库
target_link_libraries(Client_Lib
    PUBLIC
    ${OPENSSL_LIBRARIES}   # 链接OpenSSL库
    pthread               # 可能需要的线程库
)