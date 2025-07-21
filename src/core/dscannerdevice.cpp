// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dscannerdevice_p.h"
#include "Scanner/DScannerDevice.h"
#include "Scanner/DScannerException.h"
#include "Scanner/DScannerTypes.h"

#include <QLoggingCategory>
#include <QMutexLocker>
#include <QTimer>
#include <QThread>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerDevice, "deepinscan.device")

// DScannerDevicePrivate implementation
DScannerDevicePrivate::DScannerDevicePrivate(DScannerDevice *q)
    : QObject(q)
    , q_ptr(q)
    , status(DScannerDevice::Status::Offline)
    , driverType(DriverType::Generic)
    , isConnected(false)
    , isOpen(false)
    , isScanInProgress(false)
    , isPreviewInProgress(false)
    , isPaused(false)
    , scanProgress(0)
    , deviceHandle(nullptr)
    , driverData(nullptr)
    , statusTimer(new QTimer(this))
    , progressTimer(new QTimer(this))
{
    // 初始化定时器
    statusTimer->setInterval(1000); // 1秒检查一次状态
    progressTimer->setInterval(100); // 100ms更新一次进度
    
    // 连接定时器信号
    connect(statusTimer, &QTimer::timeout, this, &DScannerDevicePrivate::onStatusTimer);
    connect(progressTimer, &QTimer::timeout, this, &DScannerDevicePrivate::onProgressTimer);
    
    // 启动状态监控定时器
    statusTimer->start();
}

DScannerDevicePrivate::~DScannerDevicePrivate()
{
    cleanup();
}

void DScannerDevicePrivate::cleanup()
{
    QMutexLocker locker(&mutex);
    
    // 停止定时器
    if (statusTimer) {
        statusTimer->stop();
    }
    if (progressTimer) {
        progressTimer->stop();
    }
    
    // 清理设备句柄
    if (deviceHandle) {
        deviceHandle = nullptr;
    }
    
    if (driverData) {
        driverData = nullptr;
    }
    
    // 重置状态
    status = DScannerDevice::Status::Offline;
    isConnected = false;
    isOpen = false;
    isScanInProgress = false;
    isPreviewInProgress = false;
    isPaused = false;
    scanProgress = 0;
}

bool DScannerDevicePrivate::initializeDevice()
{
    QMutexLocker locker(&mutex);
    
    if (deviceInfo.deviceId.isEmpty()) {
        lastError = QStringLiteral("设备ID为空");
        return false;
    }
    
    // 初始化设备句柄
    deviceHandle = nullptr;
    driverData = nullptr;
    
    // 设置初始状态
    status = DScannerDevice::Status::Ready;
    isConnected = true;
    
    return true;
}

void DScannerDevicePrivate::onStatusTimer()
{
    // 定期检查设备状态
    if (isConnected && deviceHandle) {
        // 这里可以添加设备状态检查逻辑
        // 目前只是占位实现
    }
}

void DScannerDevicePrivate::onProgressTimer()
{
    // 定期更新扫描进度
    if (isScanInProgress) {
        // 这里可以添加进度更新逻辑
        // 目前只是占位实现
    }
}

bool DScannerDevicePrivate::validateScanParameters(const ScanParameters &params)
{
    if (!params.isValid()) {
        lastError = QStringLiteral("扫描参数无效");
        return false;
    }
    
    // 检查分辨率是否支持
    if (!capabilities.supportedResolutions.contains(params.resolution)) {
        lastError = QStringLiteral("不支持的分辨率: %1").arg(params.resolution);
        return false;
    }
    
    // 检查颜色模式是否支持
    if (!capabilities.supportedColorModes.contains(params.colorMode)) {
        lastError = QStringLiteral("不支持的颜色模式");
        return false;
    }
    
    return true;
}

void DScannerDevicePrivate::updateStatus(DScannerDevice::Status newStatus)
{
    if (status != newStatus) {
        status = newStatus;
        emit q_ptr->statusChanged(status);
    }
}

void DScannerDevicePrivate::updateProgress(int percentage)
{
    if (scanProgress != percentage) {
        scanProgress = percentage;
        emit q_ptr->scanProgress(percentage);
    }
}

void DScannerDevicePrivate::emitError(const QString &error)
{
    lastError = error;
    emit q_ptr->errorOccurred(error);
}

void DScannerDevicePrivate::emitError(int code, const QString &message)
{
    lastError = message;
    emit q_ptr->errorOccurred(code, message);
}

// 设备操作的默认实现
bool DScannerDevicePrivate::openDevice()
{
    QMutexLocker locker(&mutex);
    if (isOpen) {
        return true;
    }
    
    // 默认实现，子类可以重写
    isOpen = true;
    updateStatus(DScannerDevice::Status::Ready);
    return true;
}

void DScannerDevicePrivate::closeDevice()
{
    QMutexLocker locker(&mutex);
    if (!isOpen) {
        return;
    }
    
    // 默认实现，子类可以重写
    isOpen = false;
    updateStatus(DScannerDevice::Status::Offline);
}

bool DScannerDevicePrivate::resetDevice()
{
    QMutexLocker locker(&mutex);
    // 默认实现，子类可以重写
    updateStatus(DScannerDevice::Status::Ready);
    return true;
}

bool DScannerDevicePrivate::calibrateDevice()
{
    QMutexLocker locker(&mutex);
    // 默认实现，子类可以重写
    return true;
}

bool DScannerDevicePrivate::startScanOperation(const ScanParameters &params)
{
    QMutexLocker locker(&mutex);
    
    if (!validateScanParameters(params)) {
        return false;
    }
    
    if (isScanInProgress) {
        lastError = QStringLiteral("扫描正在进行中");
        return false;
    }
    
    currentScanParams = params;
    isScanInProgress = true;
    isPaused = false;
    scanProgress = 0;
    
    updateStatus(DScannerDevice::Status::Busy);
    progressTimer->start();
    
    // 默认实现，子类可以重写
    return true;
}

void DScannerDevicePrivate::stopScanOperation()
{
    QMutexLocker locker(&mutex);
    
    if (!isScanInProgress) {
        return;
    }
    
    isScanInProgress = false;
    isPaused = false;
    scanProgress = 0;
    
    progressTimer->stop();
    updateStatus(DScannerDevice::Status::Ready);
    
    // 默认实现，子类可以重写
}

bool DScannerDevicePrivate::pauseScanOperation()
{
    QMutexLocker locker(&mutex);
    
    if (!isScanInProgress || isPaused) {
        return false;
    }
    
    isPaused = true;
    progressTimer->stop();
    
    // 默认实现，子类可以重写
    return true;
}

bool DScannerDevicePrivate::resumeScanOperation()
{
    QMutexLocker locker(&mutex);
    
    if (!isScanInProgress || !isPaused) {
        return false;
    }
    
    isPaused = false;
    progressTimer->start();
    
    // 默认实现，子类可以重写
    return true;
}

bool DScannerDevicePrivate::startPreviewOperation()
{
    QMutexLocker locker(&mutex);
    
    if (isPreviewInProgress) {
        return false;
    }
    
    isPreviewInProgress = true;
    updateStatus(DScannerDevice::Status::Busy);
    
    // 默认实现，子类可以重写
    return true;
}

void DScannerDevicePrivate::stopPreviewOperation()
{
    QMutexLocker locker(&mutex);
    
    if (!isPreviewInProgress) {
        return;
    }
    
    isPreviewInProgress = false;
    updateStatus(DScannerDevice::Status::Ready);
    
    // 默认实现，子类可以重写
}

bool DScannerDevicePrivate::setDeviceParameter(const QString &name, const QVariant &value)
{
    Q_UNUSED(name)
    Q_UNUSED(value)
    // 默认实现，子类可以重写
    return true;
}

QVariant DScannerDevicePrivate::getDeviceParameter(const QString &name) const
{
    Q_UNUSED(name)
    // 默认实现，子类可以重写
    return QVariant();
}

ScannerCapabilities DScannerDevicePrivate::queryCapabilities()
{
    // 默认实现，子类可以重写
    return capabilities;
}

// DScannerDevice implementation
DScannerDevice::DScannerDevice(const DeviceInfo &deviceInfo, QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerDevicePrivate(this))
{
    Q_D(DScannerDevice);
    d->deviceInfo = deviceInfo;
    d->initializeDevice();
}

DScannerDevice::DScannerDevice(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerDevicePrivate(this))
{
    Q_D(DScannerDevice);
    d->initializeDevice();
}

DScannerDevice::DScannerDevice(DScannerDevicePrivate &dd, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
    Q_D(DScannerDevice);
    d->initializeDevice();
}

DScannerDevice::~DScannerDevice()
{
    // d_ptr is managed by QScopedPointer, no need to delete manually
}

DeviceInfo DScannerDevice::deviceInfo() const
{
    Q_D(const DScannerDevice);
    return d->deviceInfo;
}

QString DScannerDevice::deviceId() const
{
    Q_D(const DScannerDevice);
    return d->deviceInfo.deviceId;
}

QString DScannerDevice::deviceName() const
{
    Q_D(const DScannerDevice);
    return d->deviceInfo.name;
}

QString DScannerDevice::manufacturer() const
{
    Q_D(const DScannerDevice);
    return d->deviceInfo.manufacturer;
}

QString DScannerDevice::model() const
{
    Q_D(const DScannerDevice);
    return d->deviceInfo.model;
}

QString DScannerDevice::driverType() const
{
    Q_D(const DScannerDevice);
    // 将枚举转换为字符串
    switch (d->driverType) {
    case DriverType::SANE:
        return QStringLiteral("SANE");
    case DriverType::Genesys:
        return QStringLiteral("Genesys");
    case DriverType::Canon:
        return QStringLiteral("Canon");
    case DriverType::Epson:
        return QStringLiteral("Epson");
    case DriverType::HP:
        return QStringLiteral("HP");
    case DriverType::Brother:
        return QStringLiteral("Brother");
    case DriverType::Fujitsu:
        return QStringLiteral("Fujitsu");
    case DriverType::Generic:
    default:
        return QStringLiteral("Generic");
    }
}

DScannerDevice::Status DScannerDevice::status() const
{
    Q_D(const DScannerDevice);
    return d->status;
}

bool DScannerDevice::isConnected() const
{
    Q_D(const DScannerDevice);
    return d->isConnected;
}

bool DScannerDevice::isReady() const
{
    Q_D(const DScannerDevice);
    return d->status == Status::Ready;
}

ScannerCapabilities DScannerDevice::capabilities() const
{
    Q_D(const DScannerDevice);
    return d->capabilities;
}

QList<int> DScannerDevice::supportedResolutions() const
{
    Q_D(const DScannerDevice);
    return d->capabilities.supportedResolutions;
}

QList<ColorMode> DScannerDevice::supportedColorModes() const
{
    Q_D(const DScannerDevice);
    return d->capabilities.supportedColorModes;
}

QList<ImageFormat> DScannerDevice::supportedFormats() const
{
    Q_D(const DScannerDevice);
    return d->capabilities.supportedFormats;
}

ScanArea DScannerDevice::maxScanArea() const
{
    Q_D(const DScannerDevice);
    return d->capabilities.maxScanArea;
}

bool DScannerDevice::open()
{
    Q_D(DScannerDevice);
    return d->openDevice();
}

void DScannerDevice::close()
{
    Q_D(DScannerDevice);
    d->closeDevice();
}

bool DScannerDevice::reset()
{
    Q_D(DScannerDevice);
    return d->resetDevice();
}

bool DScannerDevice::calibrate()
{
    Q_D(DScannerDevice);
    return d->calibrateDevice();
}

bool DScannerDevice::startScan(const ScanParameters &params)
{
    Q_D(DScannerDevice);
    return d->startScanOperation(params);
}

void DScannerDevice::stopScan()
{
    Q_D(DScannerDevice);
    d->stopScanOperation();
}

bool DScannerDevice::pauseScan()
{
    Q_D(DScannerDevice);
    return d->pauseScanOperation();
}

bool DScannerDevice::resumeScan()
{
    Q_D(DScannerDevice);
    return d->resumeScanOperation();
}

ScanParameters DScannerDevice::scanParameters() const
{
    Q_D(const DScannerDevice);
    return d->currentScanParams;
}

bool DScannerDevice::setScanParameters(const ScanParameters &params)
{
    Q_D(DScannerDevice);
    if (d->validateScanParameters(params)) {
        d->currentScanParams = params;
        return true;
    }
    return false;
}

bool DScannerDevice::setParameter(const QString &name, const QVariant &value)
{
    Q_D(DScannerDevice);
    return d->setDeviceParameter(name, value);
}

QVariant DScannerDevice::parameter(const QString &name) const
{
    Q_D(const DScannerDevice);
    return d->getDeviceParameter(name);
}

// processingOptions methods removed as they are not declared in header

bool DScannerDevice::startPreview()
{
    Q_D(DScannerDevice);
    return d->startPreviewOperation();
}

void DScannerDevice::stopPreview()
{
    Q_D(DScannerDevice);
    d->stopPreviewOperation();
}

QString DScannerDevice::name() const
{
    return deviceName();
}

QVariantMap DScannerDevice::parameters() const
{
    // 实现参数获取的基础版本
    QVariantMap params;
    Q_D(const DScannerDevice);
    
    params["deviceId"] = d->deviceInfo.deviceId;
    params["name"] = d->deviceInfo.name;
    params["manufacturer"] = d->deviceInfo.manufacturer;
    params["model"] = d->deviceInfo.model;
    params["status"] = static_cast<int>(d->status);
    params["isConnected"] = d->isConnected;
    params["isReady"] = isReady();
    
    return params;
}

bool DScannerDevice::isPreviewAvailable() const
{
    // 基础实现，可以根据设备能力进行扩展
    return true;
}

int DScannerDevice::scanProgress() const
{
    Q_D(const DScannerDevice);
    return d->scanProgress;
}

// Manual MOC inclusion for private class that needs signal processing
#include "moc_dscannerdevice_p.cpp" 