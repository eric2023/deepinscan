// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERGENESYS_H
#define DSCANNERGENESYS_H

#include "DScannerGlobal.h"
#include "DScannerTypes.h"
#include "DScannerDriver.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief Genesys芯片组类型枚举
 * 基于技术研究发现的芯片组类型
 */
enum class GenesysChipset {
    Unknown = 0,            ///< 未知芯片组
    GL646,                  ///< GL646芯片组
    GL841,                  ///< GL841芯片组
    GL842,                  ///< GL842芯片组
    GL843,                  ///< GL843芯片组
    GL846,                  ///< GL846芯片组
    GL847,                  ///< GL847芯片组
    GL124                   ///< GL124芯片组
};

/**
 * @brief 扫描方法枚举
 */
enum class ScanMethod {
    Flatbed = 0,            ///< 平板扫描
    ADF,                    ///< 自动进纸器
    ADFDuplex               ///< 双面自动进纸器
};

/**
 * @brief 传感器类型枚举
 */
enum class SensorType {
    Unknown = 0,            ///< 未知传感器
    CCD,                    ///< CCD传感器
    CIS,                    ///< CIS传感器
    Contact                 ///< 接触式传感器
};

/**
 * @brief Genesys设备模型信息
 */
struct GenesysModel {
    QString name;                   ///< 设备名称
    QString vendor;                 ///< 厂商名称
    QString model;                  ///< 型号
    GenesysChipset chipset;         ///< 芯片组类型
    
    // USB设备信息
    quint16 vendorId;               ///< USB厂商ID
    quint16 productId;              ///< USB产品ID
    
    // 扫描能力
    int maxResolution;              ///< 最大分辨率
    int minResolution;              ///< 最小分辨率
    QList<ColorMode> colorModes;    ///< 支持的颜色模式
    ScanArea maxScanArea;           ///< 最大扫描区域
    
    // 传感器信息
    SensorType sensorType;          ///< 传感器类型
    int sensorPixels;               ///< 传感器像素数
    double sensorDpi;               ///< 传感器DPI
    
    // 特性标志
    bool hasADF;                    ///< 是否有自动进纸器
    bool hasDuplex;                 ///< 是否支持双面扫描
    bool hasCalibration;            ///< 是否支持校准
    bool hasLamp;                   ///< 是否有灯管
    
    GenesysModel() 
        : chipset(GenesysChipset::Unknown)
        , vendorId(0)
        , productId(0)
        , maxResolution(1200)
        , minResolution(75)
        , sensorType(SensorType::Unknown)
        , sensorPixels(0)
        , sensorDpi(0.0)
        , hasADF(false)
        , hasDuplex(false)
        , hasCalibration(false)
        , hasLamp(false)
    {}
};

/**
 * @brief Genesys传感器配置
 */
struct GenesysSensor {
    QString name;                   ///< 传感器名称
    SensorType type;                ///< 传感器类型
    int sensorPixels;               ///< 传感器像素数
    int blackPixels;                ///< 黑色像素数
    int dummyPixel;                 ///< 虚拟像素
    int ccdPixels;                  ///< CCD像素数
    double opticalDpi;              ///< 光学DPI
    
    // 颜色通道偏移
    int redOffset;                  ///< 红色通道偏移
    int greenOffset;                ///< 绿色通道偏移
    int blueOffset;                 ///< 蓝色通道偏移
    
    // 时序参数
    int exposureTime;               ///< 曝光时间
    int ledExposureTime;            ///< LED曝光时间
    
    GenesysSensor()
        : type(SensorType::Unknown)
        , sensorPixels(0)
        , blackPixels(0)
        , dummyPixel(0)
        , ccdPixels(0)
        , opticalDpi(0.0)
        , redOffset(0)
        , greenOffset(0)
        , blueOffset(0)
        , exposureTime(0)
        , ledExposureTime(0)
    {}
};

/**
 * @brief Genesys寄存器定义
 */
namespace GenesysRegisters {
    // 常用寄存器地址
    constexpr int REG_0x01 = 0x01;  ///< 芯片ID寄存器
    constexpr int REG_0x02 = 0x02;  ///< 芯片版本寄存器
    constexpr int REG_0x03 = 0x03;  ///< 控制寄存器
    constexpr int REG_0x04 = 0x04;  ///< 状态寄存器
    constexpr int REG_0x05 = 0x05;  ///< 配置寄存器
    
    // 扫描控制寄存器
    constexpr int REG_SCAN_START = 0x10;    ///< 扫描开始
    constexpr int REG_SCAN_STOP = 0x11;     ///< 扫描停止
    constexpr int REG_SCAN_MODE = 0x12;     ///< 扫描模式
    
    // 传感器控制寄存器
    constexpr int REG_SENSOR_CTRL = 0x20;   ///< 传感器控制
    constexpr int REG_EXPOSURE = 0x21;      ///< 曝光时间
    constexpr int REG_GAIN = 0x22;          ///< 增益控制
    
    // 运动控制寄存器
    constexpr int REG_MOTOR_CTRL = 0x30;    ///< 马达控制
    constexpr int REG_STEP_SIZE = 0x31;     ///< 步进大小
    constexpr int REG_SPEED = 0x32;         ///< 扫描速度
}

/**
 * @brief Genesys扫描仪驱动
 * 
 * 基于对Genesys芯片组的深度技术研究实现的驱动，
 * 支持GL646、GL842、GL843、GL846、GL847等主流芯片组
 */
class DSCANNER_EXPORT DScannerGenesysDriver : public DScannerDriver
{
    Q_OBJECT

public:
    explicit DScannerGenesysDriver(QObject *parent = nullptr);
    virtual ~DScannerGenesysDriver();

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

    ScannerCapabilities getCapabilities() override;
    bool setScanParameters(const ScanParameters &params) override;
    ScanParameters getScanParameters() override;

    bool startScan() override;
    void stopScan() override;
    bool pauseScan() override;
    bool resumeScan() override;

    QImage getPreview() override;
    bool calibrateDevice() override;

    bool setParameter(const QString &name, const QVariant &value) override;
    QVariant getParameter(const QString &name) const override;
    QStringList getParameterNames() const override;

    QString lastError() const override;

    // Genesys特定方法
    /**
     * @brief 获取芯片组类型
     * @return 芯片组类型
     */
    GenesysChipset getChipset() const;

    /**
     * @brief 获取设备模型信息
     * @return 设备模型
     */
    GenesysModel getModel() const;

    /**
     * @brief 获取传感器信息
     * @return 传感器配置
     */
    GenesysSensor getSensor() const;

    /**
     * @brief 读取寄存器
     * @param address 寄存器地址
     * @return 寄存器值
     */
    quint8 readRegister(int address);

    /**
     * @brief 写入寄存器
     * @param address 寄存器地址
     * @param value 寄存器值
     * @return 是否成功
     */
    bool writeRegister(int address, quint8 value);

    /**
     * @brief 批量读取寄存器
     * @param startAddress 起始地址
     * @param count 寄存器数量
     * @return 寄存器值列表
     */
    QByteArray readRegisters(int startAddress, int count);

    /**
     * @brief 批量写入寄存器
     * @param startAddress 起始地址
     * @param values 寄存器值
     * @return 是否成功
     */
    bool writeRegisters(int startAddress, const QByteArray &values);

    /**
     * @brief 执行芯片组初始化
     * @return 是否成功
     */
    bool initializeChipset();

    /**
     * @brief 执行传感器校准
     * @return 是否成功
     */
    bool calibrateSensor();

    /**
     * @brief 设置扫描区域
     * @param area 扫描区域
     * @return 是否成功
     */
    bool setScanArea(const ScanArea &area);

    /**
     * @brief 设置扫描分辨率
     * @param resolution 分辨率
     * @return 是否成功
     */
    bool setScanResolution(int resolution);

    /**
     * @brief 设置颜色模式
     * @param mode 颜色模式
     * @return 是否成功
     */
    bool setColorMode(ColorMode mode);

    /**
     * @brief 获取支持的芯片组列表
     * @return 芯片组列表
     */
    static QList<GenesysChipset> supportedChipsets();

    /**
     * @brief 获取芯片组名称
     * @param chipset 芯片组类型
     * @return 芯片组名称
     */
    static QString chipsetName(GenesysChipset chipset);

    /**
     * @brief 检测芯片组类型
     * @param vendorId USB厂商ID
     * @param productId USB产品ID
     * @return 芯片组类型
     */
    static GenesysChipset detectChipset(quint16 vendorId, quint16 productId);

    /**
     * @brief 获取预定义设备模型
     * @param vendorId USB厂商ID
     * @param productId USB产品ID
     * @return 设备模型，如果未找到返回空模型
     */
    static GenesysModel getDeviceModel(quint16 vendorId, quint16 productId);

signals:
    /**
     * @brief 芯片组状态改变信号
     * @param status 新状态
     */
    void chipsetStatusChanged(const QString &status);

    /**
     * @brief 寄存器访问信号
     * @param address 寄存器地址
     * @param value 寄存器值
     * @param isWrite 是否为写操作
     */
    void registerAccessed(int address, quint8 value, bool isWrite);

    /**
     * @brief 校准进度信号
     * @param progress 进度百分比
     */
    void calibrationProgress(int progress);

private slots:
    void onUSBDataReceived(const QByteArray &data);
    void onUSBError(const QString &error);

private:
    class DScannerGenesysDriverPrivate;
    DScannerGenesysDriverPrivate *d_ptr;
    Q_DECLARE_PRIVATE(DScannerGenesysDriver)
    Q_DISABLE_COPY(DScannerGenesysDriver)
};

DSCANNER_END_NAMESPACE

Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::GenesysChipset)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ScanMethod)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::SensorType)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::GenesysModel)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::GenesysSensor)

#endif // DSCANNERGENESYS_H 