# 网络通信层源文件 - 使用基础实现
set(NETWORK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple_impl.cpp
    # 暂时只启用基础实现，避免私有类问题
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery.cpp  
    # ${CMAKE_CURRENT_SOURCE_DIR}/network_complete_discovery.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/network_discovery_stubs.cpp
)

# 设置父作用域变量
set(NETWORK_SOURCES ${NETWORK_SOURCES} PARENT_SCOPE) 