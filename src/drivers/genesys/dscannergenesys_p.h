// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERGENESYS_P_H
#define DSCANNERGENESYS_P_H

#include "Scanner/DScannerGenesys.h"
#include "Scanner/DScannerUSB.h"

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QHash>

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief Genesys寄存器操作结果
 */
struct RegisterOperationResult {
    bool success;               ///< 操作是否成功
    QByteArray data;            ///< 读取的数据
    QString errorMessage;       ///< 错误信息
    
    RegisterOperationResult(bool s = false) : success(s) {}
};

/**
 * @brief DScannerGenesysDriver的私有实现类
 */
class DScannerGenesysDriverPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(DScannerGenesysDriver)

public:
    explicit DScannerGenesysDriverPrivate(DScannerGenesysDriver *q);
    virtual ~DScannerGenesysDriverPrivate();

    // 初始化和清理
    bool initializeUSB();
    void shutdownUSB();
    bool loadDeviceDatabase();

    // 设备管理
    bool openUSBDevice(const QString &deviceId);
    void closeUSBDevice();
    bool initializeDevice();
    void shutdownDevice();

    // 设备信息转换
    DeviceInfo convertToDeviceInfo(const USBDeviceDescriptor &usbDevice, const GenesysModel &model);
    QStringList getSupportedDeviceNames() const;

    // 扫描能力和参数
    ScannerCapabilities buildCapabilities();
    bool applyScanParameters(const ScanParameters &params);

    // 扫描操作
    bool startScanning();
    void stopScanning();
    QImage performPreviewScan();
    bool performCalibration();

    // 设备参数管理
    bool setDeviceParameter(const QString &name, const QVariant &value);
    QVariant getDeviceParameter(const QString &name) const;
    QStringList getDeviceParameterNames() const;

    // 寄存器操作
    quint8 readRegister(int address);
    bool writeRegister(int address, quint8 value);
    QByteArray readRegisters(int startAddress, int count);
    bool writeRegisters(int startAddress, const QByteArray &values);

    // 芯片组特定操作
    bool initializeChipset();
    bool calibrateSensor();
    bool setScanArea(const ScanArea &area);
    bool setScanResolution(int resolution);
    bool setColorMode(ColorMode mode);

    // USB数据处理
    void handleUSBData(const QByteArray &data);

    // 芯片组特定实现
    bool initializeGL646();
    bool initializeGL841();
    bool initializeGL842();
    bool initializeGL843();
    bool initializeGL846();
    bool initializeGL847();

    // 传感器操作
    bool configureSensor();
    bool startSensorCalibration();
    bool readSensorData(QByteArray &data);

    // 马达控制
    bool initializeMotor();
    bool moveToPosition(int position);
    bool setMotorSpeed(int speed);

    // 灯管控制
    bool turnOnLamp();
    void turnOffLamp();
    bool isLampReady();

public:
    DScannerGenesysDriver *q_ptr;

    // 状态管理
    bool initialized;
    QString lastError;

    // USB通信
    DScannerUSB *usbComm;
    void *currentDevice; // USB设备句柄的抽象指针

    // 设备信息
    QString currentDeviceId;
    GenesysModel currentModel;
    GenesysSensor currentSensor;
    ScanParameters currentScanParams;
    bool isScanning;

    // 设备数据库
    QHash<QPair<quint16, quint16>, GenesysModel> deviceDatabase;

    // 缓存和状态
    QHash<int, quint8> registerCache;
    bool registerCacheEnabled;
    mutable QMutex deviceMutex;

    // 校准数据
    QByteArray whiteCalibrationData;
    QByteArray blackCalibrationData;
    bool isCalibrated;

    // 扫描状态
    ScanArea currentScanArea;
    int currentResolution;
    ColorMode currentColorMode;

private slots:
    void onUSBDeviceConnected(const USBDeviceDescriptor &descriptor);
    void onUSBDeviceDisconnected(const QString &devicePath);
    void onUSBError(int errorCode, const QString &errorMessage);

private:
    // 工具方法
    RegisterOperationResult performRegisterOperation(int address, bool isWrite = false, 
                                                   quint8 value = 0, int count = 1);
    bool validateScanParameters(const ScanParameters &params);
    GenesysSensor getDefaultSensor(GenesysChipset chipset);
    
    // 禁用拷贝
    Q_DISABLE_COPY(DScannerGenesysDriverPrivate)
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERGENESYS_P_H 