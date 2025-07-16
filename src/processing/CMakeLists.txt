# DeepinScan Processing Module

find_package(Qt5 REQUIRED COMPONENTS Core Gui Concurrent)

# 源文件列表
set(PROCESSING_SOURCES
    dscannerimageprocessor.cpp
    dscannerimageprocessor_p.cpp
    advanced_image_processor.cpp
    simd_image_algorithms.cpp
    imageprocessing_algorithms.cpp
    imageformat_handler.cpp
    scandata_processor.cpp
    multithreaded_processor.cpp
    memory_optimized_processor.cpp
)

# 头文件列表
set(PROCESSING_HEADERS
    dscannerimageprocessor_p.h
    advanced_image_processor.h
    simd_image_algorithms.h
    multithreaded_processor.h
    memory_optimized_processor.h
)

# 创建处理模块库
add_library(deepinscan-processing STATIC
    ${PROCESSING_SOURCES}
    ${PROCESSING_HEADERS}
)

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