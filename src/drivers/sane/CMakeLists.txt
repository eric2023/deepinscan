# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# SANE驱动源文件
set(SANE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannersane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannersane_p.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sane_option_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sane_api_complete.cpp
)

# SANE驱动头文件
set(SANE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannersane_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sane_option_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sane_preview_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sane_api_complete.h
)

# 如果找到SANE库，则添加相关编译定义
if(SANE_FOUND)
    add_definitions(-DHAVE_SANE)
    include_directories(${SANE_INCLUDE_DIRS})
endif()

# 将SANE模块源文件添加到父目标
set(SANE_SOURCES ${SANE_SOURCES} PARENT_SCOPE)
set(SANE_HEADERS ${SANE_HEADERS} PARENT_SCOPE) 