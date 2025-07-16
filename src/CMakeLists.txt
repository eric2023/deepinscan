# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# 启用Qt自动MOC
set(CMAKE_AUTOMOC ON)

# 添加子目录
add_subdirectory(core)
add_subdirectory(drivers)
add_subdirectory(communication/network)
add_subdirectory(processing)

# 收集所有源文件
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${DRIVERS_SOURCES}
    ${NETWORK_SOURCES}
)

# 添加库目标
add_library(deepinscan SHARED ${ALL_SOURCES})

# 设置库属性
set_target_properties(deepinscan PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/Scanner/DScannerGlobal.h"
)

# 链接依赖
target_link_libraries(deepinscan
    Qt5::Core
    Qt5::Network
    Qt5::Xml
    ${CMAKE_DL_LIBS}
)

# 包含目录
target_include_directories(deepinscan
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