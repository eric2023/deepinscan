# SPDX-FileCopyrightText: 2024 DeepinScan Team
# SPDX-License-Identifier: GPL-3.0-or-later

# 核心测试程序（不使用Q_OBJECT功能，避免MOC问题）
add_executable(deepinscan_core_test core_test.cpp)

target_link_libraries(deepinscan_core_test
    PRIVATE
        deepinscan_static
        Qt5::Core
)

# 最小化示例（暂时禁用直到链接问题解决）
# add_executable(deepinscan_minimal minimal_example.cpp)
# 
# target_link_libraries(deepinscan_minimal
#     PRIVATE
#         deepinscan
#         Qt5::Core
# )

# Basic usage example (暂时禁用)
# add_executable(deepinscan_basic_usage basic_usage.cpp)
# 
# target_link_libraries(deepinscan_basic_usage
#     PRIVATE
#         deepinscan
#         Qt5::Core
#         Qt5::Widgets
# )

# Install examples
install(TARGETS deepinscan_core_test
    RUNTIME DESTINATION bin/examples
) 