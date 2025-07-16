# SPDX-FileCopyrightText: 2024-2025 eric2023
# SPDX-License-Identifier: GPL-3.0-or-later

# 启用Qt自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt和DTK依赖
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DTK REQUIRED dtkwidget)

# 收集源文件
set(GUI_SOURCES
    main.cpp
    application.cpp
    mainwindow.cpp
    widgets/devicelistwidget.cpp
    widgets/scancontrolwidget.cpp
    widgets/imagepreviewwidget.cpp
    widgets/imageprocessingwidget.cpp
    widgets/batchprocesswidget.cpp
    dialogs/settingsdialog.cpp
    dialogs/aboutdialog.cpp
    dialogs/devicepropertiesdialog.cpp
    utils/imageutils.cpp
    utils/uiutils.cpp
    utils/thememanager.cpp
)

# 收集头文件
set(GUI_HEADERS
    application.h
    mainwindow.h
    widgets/devicelistwidget.h
    widgets/scancontrolwidget.h
    widgets/imagepreviewwidget.h
    widgets/imageprocessingwidget.h
    widgets/batchprocesswidget.h
    dialogs/settingsdialog.h
    dialogs/aboutdialog.h
    dialogs/devicepropertiesdialog.h
    utils/imageutils.h
    utils/uiutils.h
    utils/thememanager.h
)

# 资源文件
set(GUI_RESOURCES
    resources/deepinscan-gui.qrc
)

# 创建可执行文件
add_executable(deepinscan-gui 
    ${GUI_SOURCES} 
    ${GUI_HEADERS}
    ${GUI_RESOURCES}
)

# 设置包含目录
target_include_directories(deepinscan-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/communication/usb
    ${CMAKE_SOURCE_DIR}/src/communication/network
    ${CMAKE_SOURCE_DIR}/src/drivers/sane
    ${CMAKE_SOURCE_DIR}/src/drivers/genesys
    ${CMAKE_SOURCE_DIR}/src/processing
)

# 链接库
target_link_libraries(deepinscan-gui
    deepinscan          # 核心库
    Qt5::Widgets
    ${DTK_LIBRARIES}
)

# 设置可执行文件属性
set_target_properties(deepinscan-gui PROPERTIES
    OUTPUT_NAME "deepinscan"
    VERSION ${PROJECT_VERSION}
)

# 安装规则
install(TARGETS deepinscan-gui
    RUNTIME DESTINATION bin
)

# 安装桌面文件和图标
install(FILES resources/deepinscan.desktop
    DESTINATION share/applications
)

install(FILES resources/icons/deepinscan.svg
    DESTINATION share/icons/hicolor/scalable/apps
) 