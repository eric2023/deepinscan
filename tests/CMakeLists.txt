# DeepinScan Tests Module

find_package(Qt5 REQUIRED COMPONENTS Test Core Gui Concurrent)

# 测试源文件列表
set(TEST_SOURCES
    test_dscannerdevice.cpp
    test_dscannerexception.cpp
    test_dscannertypes.cpp
    test_advanced_algorithms.cpp
    test_integration.cpp
    test_simd_performance.cpp
    test_memory_optimization.cpp
    test_comprehensive_suite.cpp
    test_device_discovery.cpp
    test_scan_workflow.cpp
    test_image_processing.cpp
    test_error_handling.cpp
)

# 为每个测试创建可执行文件
foreach(TEST_SOURCE ${TEST_SOURCES})
    # 提取测试名称
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # 创建测试可执行文件
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # 链接库
    target_link_libraries(${TEST_NAME}
        Qt5::Test
        Qt5::Core
        Qt5::Gui
        Qt5::Concurrent
        deepinscan-processing
    )
    
    # 包含目录
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/processing
        ${CMAKE_SOURCE_DIR}/src/communication
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # 编译选项
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
    target_compile_options(${TEST_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -O2>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -O2>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
    )
    
    # SIMD支持（与processing模块保持一致）
    if(COMPILER_SUPPORTS_SSE2)
        target_compile_definitions(${TEST_NAME} PRIVATE SIMD_SSE2_SUPPORTED)
        target_compile_options(${TEST_NAME} PRIVATE -msse2)
    endif()
    
    if(COMPILER_SUPPORTS_AVX2)
        target_compile_definitions(${TEST_NAME} PRIVATE SIMD_AVX2_SUPPORTED)
        target_compile_options(${TEST_NAME} PRIVATE -mavx2)
    endif()
    
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64" AND COMPILER_SUPPORTS_NEON)
        target_compile_definitions(${TEST_NAME} PRIVATE SIMD_NEON_SUPPORTED)
        target_compile_options(${TEST_NAME} PRIVATE -mfpu=neon)
    endif()
    
    # 添加到测试套件
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # 设置测试属性
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 300  # 5分钟超时
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()

# 特殊配置：性能测试需要更长时间
set_tests_properties(test_simd_performance PROPERTIES TIMEOUT 600)  # 10分钟
set_tests_properties(test_memory_optimization PROPERTIES TIMEOUT 600)
set_tests_properties(test_comprehensive_suite PROPERTIES TIMEOUT 900)  # 15分钟

# 性能测试标签
set_tests_properties(test_simd_performance test_memory_optimization test_comprehensive_suite 
    PROPERTIES LABELS "performance")

# 快速测试标签
set_tests_properties(test_dscannerdevice test_dscannerexception test_dscannertypes 
    PROPERTIES LABELS "unit")

# 集成测试标签
set_tests_properties(test_advanced_algorithms test_integration 
    PROPERTIES LABELS "integration")

# 创建性能基准测试目标
add_custom_target(benchmark
    COMMAND ${CMAKE_CTEST_COMMAND} -L performance -V
    COMMENT "运行性能基准测试"
    DEPENDS test_simd_performance test_memory_optimization test_comprehensive_suite
)

# 创建快速测试目标
add_custom_target(quick_test
    COMMAND ${CMAKE_CTEST_COMMAND} -L unit -j ${CMAKE_BUILD_PARALLEL_LEVEL}
    COMMENT "运行快速单元测试"
    DEPENDS test_dscannerdevice test_dscannerexception test_dscannertypes
)

# 创建集成测试目标
add_custom_target(integration_test
    COMMAND ${CMAKE_CTEST_COMMAND} -L integration -V
    COMMENT "运行集成测试"
    DEPENDS test_advanced_algorithms test_integration
)

# 测试覆盖率支持（如果可用）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        # 为调试版本添加覆盖率标志
        foreach(TEST_SOURCE ${TEST_SOURCES})
            get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
            target_compile_options(${TEST_NAME} PRIVATE --coverage)
            target_link_options(${TEST_NAME} PRIVATE --coverage)
        endforeach()
        
        # 创建覆盖率目标
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND}
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '${CMAKE_BINARY_DIR}/*' --output-file coverage.info.cleaned
            COMMAND ${GENHTML_PATH} -o coverage coverage.info.cleaned
            COMMAND ${CMAKE_COMMAND} -E remove coverage.info coverage.info.cleaned
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "生成测试覆盖率报告"
        )
    endif()
endif()

# 安装测试（可选）
if(INSTALL_TESTS)
    foreach(TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        install(TARGETS ${TEST_NAME}
            RUNTIME DESTINATION bin/tests
        )
    endforeach()
endif()

# 打印测试配置信息
message(STATUS "DeepinScan测试配置:")
message(STATUS "  单元测试: ${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "  性能测试: 包含SIMD和内存优化基准测试")
message(STATUS "  测试超时: 单元测试5分钟，性能测试10-15分钟")
message(STATUS "  并行测试: 支持")

if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "  覆盖率分析: 已启用 (make coverage)")
endif() 