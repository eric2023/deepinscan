# DeepinScan 文件组织规范

## 命名规范

### 1. 类名命名
```cpp
// 公共接口类 - 使用 DScanner 前缀
class DScannerManager;
class DScannerDevice;
class DScannerDriver;

// 私有实现类 - 使用 Private 后缀
class DScannerManagerPrivate;
class DScannerDevicePrivate;

// 异常类 - 使用 Exception 后缀
class DScannerException;
class DScannerConnectionException;
```

### 2. 文件命名
```
头文件:        DScanner*.h          (公共接口)
源文件:        dscanner*.cpp        (实现文件)
私有头文件:    dscanner*_p.h        (私有实现)
测试文件:      test_*.cpp           (测试代码)
示例文件:      *_example.cpp        (示例程序)
```

### 3. 目录命名
- 使用小写字母和下划线
- 功能相关的文件归类到同一目录
- 避免过深的目录层次（不超过4层）

## 头文件组织

### 1. 包含顺序
```cpp
// 1. 对应的头文件（对于.cpp文件）
#include "dscannerdevice.h"

// 2. C++标准库
#include <memory>
#include <vector>
#include <string>

// 3. Qt框架
#include <QObject>
#include <QString>
#include <QThread>

// 4. DTK框架
#include <DWidget>
#include <DMainWindow>

// 5. 第三方库
#include <libusb.h>
#include <sane/sane.h>

// 6. 项目内部头文件
#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerException.h"
```

### 2. 头文件保护
```cpp
#ifndef DSCANNERDEVICE_H
#define DSCANNERDEVICE_H

// 头文件内容

#endif // DSCANNERDEVICE_H
```

### 3. 前置声明
```cpp
// 优先使用前置声明
class DScannerManager;
class QTimer;

// 避免不必要的包含
// #include "dscannerManager.h"  // 不推荐
```

## 源文件组织

### 1. 文件结构
```cpp
// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dscannerdevice.h"
#include "dscannerdevice_p.h"

// 匿名命名空间用于内部函数
namespace {
    // 内部辅助函数
    bool isValidDevice(const QString &deviceId) {
        return !deviceId.isEmpty();
    }
}

DSCANNER_USE_NAMESPACE

// 类实现
DScannerDevice::DScannerDevice(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerDevicePrivate(this))
{
    // 构造函数实现
}
```

### 2. 私有实现模式
```cpp
// dscannerdevice_p.h
class DScannerDevicePrivate
{
public:
    explicit DScannerDevicePrivate(DScannerDevice *q);
    
    DScannerDevice *q_ptr;
    
    // 私有成员变量
    QString deviceId;
    bool isConnected;
    
    // 私有成员函数
    void initializeDevice();
    void cleanupResources();
    
    Q_DECLARE_PUBLIC(DScannerDevice)
};
```

## 代码组织规范

### 1. 类成员顺序
```cpp
class DScannerDevice : public QObject
{
    Q_OBJECT
    
public:
    // 1. 构造函数和析构函数
    explicit DScannerDevice(QObject *parent = nullptr);
    ~DScannerDevice();
    
    // 2. 公共成员函数
    bool openDevice(const QString &deviceId);
    void closeDevice();
    
    // 3. 静态函数
    static QStringList availableDevices();
    
public slots:
    // 4. 公共槽函数
    void startScan();
    void stopScan();
    
signals:
    // 5. 信号
    void deviceOpened();
    void scanCompleted(const QImage &image);
    
protected:
    // 6. 保护成员函数
    virtual void handleScanData(const QByteArray &data);
    
private slots:
    // 7. 私有槽函数
    void onDataReceived();
    
private:
    // 8. 私有成员函数
    void initializeConnection();
    
    // 9. 私有成员变量
    Q_DECLARE_PRIVATE(DScannerDevice)
    QScopedPointer<DScannerDevicePrivate> d_ptr;
};
```

### 2. 函数实现组织
```cpp
// 按照头文件声明顺序实现函数
DScannerDevice::DScannerDevice(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerDevicePrivate(this))
{
    Q_D(DScannerDevice);
    d->initializeDevice();
}

bool DScannerDevice::openDevice(const QString &deviceId)
{
    Q_D(DScannerDevice);
    
    // 参数验证
    if (deviceId.isEmpty()) {
        qWarning() << "Device ID cannot be empty";
        return false;
    }
    
    // 核心逻辑
    d->deviceId = deviceId;
    d->isConnected = true;
    
    // 信号发射
    emit deviceOpened();
    return true;
}
```

## 注释规范

### 1. 类注释
```cpp
/**
 * @brief 扫描仪设备抽象类
 * 
 * DScannerDevice 提供了扫描仪设备的统一接口，支持多种扫描仪类型。
 * 该类负责设备的连接、配置和扫描操作。
 * 
 * @since 1.0.0
 * @author DeepinScan Team
 */
class DScannerDevice : public QObject
```

### 2. 函数注释
```cpp
/**
 * @brief 打开指定的扫描仪设备
 * 
 * 根据设备ID连接到对应的扫描仪设备。连接成功后会发射 deviceOpened() 信号。
 * 
 * @param deviceId 设备唯一标识符
 * @return 成功返回 true，失败返回 false
 * 
 * @note 在调用此函数前，确保设备ID有效且设备可访问
 * @see closeDevice(), availableDevices()
 */
bool openDevice(const QString &deviceId);
```

### 3. 日志规范
```cpp
// 使用Qt日志系统，添加模块前缀
qDebug() << "[DScannerDevice]" << "Opening device:" << deviceId;
qInfo()  << "[DScannerDevice]" << "Device opened successfully";
qWarning() << "[DScannerDevice]" << "Failed to open device:" << deviceId;
qCritical() << "[DScannerDevice]" << "Critical error occurred";
```

## CMake组织规范

### 1. 模块化CMakeLists.txt
```cmake
# src/core/CMakeLists.txt
set(CORE_SOURCES
    dscannerdevice.cpp
    dscannerdevice_p.cpp
    dscannermanager.cpp
    dscannerexception.cpp
)

set(CORE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Scanner/DScannerDevice.h
    ${CMAKE_SOURCE_DIR}/include/Scanner/DScannerManager.h
    ${CMAKE_SOURCE_DIR}/include/Scanner/DScannerException.h
)

target_sources(deepinscan PRIVATE
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)
```

### 2. 测试文件组织
```cmake
# tests/CMakeLists.txt
add_executable(test_device_discovery test_device_discovery.cpp)
target_link_libraries(test_device_discovery 
    deepinscan
    Qt5::Test
    Qt5::Core
)

add_test(NAME DeviceDiscoveryTest COMMAND test_device_discovery)
```

## 资源文件组织

### 1. Qt资源文件
```xml
<!-- resources/resources.qrc -->
<RCC>
    <qresource prefix="/icons">
        <file>icons/scanner.png</file>
        <file>icons/scan.png</file>
    </qresource>
    <qresource prefix="/themes">
        <file>themes/light.qss</file>
        <file>themes/dark.qss</file>
    </qresource>
    <qresource prefix="/translations">
        <file>translations/deepinscan_zh_CN.qm</file>
        <file>translations/deepinscan_en_US.qm</file>
    </qresource>
</RCC>
```

### 2. 数据文件组织
```
data/
├── device_database.json     # 设备数据库
├── presets/                 # 扫描预设
│   ├── document.json
│   ├── photo.json
│   └── custom.json
└── calibration/             # 校准数据
    ├── color_profiles/
    └── gamma_tables/
```

## 版本控制规范

### 1. Git忽略规则
```gitignore
# 构建产物
build/
*.o
*.so
*.a

# IDE文件
.vscode/
.idea/
*.user

# 临时文件
.task/
test_results/
```

### 2. 提交信息规范
```
格式: <type>(<scope>): <subject>

类型:
- feat: 新功能
- fix: 修复bug
- docs: 文档更新
- style: 代码格式化
- refactor: 重构
- test: 测试相关
- build: 构建系统

示例:
feat(drivers): add Canon PIXMA driver support
fix(usb): resolve device enumeration issue
docs(api): update scanner device documentation
```

---

*此规范确保DeepinScan项目代码的一致性、可读性和可维护性。所有开发者都应遵循这些规范。* 