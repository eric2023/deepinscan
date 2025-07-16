# 网络通信层源文件
set(NETWORK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple_impl.cpp
)

# 设置父作用域变量
set(NETWORK_SOURCES ${NETWORK_SOURCES} PARENT_SCOPE) 