# 网络通信层源文件 - 使用基础实现
set(NETWORK_SOURCES
    # 使用桩实现解决链接问题，避免复杂的私有类依赖
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple_stubs.cpp
    # 添加包含dscannerCore函数和SANEAPIManager的文件
    ${CMAKE_CURRENT_SOURCE_DIR}/network_discovery_stubs.cpp
    # 暂时注释完整实现，避免多重定义错误
    # ${CMAKE_CURRENT_SOURCE_DIR}/network_complete_discovery.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple_impl.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery_simple.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/dscannernetworkdiscovery.cpp  
)

# 设置父作用域变量
set(NETWORK_SOURCES ${NETWORK_SOURCES} PARENT_SCOPE) 