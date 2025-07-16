// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "genesys_driver_complete.h"
#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerUSB.h"

#include <QLoggingCategory>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QByteArray>

#include <algorithm>
#include <cmath>
#include <cstring>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerGenesysComplete, "deepinscan.genesys.complete")

// Genesys芯片组寄存器定义（基于技术研究）
namespace GenesysRegisters {
    // 通用寄存器
    constexpr uint8_t REG_CHIPID           = 0x00;  // 芯片ID寄存器
    constexpr uint8_t REG_STATUS           = 0x01;  // 状态寄存器
    constexpr uint8_t REG_CONTROL          = 0x02;  // 控制寄存器
    constexpr uint8_t REG_SCAN_CONTROL     = 0x03;  // 扫描控制
    constexpr uint8_t REG_MOTOR_CONTROL    = 0x04;  // 马达控制
    constexpr uint8_t REG_LAMP_CONTROL     = 0x05;  // 灯管控制
    
    // GL646特定寄存器
    constexpr uint8_t GL646_REG_EXPOSURE   = 0x10;  // 曝光时间
    constexpr uint8_t GL646_REG_PIXELSTART = 0x11;  // 像素起始位置
    constexpr uint8_t GL646_REG_PIXELEND   = 0x12;  // 像素结束位置
    
    // GL843特定寄存器
    constexpr uint8_t GL843_REG_RESOLUTION = 0x20;  // 分辨率设置
    constexpr uint8_t GL843_REG_SCAN_MODE  = 0x21;  // 扫描模式
    constexpr uint8_t GL843_REG_COLOR_MODE = 0x22;  // 颜色模式
    
    // GL846特定寄存器
    constexpr uint8_t GL846_REG_ADC_CONTROL= 0x30;  // ADC控制
    constexpr uint8_t GL846_REG_CCD_CONTROL= 0x31;  // CCD控制
    constexpr uint8_t GL846_REG_GAMMA      = 0x32;  // 伽马校正
    
    // GL847特定寄存器
    constexpr uint8_t GL847_REG_SHADING    = 0x40;  // 阴影校正
    constexpr uint8_t GL847_REG_CALIBRATION= 0x41;  // 校准控制
    constexpr uint8_t GL847_REG_LED_CONTROL= 0x42;  // LED控制
}

// Genesys命令定义
namespace GenesysCommands {
    constexpr uint8_t CMD_INIT             = 0x01;  // 初始化
    constexpr uint8_t CMD_START_SCAN       = 0x02;  // 开始扫描
    constexpr uint8_t CMD_STOP_SCAN        = 0x03;  // 停止扫描
    constexpr uint8_t CMD_CALIBRATE        = 0x04;  // 校准
    constexpr uint8_t CMD_PARK_HEAD        = 0x05;  // 停放扫描头
    constexpr uint8_t CMD_MOVE_HEAD        = 0x06;  // 移动扫描头
    constexpr uint8_t CMD_READ_DATA        = 0x07;  // 读取数据
    constexpr uint8_t CMD_WRITE_REG        = 0x08;  // 写入寄存器
    constexpr uint8_t CMD_READ_REG         = 0x09;  // 读取寄存器
}

// GenesysDriverComplete实现
GenesysDriverComplete::GenesysDriverComplete(QObject *parent)
    : QObject(parent)
    , m_chipsetType(GenesysChipsetType::Unknown)
    , m_deviceHandle(nullptr)
    , m_usbDevice(nullptr)
    , m_isInitialized(false)
    , m_isScanning(false)
    , m_calibrationData()
    , m_currentSettings()
    , m_scanBuffer()
    , m_statusTimer(new QTimer(this))
{
    qCDebug(dscannerGenesysComplete) << "GenesysDriverComplete created";
    
    // 设置状态监控定时器
    m_statusTimer->setInterval(1000); // 1秒检查一次
    connect(m_statusTimer, &QTimer::timeout, this, &GenesysDriverComplete::checkDeviceStatus);
    
    // 初始化默认设置
    initializeDefaultSettings();
}

GenesysDriverComplete::~GenesysDriverComplete()
{
    cleanup();
    qCDebug(dscannerGenesysComplete) << "GenesysDriverComplete destroyed";
}

bool GenesysDriverComplete::initialize(const DeviceInfo &deviceInfo, DScannerUSB *usbDevice)
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (m_isInitialized) {
        qCDebug(dscannerGenesysComplete) << "Genesys driver already initialized";
        return true;
    }
    
    if (!usbDevice) {
        qCWarning(dscannerGenesysComplete) << "Invalid USB device";
        return false;
    }
    
    m_deviceInfo = deviceInfo;
    m_usbDevice = usbDevice;
    
    qCInfo(dscannerGenesysComplete) << "Initializing Genesys driver for device:" << deviceInfo.name;
    
    // 检测芯片组类型
    if (!detectChipsetType()) {
        qCWarning(dscannerGenesysComplete) << "Failed to detect chipset type";
        return false;
    }
    
    qCInfo(dscannerGenesysComplete) << "Detected Genesys chipset:" 
                                    << static_cast<int>(m_chipsetType);
    
    // 初始化芯片组
    if (!initializeChipset()) {
        qCWarning(dscannerGenesysComplete) << "Failed to initialize chipset";
        return false;
    }
    
    // 加载校准数据
    if (!loadCalibrationData()) {
        qCWarning(dscannerGenesysComplete) << "Failed to load calibration data";
        // 校准数据失败不是致命错误，可以使用默认校准
        createDefaultCalibration();
    }
    
    // 启动状态监控
    m_statusTimer->start();
    
    m_isInitialized = true;
    qCInfo(dscannerGenesysComplete) << "Genesys driver initialized successfully";
    
    emit deviceInitialized(deviceInfo);
    return true;
}

void GenesysDriverComplete::cleanup()
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!m_isInitialized) {
        return;
    }
    
    qCDebug(dscannerGenesysComplete) << "Cleaning up Genesys driver";
    
    // 停止扫描
    if (m_isScanning) {
        stopScan();
    }
    
    // 停止状态监控
    m_statusTimer->stop();
    
    // 停放扫描头
    parkScanHead();
    
    // 关闭灯管
    setLampState(false);
    
    // 重置芯片组
    resetChipset();
    
    m_isInitialized = false;
    m_deviceHandle = nullptr;
    m_usbDevice = nullptr;
    
    qCDebug(dscannerGenesysComplete) << "Genesys driver cleanup completed";
}

ScannerCapabilities GenesysDriverComplete::getCapabilities() const
{
    ScannerCapabilities caps;
    
    // 基于芯片组类型设置能力
    switch (m_chipsetType) {
    case GenesysChipsetType::GL646:
        caps = getGL646Capabilities();
        break;
    case GenesysChipsetType::GL843:
        caps = getGL843Capabilities();
        break;
    case GenesysChipsetType::GL846:
        caps = getGL846Capabilities();
        break;
    case GenesysChipsetType::GL847:
        caps = getGL847Capabilities();
        break;
    default:
        caps = getGenericCapabilities();
        break;
    }
    
    return caps;
}

bool GenesysDriverComplete::startScan(const ScanParameters &params)
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!m_isInitialized) {
        qCWarning(dscannerGenesysComplete) << "Driver not initialized";
        return false;
    }
    
    if (m_isScanning) {
        qCWarning(dscannerGenesysComplete) << "Scan already in progress";
        return false;
    }
    
    qCInfo(dscannerGenesysComplete) << "Starting scan with parameters:"
                                    << "resolution:" << params.resolution
                                    << "colorMode:" << static_cast<int>(params.colorMode)
                                    << "area:" << params.scanArea.x << params.scanArea.y
                                    << params.scanArea.width << params.scanArea.height;
    
    // 验证扫描参数
    if (!validateScanParameters(params)) {
        qCWarning(dscannerGenesysComplete) << "Invalid scan parameters";
        return false;
    }
    
    m_currentScanParams = params;
    
    // 准备扫描
    if (!prepareScan()) {
        qCWarning(dscannerGenesysComplete) << "Failed to prepare scan";
        return false;
    }
    
    // 配置扫描参数
    if (!configureScanParameters()) {
        qCWarning(dscannerGenesysComplete) << "Failed to configure scan parameters";
        return false;
    }
    
    // 开启灯管
    if (!setLampState(true)) {
        qCWarning(dscannerGenesysComplete) << "Failed to turn on lamp";
        return false;
    }
    
    // 等待灯管预热
    waitForLampWarmup();
    
    // 执行预扫描校准（如果需要）
    if (m_currentSettings.autoCalibration) {
        performPreScanCalibration();
    }
    
    // 开始实际扫描
    if (!startPhysicalScan()) {
        qCWarning(dscannerGenesysComplete) << "Failed to start physical scan";
        setLampState(false);
        return false;
    }
    
    m_isScanning = true;
    m_scanStartTime = QDateTime::currentDateTime();
    
    qCInfo(dscannerGenesysComplete) << "Scan started successfully";
    emit scanStarted();
    
    return true;
}

bool GenesysDriverComplete::stopScan()
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!m_isScanning) {
        qCDebug(dscannerGenesysComplete) << "No scan to stop";
        return true;
    }
    
    qCInfo(dscannerGenesysComplete) << "Stopping scan";
    
    // 发送停止扫描命令
    if (!sendCommand(GenesysCommands::CMD_STOP_SCAN)) {
        qCWarning(dscannerGenesysComplete) << "Failed to send stop scan command";
    }
    
    // 停放扫描头
    parkScanHead();
    
    // 关闭灯管
    setLampState(false);
    
    m_isScanning = false;
    m_scanBuffer.clear();
    
    qCInfo(dscannerGenesysComplete) << "Scan stopped";
    emit scanStopped();
    
    return true;
}

QByteArray GenesysDriverComplete::readScanData(int maxBytes)
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!m_isScanning) {
        qCWarning(dscannerGenesysComplete) << "No scan in progress";
        return QByteArray();
    }
    
    // 从设备读取原始数据
    QByteArray rawData = readRawScanData(maxBytes);
    if (rawData.isEmpty()) {
        return QByteArray();
    }
    
    // 处理原始数据（校正、转换等）
    QByteArray processedData = processScanData(rawData);
    
    // 更新扫描进度
    updateScanProgress();
    
    return processedData;
}

bool GenesysDriverComplete::isScanning() const
{
    return m_isScanning;
}

GenesysChipsetType GenesysDriverComplete::getChipsetType() const
{
    return m_chipsetType;
}

// 私有方法实现
bool GenesysDriverComplete::detectChipsetType()
{
    qCDebug(dscannerGenesysComplete) << "Detecting Genesys chipset type";
    
    // 读取芯片ID寄存器
    uint8_t chipId = 0;
    if (!readRegister(GenesysRegisters::REG_CHIPID, chipId)) {
        qCWarning(dscannerGenesysComplete) << "Failed to read chip ID register";
        return false;
    }
    
    // 根据芯片ID确定类型
    switch (chipId) {
    case 0x46:
        m_chipsetType = GenesysChipsetType::GL646;
        break;
    case 0x43:
        m_chipsetType = GenesysChipsetType::GL843;
        break;
    case 0x46: // 注意：GL846可能与GL646有相同的ID，需要进一步检测
        if (detectGL846Features()) {
            m_chipsetType = GenesysChipsetType::GL846;
        } else {
            m_chipsetType = GenesysChipsetType::GL646;
        }
        break;
    case 0x47:
        m_chipsetType = GenesysChipsetType::GL847;
        break;
    default:
        qCWarning(dscannerGenesysComplete) << "Unknown Genesys chip ID:" << QString::number(chipId, 16);
        m_chipsetType = GenesysChipsetType::Unknown;
        return false;
    }
    
    qCDebug(dscannerGenesysComplete) << "Detected chipset type:" << static_cast<int>(m_chipsetType);
    return true;
}

bool GenesysDriverComplete::initializeChipset()
{
    qCDebug(dscannerGenesysComplete) << "Initializing Genesys chipset";
    
    // 发送初始化命令
    if (!sendCommand(GenesysCommands::CMD_INIT)) {
        qCWarning(dscannerGenesysComplete) << "Failed to send init command";
        return false;
    }
    
    // 等待初始化完成
    if (!waitForReady(5000)) { // 5秒超时
        qCWarning(dscannerGenesysComplete) << "Chipset initialization timeout";
        return false;
    }
    
    // 芯片组特定的初始化
    switch (m_chipsetType) {
    case GenesysChipsetType::GL646:
        return initializeGL646();
    case GenesysChipsetType::GL843:
        return initializeGL843();
    case GenesysChipsetType::GL846:
        return initializeGL846();
    case GenesysChipsetType::GL847:
        return initializeGL847();
    default:
        qCWarning(dscannerGenesysComplete) << "Unsupported chipset type";
        return false;
    }
}

bool GenesysDriverComplete::initializeGL646()
{
    qCDebug(dscannerGenesysComplete) << "Initializing GL646 specific features";
    
    // GL646特定的初始化序列
    // 设置默认曝光时间
    if (!writeRegister(GenesysRegisters::GL646_REG_EXPOSURE, 0x1000)) {
        return false;
    }
    
    // 设置扫描区域默认值
    if (!writeRegister(GenesysRegisters::GL646_REG_PIXELSTART, 0x0000)) {
        return false;
    }
    
    if (!writeRegister(GenesysRegisters::GL646_REG_PIXELEND, 0x2580)) { // 9600像素
        return false;
    }
    
    // 设置马达控制参数
    if (!setupGL646Motor()) {
        return false;
    }
    
    qCDebug(dscannerGenesysComplete) << "GL646 initialization completed";
    return true;
}

bool GenesysDriverComplete::initializeGL843()
{
    qCDebug(dscannerGenesysComplete) << "Initializing GL843 specific features";
    
    // GL843特定的初始化序列
    // 设置默认分辨率
    if (!writeRegister(GenesysRegisters::GL843_REG_RESOLUTION, 0x0300)) { // 300 DPI
        return false;
    }
    
    // 设置扫描模式
    if (!writeRegister(GenesysRegisters::GL843_REG_SCAN_MODE, 0x01)) { // 平板扫描
        return false;
    }
    
    // 设置颜色模式
    if (!writeRegister(GenesysRegisters::GL843_REG_COLOR_MODE, 0x03)) { // RGB
        return false;
    }
    
    // 设置马达控制参数
    if (!setupGL843Motor()) {
        return false;
    }
    
    qCDebug(dscannerGenesysComplete) << "GL843 initialization completed";
    return true;
}

bool GenesysDriverComplete::readRegister(uint8_t reg, uint8_t &value)
{
    if (!m_usbDevice) {
        return false;
    }
    
    // 构造读取寄存器命令
    QByteArray command;
    command.append(GenesysCommands::CMD_READ_REG);
    command.append(reg);
    
    // 发送命令
    size_t bytesWritten = command.size();
    if (!m_usbDevice->bulkWrite(reinterpret_cast<const uint8_t*>(command.data()), &bytesWritten)) {
        qCWarning(dscannerGenesysComplete) << "Failed to send read register command";
        return false;
    }
    
    // 读取响应
    uint8_t response;
    size_t bytesRead = 1;
    if (!m_usbDevice->bulkRead(0x81, &response, bytesRead)) { // 0x81是输入端点
        qCWarning(dscannerGenesysComplete) << "Failed to read register response";
        return false;
    }
    
    value = response;
    qCDebug(dscannerGenesysComplete) << "Read register" << QString::number(reg, 16) 
                                     << "value:" << QString::number(value, 16);
    return true;
}

bool GenesysDriverComplete::writeRegister(uint8_t reg, uint16_t value)
{
    if (!m_usbDevice) {
        return false;
    }
    
    // 构造写入寄存器命令
    QByteArray command;
    command.append(GenesysCommands::CMD_WRITE_REG);
    command.append(reg);
    command.append(static_cast<uint8_t>(value & 0xFF));        // 低字节
    command.append(static_cast<uint8_t>((value >> 8) & 0xFF)); // 高字节
    
    // 发送命令
    size_t bytesWritten = command.size();
    if (!m_usbDevice->bulkWrite(reinterpret_cast<const uint8_t*>(command.data()), &bytesWritten)) {
        qCWarning(dscannerGenesysComplete) << "Failed to send write register command";
        return false;
    }
    
    qCDebug(dscannerGenesysComplete) << "Wrote register" << QString::number(reg, 16) 
                                     << "value:" << QString::number(value, 16);
    return true;
}

bool GenesysDriverComplete::sendCommand(uint8_t command)
{
    if (!m_usbDevice) {
        return false;
    }
    
    QByteArray cmdData;
    cmdData.append(command);
    
    size_t bytesWritten = cmdData.size();
    bool result = m_usbDevice->bulkWrite(reinterpret_cast<const uint8_t*>(cmdData.data()), &bytesWritten);
    
    if (result) {
        qCDebug(dscannerGenesysComplete) << "Sent command:" << QString::number(command, 16);
    } else {
        qCWarning(dscannerGenesysComplete) << "Failed to send command:" << QString::number(command, 16);
    }
    
    return result;
}

bool GenesysDriverComplete::waitForReady(int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeoutMs) {
        uint8_t status;
        if (readRegister(GenesysRegisters::REG_STATUS, status)) {
            if ((status & 0x01) == 0) { // 准备就绪位
                return true;
            }
        }
        
        QThread::msleep(10); // 10ms延迟
    }
    
    qCWarning(dscannerGenesysComplete) << "Wait for ready timeout after" << timeoutMs << "ms";
    return false;
}

void GenesysDriverComplete::initializeDefaultSettings()
{
    m_currentSettings.autoCalibration = true;
    m_currentSettings.lampWarmupTime = 3000; // 3秒
    m_currentSettings.motorSpeed = 100;
    m_currentSettings.defaultResolution = 300;
    m_currentSettings.maxResolution = 1200;
    m_currentSettings.colorDepth = 8;
    m_currentSettings.enableShading = true;
    m_currentSettings.enableGammaCorrection = true;
}

ScannerCapabilities GenesysDriverComplete::getGL646Capabilities() const
{
    ScannerCapabilities caps;
    caps.supportedResolutions = {75, 150, 300, 600, 1200};
    caps.supportedColorModes = {ColorMode::Lineart, ColorMode::Grayscale, ColorMode::Color};
    caps.supportedFormats = {ImageFormat::PNG, ImageFormat::JPEG, ImageFormat::TIFF};
    caps.maxScanArea = ScanArea(0, 0, 216, 297); // A4大小
    caps.minScanArea = ScanArea(0, 0, 10, 10);
    caps.hasADF = false;
    caps.hasDuplex = false;
    caps.hasPreview = true;
    caps.hasCalibration = true;
    caps.hasLamp = true;
    caps.maxBatchSize = 1;
    return caps;
}

ScannerCapabilities GenesysDriverComplete::getGL843Capabilities() const
{
    ScannerCapabilities caps = getGL646Capabilities();
    caps.supportedResolutions = {75, 150, 300, 600, 1200, 2400};
    caps.hasADF = true; // GL843通常支持ADF
    return caps;
}

// #include "genesys_driver_complete.moc" 