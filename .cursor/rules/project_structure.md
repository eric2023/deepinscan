# DeepinScan 项目目录结构规则

## 项目架构概述

DeepinScan 采用五层架构设计，每个目录对应特定的功能模块和职责：

```
deepinscan/
├── include/Scanner/          # 公共头文件 - 接口层
├── src/                      # 源代码实现
│   ├── core/                # 核心模块 - 管理器和设备抽象
│   ├── drivers/             # 驱动层 - 厂商驱动实现
│   ├── communication/       # 硬件抽象层 - 通信接口
│   ├── processing/          # 图像处理层 - 数据处理
│   └── gui/                 # 应用层 - 图形界面
├── tests/                   # 测试代码
├── examples/                # 示例程序
├── docs/                    # 技术文档
├── data/                    # 数据文件
├── resources/               # 资源文件
├── misc/                    # 杂项文件
├── debian/                  # Debian打包文件
└── .task/                   # 任务和报告文件
```

## 目录职责说明

### include/Scanner/ - 公共头文件
**用途**: 对外提供的API接口定义
**文件类型**: `.h` 头文件
**命名规范**: `DScanner*.h`
**职责**:
- 定义公共接口和抽象类
- 提供类型定义和枚举
- 异常类定义
- 全局配置和宏定义

### src/core/ - 核心模块
**用途**: 核心业务逻辑和设备管理
**主要类**: DScannerManager, DScannerDevice, DScannerException
**职责**:
- 设备生命周期管理
- 扫描会话控制
- 异常处理机制
- 全局配置管理

### src/drivers/ - 驱动层
**用途**: 厂商特定驱动实现
**子目录结构**:
```
drivers/
├── sane/                    # SANE协议驱动
├── genesys/                 # Genesys芯片驱动
└── vendors/                 # 厂商特定驱动
    ├── canon/               # Canon驱动
    ├── epson/               # Epson驱动
    └── genesys/             # Genesys完整驱动
```
**职责**:
- 实现具体的扫描仪驱动
- 硬件特定的参数控制
- 厂商协议适配

### src/communication/ - 硬件抽象层
**用途**: 硬件通信接口实现
**子目录结构**:
```
communication/
├── usb/                     # USB通信
└── network/                 # 网络通信
```
**职责**:
- USB设备通信
- 网络设备发现和连接
- 协议适配和数据传输

### src/processing/ - 图像处理层
**用途**: 图像处理算法和优化
**主要功能**:
- 图像格式转换
- 色彩校正和增强
- 性能优化算法
- 批处理支持

### src/gui/ - 应用层
**用途**: DTK图形界面实现
**子目录结构**:
```
gui/
├── dialogs/                 # 对话框
├── widgets/                 # 自定义控件
├── resources/               # 界面资源
└── utils/                   # 界面工具
```

### tests/ - 测试代码
**用途**: 单元测试和集成测试
**文件命名**: `test_*.cpp`
**职责**:
- 功能验证测试
- 性能基准测试
- 集成测试套件

### examples/ - 示例程序
**用途**: API使用示例和演示程序
**文件命名**: `*_example.cpp`

### docs/ - 技术文档
**用途**: 项目文档和开发指南
**文件类型**: `.md` 和相关文档

### data/ - 数据文件
**用途**: 配置数据和设备数据库
**文件类型**: `.json`, `.xml` 等数据文件

### resources/ - 资源文件
**用途**: Qt资源文件
**包含**: 图标、主题、翻译文件

### misc/ - 杂项文件
**用途**: 项目配置和元数据
**包含**: pkg-config模板等

### debian/ - Debian打包文件
**用途**: Linux发行版打包支持
**包含**: control, rules, changelog等

### .task/ - 任务和报告文件
**用途**: 开发任务、验证报告、完成记录
**文件类型**: `.md` 报告文件

## 文件组织原则

### 1. 模块化原则
- 每个目录职责单一
- 相关功能聚合在同一目录
- 避免跨目录的紧耦合

### 2. 分层原则
- 上层依赖下层，下层不依赖上层
- 接口与实现分离
- 抽象与具体分离

### 3. 扩展性原则
- 新厂商驱动加入 `src/drivers/vendors/`
- 新通信方式加入 `src/communication/`
- 新处理算法加入 `src/processing/`

### 4. 维护性原则
- 测试代码与源码目录对应
- 文档与代码同步维护
- 示例代码简洁易懂

## 新文件放置指南

| 文件类型 | 放置目录 | 命名规范 |
|---------|---------|----------|
| 公共头文件 | `include/Scanner/` | `DScanner*.h` |
| 核心业务逻辑 | `src/core/` | `dscanner*.cpp/h` |
| 厂商驱动 | `src/drivers/vendors/{厂商}/` | `{厂商}_driver*.cpp/h` |
| 通信接口 | `src/communication/{类型}/` | `dscanner{类型}*.cpp/h` |
| 图像处理 | `src/processing/` | `*_processor.cpp/h` |
| GUI组件 | `src/gui/{类型}/` | `{功能}widget.cpp/h` |
| 测试文件 | `tests/` | `test_*.cpp` |
| 示例程序 | `examples/` | `*_example.cpp` |
| 技术文档 | `docs/` | `*.md` |
| 任务报告 | `.task/` | `*.md` |

---

*此规则文件定义了DeepinScan项目的目录结构标准，所有新增文件都应遵循此规范。* 