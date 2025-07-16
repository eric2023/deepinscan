# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# 添加子目录
add_subdirectory(sane)
add_subdirectory(genesys)
add_subdirectory(vendors)

# 驱动模块源文件
set(DRIVERS_SOURCES
    ${SANE_SOURCES}
    ${GENESYS_SOURCES}
    ${VENDORS_SOURCES}
)

# 将驱动模块源文件添加到父目标
set(DRIVERS_SOURCES ${DRIVERS_SOURCES} PARENT_SCOPE) 