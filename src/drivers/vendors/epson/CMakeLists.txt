# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# Epson驱动源文件
set(EPSON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/epson_driver_complete.cpp
)

# Epson驱动头文件
set(EPSON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/epson_driver_complete.h
)

# 将Epson模块源文件添加到父目标
set(EPSON_SOURCES ${EPSON_SOURCES} PARENT_SCOPE)
set(EPSON_HEADERS ${EPSON_HEADERS} PARENT_SCOPE) 