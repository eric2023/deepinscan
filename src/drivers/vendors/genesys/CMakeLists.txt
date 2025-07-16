# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# Genesys厂商驱动源文件
set(GENESYS_VENDOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/genesys_driver_complete.cpp
)

# Genesys厂商驱动头文件
set(GENESYS_VENDOR_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/genesys_driver_complete.h
)

# 将Genesys厂商模块源文件添加到父目标
set(GENESYS_VENDOR_SOURCES ${GENESYS_VENDOR_SOURCES} PARENT_SCOPE)
set(GENESYS_VENDOR_HEADERS ${GENESYS_VENDOR_HEADERS} PARENT_SCOPE) 