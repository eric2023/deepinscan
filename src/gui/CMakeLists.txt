# GUI模块CMakeLists.txt - 编译完整扫描应用和最小化应用

# 启用Qt自动MOC
set(CMAKE_AUTOMOC ON)

# 完整扫描应用源文件
set(DEEPINSCAN_GUI_SOURCES
    main.cpp
    application.cpp
    mainwindow.cpp
    dialogs/aboutdialog.cpp
    dialogs/settingsdialog.cpp
    dialogs/advancedexportdialog.cpp
    # Widget源文件
    widgets/devicelistwidget.cpp
    widgets/scancontrolwidget.cpp
    widgets/imagepreviewwidget.cpp
    widgets/imageprocessingwidget.cpp
    widgets/batchprocesswidget.cpp
)

# 最小化GUI应用源文件（只使用图像处理功能）
set(MINIMAL_GUI_SOURCES
    minimal_main.cpp
)

# 创建完整扫描应用程序
add_executable(deepinscan-gui ${DEEPINSCAN_GUI_SOURCES})

# 链接库 - 完整应用程序
target_link_libraries(deepinscan-gui
    deepinscan_static
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Network
    ${DTK_LIBRARIES}
)

# 包含目录 - 完整应用程序
target_include_directories(deepinscan-gui
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${DTK_INCLUDE_DIRS}
)

# 创建最小化应用程序
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

# 安装规则
install(TARGETS deepinscan-gui deepinscan-minimal
    DESTINATION bin
)

# 创建 deepinscan 符号链接指向完整应用程序
install(CODE "
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E create_symlink
            deepinscan-gui
            \${CMAKE_INSTALL_PREFIX}/bin/deepinscan
    )
") 