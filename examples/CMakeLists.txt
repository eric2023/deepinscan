# SPDX-FileCopyrightText: 2024 DeepinScan Team
# SPDX-License-Identifier: GPL-3.0-or-later

# Basic usage example
add_executable(deepinscan_basic_usage basic_usage.cpp)

target_link_libraries(deepinscan_basic_usage
    PRIVATE
        ${LIB_NAME}
        Qt5::Core
        Qt5::Widgets
)

# Install examples
install(TARGETS deepinscan_basic_usage
    RUNTIME DESTINATION bin/examples
) 