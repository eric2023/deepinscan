# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# Genesys驱动源文件
set(GENESYS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannergenesys.cpp
)

# Genesys驱动头文件
set(GENESYS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/dscannergenesys_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/genesys_driver_complete.h
)

# 将Genesys模块源文件添加到父目标
set(GENESYS_SOURCES ${GENESYS_SOURCES} PARENT_SCOPE)
set(GENESYS_HEADERS ${GENESYS_HEADERS} PARENT_SCOPE) 