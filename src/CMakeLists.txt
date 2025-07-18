# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# 启用Qt自动MOC
set(CMAKE_AUTOMOC ON)

# 添加子目录
add_subdirectory(core)
add_subdirectory(communication/network)
# add_subdirectory(drivers)  # 暂时禁用，修复编译错误后再启用
add_subdirectory(processing)  # 使用简化版本
add_subdirectory(gui)         # 启用GUI模块

# 收集所有源文件
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${PROCESSING_SOURCES}  # 使用简化版本
    # ${DRIVERS_SOURCES}    # 暂时禁用驱动源文件
)

# 添加动态库目标
add_library(deepinscan SHARED ${ALL_SOURCES})

# 添加静态库目标（用于解决链接问题）
add_library(deepinscan_static STATIC ${ALL_SOURCES})

# 设置库属性
set_target_properties(deepinscan PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/Scanner/DScannerGlobal.h;${CMAKE_SOURCE_DIR}/include/Scanner/DScannerImageProcessor.h"
)

# 链接依赖 - 动态库
target_link_libraries(deepinscan
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    Qt5::Xml
    ${CMAKE_DL_LIBS}
)

# 链接依赖 - 静态库
target_link_libraries(deepinscan_static
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    Qt5::Xml
    ${CMAKE_DL_LIBS}
)

# 包含目录 - 动态库
target_include_directories(deepinscan
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 包含目录 - 静态库
target_include_directories(deepinscan_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 安装规则
install(TARGETS deepinscan
    EXPORT DeepinScanTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/Scanner
) 