// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERSANE_H
#define DSCANNERSANE_H

#include "DScannerGlobal.h"
#include "DScannerTypes.h"
#include "DScannerDriver.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>
#include <QLibrary>

DSCANNER_BEGIN_NAMESPACE

// 前向声明
class DScannerSANEPrivate;
class DScannerSANEDriverPrivate;

// SANE常量定义
enum class SANEStatus {
    Good = 0,               ///< 操作成功
    Unsupported,            ///< 不支持的操作
    Cancelled,              ///< 操作被取消
    DeviceBusy,             ///< 设备忙
    Invalid,                ///< 无效参数
    EOF_,                   ///< 数据结束
    Jammed,                 ///< 设备卡纸
    NoDocs,                 ///< 没有文档
    CoverOpen,              ///< 盖子打开
    IOError,                ///< I/O错误
    NoMem,                  ///< 内存不足
    AccessDenied            ///< 访问被拒绝
};

enum class SANEAction {
    GetValue = 0,           ///< 获取值
    SetValue,               ///< 设置值
    SetAuto                 ///< 自动设置
};

enum class SANEValueType {
    Bool = 0,               ///< 布尔值
    Int,                    ///< 整数
    Fixed,                  ///< 定点数
    String,                 ///< 字符串
    Button,                 ///< 按钮
    Group                   ///< 组
};

enum class SANEUnit {
    None = 0,               ///< 无单位
    Pixel,                  ///< 像素
    Bit,                    ///< 位
    MM,                     ///< 毫米
    DPI,                    ///< DPI
    Percent,                ///< 百分比
    Microsecond             ///< 微秒
};

enum class SANEConstraintType {
    None = 0,               ///< 无约束
    Range,                  ///< 范围约束
    WordList,               ///< 词列表约束
    StringList              ///< 字符串列表约束
};

// SANE设备结构
struct SANEDevice {
    QString name;           ///< 设备名称
    QString vendor;         ///< 厂商
    QString model;          ///< 型号
    QString type;           ///< 类型
};

// SANE选项描述符
struct SANEOptionDescriptor {
    QString name;                   ///< 选项名称
    QString title;                  ///< 显示标题
    QString description;            ///< 描述
    SANEValueType type;             ///< 值类型
    SANEUnit unit;                  ///< 单位
    int size;                       ///< 大小
    int cap;                        ///< 能力标志
    SANEConstraintType constraintType; ///< 约束类型
    QVariant constraint;            ///< 约束值
};

// SANE扫描参数
struct SANEParameters {
    int format;                     ///< 图像格式
    bool lastFrame;                 ///< 是否最后一帧
    int bytesPerLine;               ///< 每行字节数
    int pixelsPerLine;              ///< 每行像素数
    int lines;                      ///< 行数
    int depth;                      ///< 位深度
};

/**
 * @brief SANE后端适配器接口
 * 
 * 提供对SANE (Scanner Access Now Easy) 协议的支持，
 * 基于现代化的内置SANE实现设计
 */
class DSCANNER_EXPORT DScannerSANE : public QObject
{
    Q_OBJECT

public:
    explicit DScannerSANE(QObject *parent = nullptr);
    virtual ~DScannerSANE();

    // SANE初始化和清理
    /**
     * @brief 初始化SANE子系统
     * @param versionCode 返回SANE版本代码
     * @return 是否初始化成功
     */
    bool init(int *versionCode = nullptr);

    /**
     * @brief 清理SANE子系统
     */
    void exit();

    /**
     * @brief 检查SANE是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const;

    // 设备发现和管理
    /**
     * @brief 获取SANE设备列表
     * @param localOnly 是否仅本地设备
     * @return 设备列表
     */
    QList<SANEDevice> getDevices(bool localOnly = false);

    /**
     * @brief 打开SANE设备
     * @param deviceName 设备名称
     * @return 设备句柄，失败返回nullptr
     */
    void* openDevice(const QString &deviceName);

    /**
     * @brief 关闭SANE设备
     * @param handle 设备句柄
     */
    void closeDevice(void *handle);

    // 选项管理
    /**
     * @brief 获取选项描述符
     * @param handle 设备句柄
     * @param option 选项索引
     * @return 选项描述符
     */
    SANEOptionDescriptor getOptionDescriptor(void *handle, int option);

    /**
     * @brief 控制选项
     * @param handle 设备句柄
     * @param option 选项索引
     * @param action 操作类型
     * @param value 值
     * @return SANE状态
     */
    SANEStatus controlOption(void *handle, int option, SANEAction action, QVariant &value);

    // 扫描操作
    /**
     * @brief 获取扫描参数
     * @param handle 设备句柄
     * @return 扫描参数
     */
    SANEParameters getParameters(void *handle);

    /**
     * @brief 开始扫描
     * @param handle 设备句柄
     * @return SANE状态
     */
    SANEStatus startScan(void *handle);

    /**
     * @brief 读取扫描数据
     * @param handle 设备句柄
     * @param buffer 数据缓冲区
     * @param maxLength 最大长度
     * @param length 实际读取长度
     * @return SANE状态
     */
    SANEStatus readData(void *handle, unsigned char *buffer, int maxLength, int *length);

    /**
     * @brief 取消扫描
     * @param handle 设备句柄
     */
    void cancelScan(void *handle);

    // 高级功能
    /**
     * @brief 设置IO模式
     * @param handle 设备句柄
     * @param nonBlocking 是否非阻塞
     * @return SANE状态
     */
    SANEStatus setIOMode(void *handle, bool nonBlocking);

    /**
     * @brief 获取选择文件描述符
     * @param handle 设备句柄
     * @param fd 文件描述符
     * @return SANE状态
     */
    SANEStatus getSelectFd(void *handle, int *fd);

    // 实用功能
    /**
     * @brief 状态码转字符串
     * @param status SANE状态
     * @return 状态描述
     */
    static QString statusToString(SANEStatus status);

    /**
     * @brief 将DeviceInfo转换为SANEDevice
     * @param deviceInfo 设备信息
     * @return SANE设备
     */
    static SANEDevice deviceInfoToSANE(const DeviceInfo &deviceInfo);

    /**
     * @brief 将SANEDevice转换为DeviceInfo
     * @param saneDevice SANE设备
     * @return 设备信息
     */
    static DeviceInfo saneToDeviceInfo(const SANEDevice &saneDevice);

signals:
    /**
     * @brief SANE状态改变信号
     * @param status 新状态
     */
    void statusChanged(SANEStatus status);

    /**
     * @brief 设备发现信号
     * @param device 发现的设备
     */
    void deviceDiscovered(const SANEDevice &device);

    /**
     * @brief 错误发生信号
     * @param status 错误状态
     * @param message 错误消息
     */
    void errorOccurred(SANEStatus status, const QString &message);

private:
    DScannerSANEPrivate *d_ptr;
    Q_DECLARE_PRIVATE(DScannerSANE)
    Q_DISABLE_COPY(DScannerSANE)
};

/**
 * @brief SANE后端驱动实现
 * 
 * 基于DScannerDriver接口实现的SANE后端驱动，
 * 提供与现有驱动架构的集成
 */
class DSCANNER_EXPORT DScannerSANEDriver : public DScannerDriver
{
    Q_OBJECT

public:
    explicit DScannerSANEDriver(QObject *parent = nullptr);
    virtual ~DScannerSANEDriver();

    // DScannerDriver接口实现
    QString driverName() const override;
    QString driverVersion() const override;
    DriverType driverType() const override;
    QStringList supportedDevices() const override;

    bool initialize() override;
    void shutdown() override;

    QList<DeviceInfo> discoverDevices() override;
    bool isDeviceSupported(const DeviceInfo &deviceInfo) override;

    bool openDevice(const QString &deviceId) override;
    void closeDevice() override;
    bool isDeviceOpen() const override;

    ScannerCapabilities getCapabilities() const override;
    bool setScanParameters(const ScanParameters &params) override;
    ScanParameters getScanParameters() const override;

    bool startScan() override;
    bool startScan(const ScanParameters &params) override;
    void stopScan() override;
    void cancelScan() override;
    QByteArray readScanData() override;
    void cleanup() override;
    bool pauseScan() override;
    bool resumeScan() override;

    QImage getPreview() override;
    bool calibrateDevice() override;

    bool setParameter(const QString &name, const QVariant &value) override;
    QVariant getParameter(const QString &name) const override;
    QStringList getParameterNames() const override;

    QString lastError() const override;

private slots:
    void onSANEStatusChanged(SANEStatus status);
    void onSANEDeviceDiscovered(const SANEDevice &device);
    void onSANEErrorOccurred(SANEStatus status, const QString &message);

private:
    DScannerSANEDriverPrivate *d_ptr;
    Q_DECLARE_PRIVATE(DScannerSANEDriver)
    Q_DISABLE_COPY(DScannerSANEDriver)
};

DSCANNER_END_NAMESPACE

Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEStatus)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEAction)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEValueType)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEUnit)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEConstraintType)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEDevice)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEOptionDescriptor)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SANEParameters)

#endif // DSCANNERSANE_H 