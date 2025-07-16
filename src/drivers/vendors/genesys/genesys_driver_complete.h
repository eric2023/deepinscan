// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GENESYS_DRIVER_COMPLETE_H
#define GENESYS_DRIVER_COMPLETE_H

#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerGlobal.h"

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QDateTime>
#include <QElapsedTimer>
#include <QByteArray>

DSCANNER_BEGIN_NAMESPACE

// 前向声明
class DScannerUSB;

/**
 * @brief Genesys芯片组类型枚举
 */
enum class GenesysChipsetType {
    Unknown,    // 未知类型
    GL646,      // GL646芯片组
    GL843,      // GL843芯片组
    GL846,      // GL846芯片组
    GL847,      // GL847芯片组
    GL124       // GL124芯片组
};

/**
 * @brief Genesys校准数据结构
 */
struct GenesysCalibrationData {
    QByteArray whiteShadingData;    // 白阴影数据
    QByteArray blackShadingData;    // 黑阴影数据
    QByteArray offsetData;          // 偏移数据
    QByteArray gainData;            // 增益数据
    bool isValid;                   // 是否有效
    
    GenesysCalibrationData() : isValid(false) {}
};

/**
 * @brief Genesys驱动设置
 */
struct GenesysDriverSettings {
    bool autoCalibration;           // 自动校准
    int lampWarmupTime;             // 灯管预热时间(ms)
    int motorSpeed;                 // 马达速度
    int defaultResolution;          // 默认分辨率
    int maxResolution;              // 最大分辨率
    int colorDepth;                 // 颜色深度
    bool enableShading;             // 启用阴影校正
    bool enableGammaCorrection;     // 启用伽马校正
};

/**
 * @brief Genesys芯片组完整驱动实现
 * 
 * 基于先进技术研究，实现对Genesys系列芯片组的完整支持
 * 包括GL646、GL843、GL846、GL847等主要芯片型号
 */
class GenesysDriverComplete : public QObject
{
    Q_OBJECT
    
public:
    explicit GenesysDriverComplete(QObject *parent = nullptr);
    ~GenesysDriverComplete();
    
    // 初始化和清理
    bool initialize(const DeviceInfo &deviceInfo, DScannerUSB *usbDevice);
    void cleanup();
    
    // 设备能力
    ScannerCapabilities getCapabilities() const;
    GenesysChipsetType getChipsetType() const;
    bool isInitialized() const { return m_isInitialized; }
    
    // 扫描操作
    bool startScan(const ScanParameters &params);
    bool stopScan();
    QByteArray readScanData(int maxBytes = 65536);
    bool isScanning() const;
    
    // 设备控制
    bool calibrateDevice();
    bool resetDevice();
    bool setLampState(bool enabled);
    bool parkScanHead();
    bool moveToPosition(int position);
    
    // 参数设置
    bool setResolution(int resolution);
    bool setColorMode(ColorMode mode);
    bool setScanArea(const ScanArea &area);
    bool setExposureTime(int microseconds);
    
signals:
    void deviceInitialized(const DeviceInfo &deviceInfo);
    void scanStarted();
    void scanStopped();
    void scanProgress(int percentage);
    void calibrationCompleted();
    void errorOccurred(const QString &error);
    
private slots:
    void checkDeviceStatus();
    
private:
    // 芯片组检测和初始化
    bool detectChipsetType();
    bool initializeChipset();
    bool initializeGL646();
    bool initializeGL843();
    bool initializeGL846();
    bool initializeGL847();
    
    // 寄存器操作
    bool readRegister(uint8_t reg, uint8_t &value);
    bool writeRegister(uint8_t reg, uint16_t value);
    bool sendCommand(uint8_t command);
    bool waitForReady(int timeoutMs = 3000);
    
    // 马达控制
    bool setupGL646Motor();
    bool setupGL843Motor();
    bool setupGL846Motor();
    bool setupGL847Motor();
    bool startMotor();
    bool stopMotor();
    
    // 扫描控制
    bool validateScanParameters(const ScanParameters &params);
    bool prepareScan();
    bool configureScanParameters();
    bool startPhysicalScan();
    void waitForLampWarmup();
    bool performPreScanCalibration();
    
    // 数据处理
    QByteArray readRawScanData(int maxBytes);
    QByteArray processScanData(const QByteArray &rawData);
    void updateScanProgress();
    
    // 校准功能
    bool loadCalibrationData();
    void createDefaultCalibration();
    bool performWhiteCalibration();
    bool performBlackCalibration();
    bool applyShadingCorrection(QByteArray &data);
    
    // 芯片组特定功能
    bool detectGL846Features();
    bool setupGL846ADC();
    bool setupGL847LED();
    void resetChipset();
    
    // 能力查询
    ScannerCapabilities getGL646Capabilities() const;
    ScannerCapabilities getGL843Capabilities() const;
    ScannerCapabilities getGL846Capabilities() const;
    ScannerCapabilities getGL847Capabilities() const;
    ScannerCapabilities getGenericCapabilities() const;
    
    // 工具函数
    void initializeDefaultSettings();
    uint16_t calculateExposureTime(int resolution, ColorMode colorMode);
    int calculateMotorSpeed(int resolution);
    bool isValidResolution(int resolution) const;
    
    // 成员变量
    GenesysChipsetType m_chipsetType;
    DeviceInfo m_deviceInfo;
    void *m_deviceHandle;
    DScannerUSB *m_usbDevice;
    
    // 状态管理
    bool m_isInitialized;
    bool m_isScanning;
    mutable QMutex m_deviceMutex;
    QTimer *m_statusTimer;
    
    // 扫描状态
    ScanParameters m_currentScanParams;
    QDateTime m_scanStartTime;
    qint64 m_totalBytesScanned;
    int m_scanProgress;
    
    // 校准和设置
    GenesysCalibrationData m_calibrationData;
    GenesysDriverSettings m_currentSettings;
    
    // 数据缓冲
    QByteArray m_scanBuffer;
    int m_bufferPosition;
};

DSCANNER_END_NAMESPACE

#endif // GENESYS_DRIVER_COMPLETE_H 