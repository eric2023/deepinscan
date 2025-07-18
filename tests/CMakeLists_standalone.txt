# 独立的性能测试CMakeLists
cmake_minimum_required(VERSION 3.16)

# 找到Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Test Widgets)

# 设置MOC
set(CMAKE_AUTOMOC ON)

# 创建独立的性能测试
add_executable(test_performance_standalone 
    test_performance_standalone.cpp
    ../src/processing/performance_optimizer.cpp
)

# 链接库
target_link_libraries(test_performance_standalone
    Qt5::Core
    Qt5::Test
    Qt5::Widgets
)

# 包含目录
target_include_directories(test_performance_standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/processing
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 添加测试
add_test(NAME "Performance-Standalone" COMMAND test_performance_standalone) 