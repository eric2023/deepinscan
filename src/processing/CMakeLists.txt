# DeepinScan Processing Module

find_package(Qt5 REQUIRED COMPONENTS Core Gui Concurrent)

# 源文件列表 - 使用简化版本（已验证稳定）
set(PROCESSING_SOURCES
    dscannerimageprocessor_simple.cpp
    dscannerimageprocessor_moc.cpp
    simple_simd_support.cpp              # 简化的SIMD支持
    performance_optimizer.cpp            # 性能优化器
    # simd_image_algorithms.cpp          # 暂时禁用，有链接错误
    # 备份文件
    # dscannerimageprocessor_simple.cpp
    # dscannerimageprocessor_complete.cpp
    # 原有复杂实现（暂时保留用于参考）
    # dscannerimageprocessor.cpp
    # dscannerimageprocessor_p.cpp
    # advanced_image_processor.cpp
    # imageprocessing_algorithms.cpp
    # imageformat_handler.cpp
    # scandata_processor.cpp
    # multithreaded_processor.cpp
    # memory_optimized_processor.cpp
)

# 头文件列表 - 简化版本
set(PROCESSING_HEADERS
    # 暂时注释掉复杂的头文件
    # dscannerimageprocessor_p.h
    # advanced_image_processor.h
    # simd_image_algorithms.h
    # multithreaded_processor.h
    # memory_optimized_processor.h
)

# 为处理源文件添加完整路径
set(PROCESSING_SOURCES_FULL)
foreach(src ${PROCESSING_SOURCES})
    list(APPEND PROCESSING_SOURCES_FULL "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

# 将处理源文件传递给父级作用域
set(PROCESSING_SOURCES ${PROCESSING_SOURCES_FULL} PARENT_SCOPE)

# 注释掉单独的库创建，因为我们将源文件添加到主库
# add_library(deepinscan-processing STATIC
#     ${PROCESSING_SOURCES}
#     ${PROCESSING_HEADERS}
# )

#===============================================================================
# 以下所有target相关配置被暂时注释掉，因为processing模块现在直接集成到主库中
# 当将来需要独立的processing库时，可以取消注释这些配置
#===============================================================================

if(FALSE)  # 禁用以下所有配置

# 包含目录
target_include_directories(deepinscan-processing
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# SIMD编译选项检测和设置
include(CheckCXXCompilerFlag)

# 检测SSE2支持
check_cxx_compiler_flag("-msse2" COMPILER_SUPPORTS_SSE2)
if(COMPILER_SUPPORTS_SSE2)
    target_compile_definitions(deepinscan-processing PRIVATE SIMD_SSE2_SUPPORTED)
    target_compile_options(deepinscan-processing PRIVATE -msse2)
    message(STATUS "SSE2 SIMD support enabled")
endif()

# 检测AVX2支持
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
if(COMPILER_SUPPORTS_AVX2)
    target_compile_definitions(deepinscan-processing PRIVATE SIMD_AVX2_SUPPORTED)
    target_compile_options(deepinscan-processing PRIVATE -mavx2)
    message(STATUS "AVX2 SIMD support enabled")
endif()

# 检测ARM NEON支持（仅在ARM平台）
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    check_cxx_compiler_flag("-mfpu=neon" COMPILER_SUPPORTS_NEON)
    if(COMPILER_SUPPORTS_NEON)
        target_compile_definitions(deepinscan-processing PRIVATE SIMD_NEON_SUPPORTED)
        target_compile_options(deepinscan-processing PRIVATE -mfpu=neon)
        message(STATUS "ARM NEON SIMD support enabled")
    endif()
endif()

# 链接Qt库
target_link_libraries(deepinscan-processing
    Qt5::Core
    Qt5::Gui
    Qt5::Concurrent
)

# 编译选项
target_compile_features(deepinscan-processing PRIVATE cxx_std_17)
target_compile_options(deepinscan-processing PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -O3 -march=native>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -O3 -march=native>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
)

# 性能优化选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(deepinscan-processing PRIVATE
        -ffast-math
        -funroll-loops
        -fomit-frame-pointer
    )
endif()

# 安装目标
install(TARGETS deepinscan-processing
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

endif()  # 结束禁用的配置块 