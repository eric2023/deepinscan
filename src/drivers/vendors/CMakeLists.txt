# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# 添加厂商驱动子目录
add_subdirectory(canon)
add_subdirectory(epson)
add_subdirectory(genesys)

# 收集所有厂商驱动源文件
set(VENDORS_SOURCES
    ${CANON_SOURCES}
    ${EPSON_SOURCES}
    ${GENESYS_VENDOR_SOURCES}
)

# 将厂商驱动模块源文件添加到父目标
set(VENDORS_SOURCES ${VENDORS_SOURCES} PARENT_SCOPE) 