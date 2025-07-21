# 网络通信层源文件 - 使用基础实现
set(NETWORK_SOURCES
    # 使用桩实现解决链接问题，避免复杂的私有类依赖
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple_stubs.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple_impl.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery.cpp  
    # ${CMAKE_CURRENT_SOURCE_DIR}/network_complete_discovery.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/network_discovery_stubs.cpp
)

# 设置父作用域变量
set(NETWORK_SOURCES ${NETWORK_SOURCES} PARENT_SCOPE) 