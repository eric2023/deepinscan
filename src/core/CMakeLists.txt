# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# 核心源文件
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannerglobal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannerexception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannerdevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannermanager.cpp
)

# 核心模块头文件
set(CORE_HEADERS
    dscannerdevice_p.h
)

# 将核心模块源文件添加到父目标
set(CORE_SOURCES ${CORE_SOURCES} PARENT_SCOPE)
set(CORE_HEADERS ${CORE_HEADERS} PARENT_SCOPE) 