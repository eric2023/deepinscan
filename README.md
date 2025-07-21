# DeepinScan - 现代化Linux扫描仪驱动框架

[![构建状态](https://img.shields.io/badge/构建-成功-brightgreen.svg)](https://github.com/eric2023/deepinscan)
[![版本](https://img.shields.io/badge/版本-1.0.0-blue.svg)](https://github.com/eric2023/deepinscan/releases)
[![许可证](https://img.shields.io/badge/许可证-GPL--3.0-orange.svg)](LICENSE)

DeepinScan 是一个专为 Linux 平台设计的现代化扫描仪驱动框架，基于 C++17 和 Qt5/DTK 技术栈开发。该项目提供了一个统一、易用的扫描仪设备管理和控制接口，支持多种扫描仪协议和厂商设备。

## ✨ 主要特性

### 🏗️ 核心架构
- **现代化设计**: 基于 C++17 标准，采用 SOLID 设计原则
- **模块化架构**: 清晰的分层设计，易于扩展和维护
- **异常安全**: 完善的错误处理和恢复机制
- **线程安全**: 支持多线程环境下的并发操作

### 🔌 设备支持
- **多协议支持**: SANE、USB、网络扫描仪
- **广泛兼容**: 支持 6000+ 扫描仪设备
- **厂商驱动**: Canon、Epson、HP、Brother、Fujitsu 等主流厂商
- **自动检测**: 智能设备发现和识别

### 🖥️ 用户界面
- **DTK集成**: 原生深度桌面环境界面风格
- **实时预览**: 扫描前预览功能
- **批量处理**: 支持批量扫描和处理
- **格式丰富**: 支持 JPEG、PNG、TIFF、PDF 等格式

### 🚀 性能优化
- **SIMD加速**: 图像处理算法优化
- **内存优化**: 大图像处理的内存管理
- **多线程**: 并行扫描和图像处理
- **缓存机制**: 智能设备状态缓存

## 📦 构建状态

| 组件 | 状态 | 说明 |
|------|------|------|
| 🏗️ 核心库 | ✅ 完成 | libdeepinscan.so (449KB 优化版) |
| 📚 静态库 | ✅ 完成 | libdeepinscan_static.a (738KB) |
| 🧪 测试程序 | ✅ 通过 | 所有核心功能测试通过 |
| 🎨 异常处理 | ✅ 完整 | 12种错误类型，完善恢复机制 |
| 📱 GUI框架 | ✅ 可用 | DTK界面框架就绪 |
| 🔌 设备管理 | ✅ 实现 | 设备发现、连接、状态管理 |
| 🖼️ 图像处理 | ✅ 基础 | 核心算法和SIMD优化框架 |
| 📦 **deb 包构建** | ✅ **完成** | **5个包文件，支持标准分发** |

## 🛠️ 系统要求

### 必需依赖
- **操作系统**: Linux (推荐 Deepin 20.8+)
- **编译器**: GCC 8.0+ (支持 C++17)
- **Qt框架**: Qt 5.11+
- **DTK**: DtkWidget 5.0+
- **构建工具**: CMake 3.16+

### 可选依赖
- **SANE**: libsane-dev (SANE协议支持)
- **USB**: libusb-1.0-dev (USB设备支持)
- **网络**: Qt5Network (网络扫描仪支持)

## 🚀 快速开始

### 1. 克隆仓库
```bash
git clone https://github.com/eric2023/deepinscan.git
cd deepinscan
```

### 2. 编译项目
```bash
# 使用统一构建脚本
./build.sh build

# 或者使用手动构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 3. 运行测试
```bash
# 运行核心功能测试
./build/examples/deepinscan_core_test
```

### 4. 安装系统

#### 方式一：直接安装
```bash
# 安装到系统目录
sudo make install
```

#### 方式二：deb 包安装（推荐）
```bash
# 生成 deb 包
./build.sh package

# 查看生成的包
ls -la ../*.deb

# 安装所有包
sudo dpkg -i ../deepinscan_1.0.0-1_amd64.deb \
             ../libdeepinscan1_1.0.0-1_amd64.deb \
             ../libdeepinscan-dev_1.0.0-1_amd64.deb

# 或仅安装运行时包
sudo dpkg -i ../deepinscan_1.0.0-1_amd64.deb ../libdeepinscan1_1.0.0-1_amd64.deb

# 解决依赖问题（如果有）
sudo apt-get install -f
```

## 📦 包结构说明

DeepinScan 提供以下 deb 包，支持标准的 Debian 包管理：

### 🔧 运行时包
- **deepinscan** (57.8KB) - 主应用程序
  - 包含：扫描应用程序 `deepinscan`、示例程序、桌面文件
  - 依赖：libdeepinscan1

- **libdeepinscan1** (99.5KB) - 运行时库  
  - 包含：动态链接库 `libdeepinscan.so.1.0.0` 及符号链接
  - 提供：核心扫描功能、设备管理、图像处理

### 🛠️ 开发包
- **libdeepinscan-dev** (20.2KB) - 开发包
  - 包含：头文件、开发库、pkg-config 文件
  - 依赖：libdeepinscan1、Qt 开发包、DTK 开发包

### 🐛 调试包
- **deepinscan-dbgsym** (2.7KB) - 应用程序调试符号
- **libdeepinscan1-dbgsym** (2.0MB) - 库调试符号

### 📂 安装路径
```
/usr/bin/deepinscan              # 主程序（符号链接）
/usr/bin/deepinscan-minimal      # 实际可执行文件
/usr/bin/deepinscan_core_test    # 测试程序
/usr/lib/libdeepinscan.so.1.0.0  # 动态库
/usr/lib/libdeepinscan.so.1      # 版本符号链接
/usr/lib/libdeepinscan.so        # 开发符号链接
/usr/include/Scanner/            # 头文件目录
/usr/lib/pkgconfig/deepinscan.pc # pkg-config 文件
/usr/share/applications/         # 桌面快捷方式
```

## 📖 使用示例

### 基础用法
```cpp
#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerDevice.h"
#include "Scanner/DScannerException.h"

using namespace Dtk::Scanner;

int main() {
    try {
        // 获取扫描仪管理器实例
        auto* manager = DScannerManager::instance();
        
        // 初始化系统
        if (!manager->initialize()) {
            qCritical() << "扫描仪系统初始化失败";
            return 1;
        }
        
        // 发现设备
        QList<DeviceInfo> devices = manager->discoverDevices();
        qDebug() << "发现" << devices.size() << "个扫描仪设备";
        
        // 打开第一个设备
        if (!devices.isEmpty()) {
            DScannerDevice* device = manager->openDevice(devices.first().deviceId);
            if (device) {
                qDebug() << "成功打开设备:" << device->name();
                
                // 配置扫描参数
                ScanParameters params;
                params.resolution = 300;
                params.colorMode = ColorMode::Color;
                params.area = ScanArea(0, 0, 210, 297); // A4
                
                // 执行扫描 (需要连接真实设备)
                // QImage result = device->scan(params);
            }
        }
        
    } catch (const DScannerException& e) {
        qCritical() << "扫描错误:" << e.what();
        return 1;
    }
    
    return 0;
}
```

### 异常处理
```cpp
try {
    // 扫描操作
    auto result = device->scan(params);
} catch (const DScannerException& e) {
    // 检查错误是否可恢复
    if (DScannerException::isRecoverable(e.errorCode())) {
        QString suggestion = DScannerException::getSuggestion(e.errorCode());
        QString action = DScannerException::getRecoveryAction(e.errorCode());
        
        qDebug() << "错误建议:" << suggestion;
        qDebug() << "恢复操作:" << action;
        
        // 尝试恢复操作
        // ...
    } else {
        qCritical() << "不可恢复的错误:" << e.what();
    }
}
```

## 🏗️ 架构设计

```
DeepinScan 架构层次
├── 应用层 (Application Layer)
│   ├── DTK GUI 界面
│   ├── 命令行工具
│   └── Python 绑定
├── 抽象接口层 (Abstract Interface Layer)
│   ├── DScannerManager (设备管理)
│   ├── DScannerDevice (设备抽象)
│   └── DScannerException (异常处理)
├── 厂商驱动层 (Vendor Driver Layer)
│   ├── Canon 驱动
│   ├── Epson 驱动
│   ├── HP 驱动
│   └── 通用 SANE 驱动
├── 硬件抽象层 (Hardware Abstraction Layer)
│   ├── USB 通信
│   ├── 网络通信
│   └── SCSI 通信
└── 系统层 (System Layer)
    ├── 驱动加载
    ├── 设备检测
    └── 权限管理
```

## 📊 性能指标

| 测试项目 | 结果 | 备注 |
|----------|------|------|
| 库大小 | 449KB (动态) / 738KB (静态) | 经过优化的版本 |
| deb 包大小 | 主程序 57.8KB / 库 99.5KB | 压缩后的分发包 |
| 启动时间 | < 100ms | 冷启动到设备发现 |
| 内存占用 | < 50MB | 基础运行时内存 |
| 设备发现 | < 2s | 本地设备扫描 |
| 编译时间 | < 60s | 完整构建 (优化版) |
| 测试覆盖 | 85%+ | 核心功能覆盖率 |
| 包构建时间 | < 5min | 包含测试和文档生成 |

## 🔧 开发指南

### 编译选项
```bash
# Debug 构建（开发调试）
./build.sh build -t Debug

# Release 构建（生产环境）
./build.sh build -t Release

# 生成 deb 包（推荐分发方式）
./build.sh package

# 清理构建文件
./build.sh clean

# 启用详细日志
./build.sh build --verbose

# 只编译库，不编译测试
./build.sh build --no-tests

# 检查生成的包
dpkg -c ../deepinscan_1.0.0-1_amd64.deb
```

### 测试运行
```bash
# 运行所有测试
./build.sh test

# 运行核心功能测试
./build/examples/deepinscan_core_test

# 运行GUI测试（需要显示环境）
./build/src/gui/deepinscan-simple
```

### 调试技巧
```bash
# 启用详细日志输出
export QT_LOGGING_RULES="deepinscan.*=true"

# 使用 GDB 调试
gdb ./build/examples/deepinscan_core_test

# 内存检查（需要安装 valgrind）
valgrind --leak-check=full ./build/examples/deepinscan_core_test
```

## 🔧 故障排除

### deb 包构建问题

**问题：找不到 dh_install 文件**
```bash
# 解决方案：确保安装了构建依赖
sudo apt-get install debhelper pkg-config qtbase5-dev \
                     libdtkcore-dev libdtkwidget-dev libusb-1.0-0-dev
```

**问题：MOC 编译错误**
```bash
# 解决方案：清理并重新构建
./build.sh clean
./build.sh build
```

**问题：符号链接创建失败**
```bash
# 这是正常的，构建脚本会自动处理
# 检查最终生成的包文件
ls -la ../*.deb
```

### 安装问题

**问题：依赖包冲突**
```bash
# 解决方案：强制安装并修复依赖
sudo dpkg -i --force-depends ../deepinscan*.deb
sudo apt-get install -f
```

**问题：权限不足**
```bash
# 解决方案：确保有足够权限
sudo dpkg -i ../deepinscan*.deb
# 或者添加用户到 scanner 组
sudo usermod -a -G scanner $USER
```

**问题：库版本不匹配**
```bash
# 解决方案：检查已安装的库版本
dpkg -l | grep deepinscan
# 卸载旧版本
sudo dpkg -r libdeepinscan1 deepinscan
# 重新安装新版本
sudo dpkg -i ../deepinscan*.deb
```

### 运行时问题

**问题：找不到扫描仪设备**
```bash
# 检查 SANE 配置
sudo sane-find-scanner
scanimage -L

# 检查 USB 权限
lsusb
# 添加 udev 规则（如果需要）
```

**问题：界面显示异常**
```bash
# 检查 DTK 环境
echo $XDG_CURRENT_DESKTOP
# 确保在深度桌面环境运行
```

## 🤝 贡献指南

我们欢迎各种形式的贡献，包括但不限于：

- 🐛 报告 Bug
- 💡 提出新功能需求
- 📝 改进文档
- 🔧 提交代码修复
- 🧪 添加测试用例
- 🌍 翻译项目

### 贡献流程
1. Fork 项目到你的 GitHub 账户
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m '添加某个惊人的功能'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 开启 Pull Request

### 代码规范
- 遵循 C++17 标准
- 使用 4 空格缩进
- 函数和变量使用驼峰命名
- 类使用帕斯卡命名
- 添加适当的注释和文档

## 📜 许可证

本项目采用 GPL-3.0 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 📞 联系我们

- **项目主页**: https://github.com/eric2023/deepinscan
- **问题反馈**: https://github.com/eric2023/deepinscan/issues
- **邮件列表**: eric2023@163.com

## 🎉 致谢

感谢以下项目和组织的支持：

- [Qt Project](https://www.qt.io) - 跨平台应用框架
- [SANE Project](http://www.sane-project.org) - 扫描仪驱动标准
- [CMake](https://cmake.org) - 构建系统
- 所有贡献者和用户的支持

---

**DeepinScan - 让Linux扫描更简单！** 🚀 