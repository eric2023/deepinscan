cmake_minimum_required(VERSION 3.16)

project(deepinscan
    VERSION 1.0.0
    DESCRIPTION "Professional scanner application for Deepin Linux"
    HOMEPAGE_URL "https://github.com/eric2023/deepinscan"
    LANGUAGES CXX C
)

# 设置项目版本变量
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# 查找依赖包
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network Xml Gui Concurrent PrintSupport)
find_package(PkgConfig REQUIRED)

# Find DTK packages
pkg_check_modules(DTK REQUIRED dtkwidget)

# Find libusb-1.0
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# Find SANE (optional)
pkg_check_modules(SANE libsane)

# 设置编译选项
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 设置编译标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# 设置资源文件
set(RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc
)

# 处理Qt资源文件
qt5_add_resources(RESOURCE_SOURCES ${RESOURCES})

# 添加子目录
add_subdirectory(src)

# 添加GUI程序 (暂时禁用以专注于核心功能)
# add_subdirectory(src/gui)

# 添加示例程序
add_subdirectory(examples)

# 添加测试
add_subdirectory(tests)

# 暂时禁用GUI和示例，专注于库编译
# add_subdirectory(src/gui)
# add_subdirectory(examples)

# 链接库（在子目录中的库创建之后）
if(TARGET deepinscan)
    target_link_libraries(deepinscan
        Qt5::Core
        Qt5::Widgets
        Qt5::Network
        Qt5::Xml
        Qt5::Gui
        Qt5::Concurrent
        Qt5::PrintSupport
        ${DTK_LIBRARIES}
        ${LIBUSB_LIBRARIES}
    )
    
    if(SANE_FOUND)
        target_link_libraries(deepinscan ${SANE_LIBRARIES})
        target_compile_definitions(deepinscan PRIVATE HAVE_SANE)
    endif()
endif()

# 安装配置
install(TARGETS deepinscan
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 生成pkg-config文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/deepinscan.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/deepinscan.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deepinscan.pc
    DESTINATION lib/pkgconfig
)

# CPack配置
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DeepinScan Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional scanner application for Deepin Linux")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

include(CPack) 