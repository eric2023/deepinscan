# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# Canon驱动源文件
set(CANON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/canon_driver_complete.cpp
)

# Canon驱动头文件
set(CANON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/canon_driver_complete.h
)

# 将Canon模块源文件添加到父目标
set(CANON_SOURCES ${CANON_SOURCES} PARENT_SCOPE)
set(CANON_HEADERS ${CANON_HEADERS} PARENT_SCOPE) 