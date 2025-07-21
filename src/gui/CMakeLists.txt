# 简化的GUI模块CMakeLists.txt - 只编译最小化应用

# 启用Qt自动MOC
set(CMAKE_AUTOMOC ON)

# 最小化GUI应用源文件（只使用图像处理功能）
set(MINIMAL_GUI_SOURCES
    minimal_main.cpp
)

# 创建可执行文件
add_executable(deepinscan-minimal ${MINIMAL_GUI_SOURCES})

# 链接库 - 使用静态库避免未定义引用
target_link_libraries(deepinscan-minimal
    deepinscan_static
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

# 包含目录
target_include_directories(deepinscan-minimal
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 安装规则 - 安装为 deepinscan 主应用程序
install(TARGETS deepinscan-minimal
    DESTINATION bin
)

# 创建 deepinscan 符号链接或副本 - 在debian/tmp目录中正确创建
install(CODE "
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E copy_if_different 
            \${CMAKE_INSTALL_PREFIX}/bin/deepinscan-minimal 
            \${CMAKE_INSTALL_PREFIX}/bin/deepinscan
    )
") 