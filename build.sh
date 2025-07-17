#!/bin/bash

# SPDX-FileCopyrightText: 2024 DeepinScan Team
# SPDX-License-Identifier: LGPL-3.0-or-later

# DeepinScan 统一构建脚本
# 功能：编译、测试、打包、清理

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_RESULTS_DIR="$PROJECT_ROOT/test_results"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')

# 颜色输出定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# 显示帮助信息
show_help() {
    cat << EOF
DeepinScan 统一构建脚本

用法: $0 [选项] [操作]

操作:
  build       构建项目 (默认)
  test        运行测试套件
  package     生成deb包
  clean       清理构建文件
  install     安装到系统
  all         执行完整流程 (build + test + package)

选项:
  -h, --help          显示此帮助信息
  -t, --type TYPE     构建类型 (Debug|Release|RelWithDebInfo) [默认: Debug]
  -j, --jobs N        并行编译任务数 [默认: \$(nproc)]
  -v, --verbose       详细输出
  --no-tests          跳过测试
  --no-package        跳过打包
  --clean-first       构建前先清理

环境变量:
  BUILD_TYPE          构建类型 (同 --type)
  CMAKE_FLAGS         额外的CMake参数
  MAKE_FLAGS          额外的Make参数

示例:
  $0                  # 默认Debug构建
  $0 build -t Release # Release构建
  $0 test             # 只运行测试
  $0 package          # 只生成deb包
  $0 all -t Release   # 完整Release流程
  $0 clean            # 清理构建文件

EOF
}

# 解析命令行参数
OPERATION="build"
PARALLEL_JOBS=$(nproc)
VERBOSE=false
NO_TESTS=false
NO_PACKAGE=false
CLEAN_FIRST=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --no-tests)
            NO_TESTS=true
            shift
            ;;
        --no-package)
            NO_PACKAGE=true
            shift
            ;;
        --clean-first)
            CLEAN_FIRST=true
            shift
            ;;
        build|test|package|clean|install|all)
            OPERATION="$1"
            shift
            ;;
        *)
            log_error "未知参数: $1"
            show_help
            exit 1
            ;;
    esac
done

# 设置详细输出
if [ "$VERBOSE" = true ]; then
    set -x
fi

# 验证构建类型
case "$BUILD_TYPE" in
    Debug|Release|RelWithDebInfo)
        ;;
    *)
        log_error "无效的构建类型: $BUILD_TYPE"
        log_info "支持的类型: Debug, Release, RelWithDebInfo"
        exit 1
        ;;
esac

echo "======================================="
echo "DeepinScan 统一构建脚本"
echo "======================================="
echo "项目根目录: $PROJECT_ROOT"
echo "构建目录: $BUILD_DIR"
echo "构建类型: $BUILD_TYPE"
echo "并行任务: $PARALLEL_JOBS"
echo "操作: $OPERATION"
echo "时间戳: $TIMESTAMP"
echo ""

# 检查环境依赖
check_environment() {
    log_info "检查构建环境..."
    
    local missing_tools=()
    
    if ! command -v cmake >/dev/null 2>&1; then
        missing_tools+=("cmake")
    fi
    
    if ! command -v make >/dev/null 2>&1; then
        missing_tools+=("make")
    fi
    
    if ! command -v g++ >/dev/null 2>&1; then
        missing_tools+=("g++")
    fi
    
    if ! command -v pkg-config >/dev/null 2>&1; then
        missing_tools+=("pkg-config")
    fi
    
    if [ ${#missing_tools[@]} -gt 0 ]; then
        log_error "缺少必要工具: ${missing_tools[*]}"
        log_info "请安装：sudo apt-get install build-essential cmake pkg-config"
        exit 1
    fi
    
    # 检查Qt环境
    if ! pkg-config --exists Qt5Core; then
        log_error "Qt5开发环境未找到"
        log_info "请安装：sudo apt-get install qtbase5-dev"
        exit 1
    fi
    
    # 检查DTK环境
    if ! pkg-config --exists dtkwidget; then
        log_warning "DTK开发环境未找到，某些功能可能不可用"
        log_info "请安装：sudo apt-get install libdtkwidget-dev"
    fi
    
    log_success "构建环境检查通过"
}

# 清理构建文件
clean_build() {
    log_info "清理构建文件..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_success "已清理构建目录: $BUILD_DIR"
    fi
    
    if [ -d "$TEST_RESULTS_DIR" ]; then
        rm -rf "$TEST_RESULTS_DIR"
        log_success "已清理测试结果目录: $TEST_RESULTS_DIR"
    fi
    
    # 清理其他临时文件
    find "$PROJECT_ROOT" -name "*.o" -delete
    find "$PROJECT_ROOT" -name "*.so" -delete 2>/dev/null || true
    find "$PROJECT_ROOT" -name "*.a" -delete 2>/dev/null || true
    find "$PROJECT_ROOT" -name "compile_commands.json" -delete 2>/dev/null || true
    
    log_success "清理完成"
}

# 配置项目
configure_project() {
    log_info "配置项目..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_INSTALL_PREFIX=/usr"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        "-DBUILD_SHARED_LIBS=ON"
        "-DBUILD_TESTING=ON"
    )
    
    # 添加额外的CMake参数
    if [ -n "$CMAKE_FLAGS" ]; then
        cmake_args+=($CMAKE_FLAGS)
    fi
    
    log_info "CMake参数: ${cmake_args[*]}"
    
    if cmake .. "${cmake_args[@]}"; then
        log_success "项目配置成功"
    else
        log_error "项目配置失败"
        exit 1
    fi
}

# 构建项目
build_project() {
    log_info "构建项目..."
    
    cd "$BUILD_DIR"
    
    local make_args=("-j$PARALLEL_JOBS")
    
    # 添加额外的Make参数
    if [ -n "$MAKE_FLAGS" ]; then
        make_args+=($MAKE_FLAGS)
    fi
    
    if [ "$VERBOSE" = true ]; then
        make_args+=("VERBOSE=1")
    fi
    
    log_info "Make参数: ${make_args[*]}"
    
    if make "${make_args[@]}"; then
        log_success "项目构建成功"
    else
        log_error "项目构建失败"
        exit 1
    fi
}

# 运行测试
run_tests() {
    if [ "$NO_TESTS" = true ]; then
        log_info "跳过测试 (--no-tests)"
        return 0
    fi
    
    log_info "运行测试套件..."
    
    mkdir -p "$TEST_RESULTS_DIR"
    cd "$BUILD_DIR"
    
    # 运行CTest
    if ctest --output-on-failure --parallel "$PARALLEL_JOBS" 2>&1 | tee "$TEST_RESULTS_DIR/ctest_${TIMESTAMP}.log"; then
        log_success "CTest测试通过"
    else
        log_warning "CTest测试有失败项，详见日志"
    fi
    
    # 运行自定义测试
    log_info "运行功能验证测试..."
    
    local test_files=(
        "test_dscannerdevice"
        "test_dscannerexception" 
        "test_dscannertypes"
        "test_device_discovery"
        "test_scan_workflow"
        "test_image_processing"
        "test_error_handling"
    )
    
    local passed_tests=0
    local total_tests=${#test_files[@]}
    
    for test_file in "${test_files[@]}"; do
        if [ -x "$test_file" ]; then
            log_info "运行测试: $test_file"
            if ./"$test_file" > "$TEST_RESULTS_DIR/${test_file}_${TIMESTAMP}.log" 2>&1; then
                log_success "$test_file 测试通过"
                ((passed_tests++))
            else
                log_error "$test_file 测试失败"
            fi
        else
            log_warning "测试文件不存在或不可执行: $test_file"
        fi
    done
    
    log_info "测试总结: $passed_tests/$total_tests 测试通过"
    
    # 生成测试报告
    generate_test_report "$passed_tests" "$total_tests"
}

# 生成测试报告
generate_test_report() {
    local passed_tests=$1
    local total_tests=$2
    local report_file="$TEST_RESULTS_DIR/test_report_${TIMESTAMP}.md"
    
    log_info "生成测试报告: $report_file"
    
    cat > "$report_file" << EOF
# DeepinScan 测试报告

**测试时间**: $(date)  
**构建类型**: $BUILD_TYPE  
**测试环境**: $(uname -a)  

## 测试概要

- **总测试数**: $total_tests
- **通过测试**: $passed_tests
- **失败测试**: $((total_tests - passed_tests))
- **成功率**: $(( total_tests > 0 ? passed_tests * 100 / total_tests : 0 ))%

## 构建信息

- **项目路径**: $PROJECT_ROOT
- **构建目录**: $BUILD_DIR
- **并行任务**: $PARALLEL_JOBS

## 测试结果

$(if [ $passed_tests -eq $total_tests ]; then
    echo "✅ 所有测试通过，项目功能验证成功！"
else
    echo "⚠️ 发现 $((total_tests - passed_tests)) 个测试失败，需要进一步检查。"
fi)

## 详细日志

$(ls -la "$TEST_RESULTS_DIR"/*_${TIMESTAMP}.log 2>/dev/null || echo "无详细日志文件")

---
报告生成时间: $(date)
EOF

    log_success "测试报告已生成: $report_file"
}

# 生成deb包
create_package() {
    if [ "$NO_PACKAGE" = true ]; then
        log_info "跳过打包 (--no-package)"
        return 0
    fi
    
    log_info "生成deb包..."
    
    cd "$PROJECT_ROOT"
    
    # 检查是否有debian目录
    if [ ! -d "debian" ]; then
        log_error "debian目录不存在，无法生成deb包"
        return 1
    fi
    
    # 检查dpkg-buildpackage是否可用
    if ! command -v dpkg-buildpackage >/dev/null 2>&1; then
        log_error "dpkg-buildpackage未找到"
        log_info "请安装：sudo apt-get install dpkg-dev"
        return 1
    fi
    
    # 生成deb包
    if dpkg-buildpackage -us -uc -b; then
        log_success "deb包生成成功"
        
        # 显示生成的包文件
        log_info "生成的包文件："
        ls -la ../*.deb 2>/dev/null || log_warning "未找到生成的deb文件"
    else
        log_error "deb包生成失败"
        return 1
    fi
}

# 安装到系统
install_project() {
    log_info "安装项目到系统..."
    
    cd "$BUILD_DIR"
    
    if sudo make install; then
        log_success "项目安装成功"
        
        # 更新库缓存
        sudo ldconfig
        
        log_info "安装完成，可以使用以下命令："
        log_info "  deepinscan          # 启动图形界面"
        log_info "  pkg-config --cflags deepinscan  # 开发编译参数"
    else
        log_error "项目安装失败"
        exit 1
    fi
}

# 主执行流程
main() {
    # 检查环境
    check_environment
    
    # 根据操作执行相应的函数
    case "$OPERATION" in
        clean)
            clean_build
            ;;
        build)
            if [ "$CLEAN_FIRST" = true ]; then
                clean_build
            fi
            configure_project
            build_project
            ;;
        test)
            if [ ! -d "$BUILD_DIR" ]; then
                log_info "构建目录不存在，先执行构建..."
                configure_project
                build_project
            fi
            run_tests
            ;;
        package)
            if [ ! -d "$BUILD_DIR" ]; then
                log_info "构建目录不存在，先执行构建..."
                configure_project
                build_project
            fi
            create_package
            ;;
        install)
            if [ ! -d "$BUILD_DIR" ]; then
                log_info "构建目录不存在，先执行构建..."
                configure_project
                build_project
            fi
            install_project
            ;;
        all)
            if [ "$CLEAN_FIRST" = true ]; then
                clean_build
            fi
            configure_project
            build_project
            run_tests
            create_package
            ;;
        *)
            log_error "未知操作: $OPERATION"
            show_help
            exit 1
            ;;
    esac
    
    log_success "操作 '$OPERATION' 完成！"
}

# 运行主函数
main "$@" 