// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerSANE.h"
#include "dscannersane_p.h"

#include <QLibrary>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>
#include <QDebug>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerSANE, "deepinscan.sane")

// DScannerSANE implementation
DScannerSANE::DScannerSANE(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerSANEPrivate(this))
{
    qCDebug(dscannerSANE) << "DScannerSANE created";
}

DScannerSANE::~DScannerSANE()
{
    Q_D(DScannerSANE);
    if (d->initialized) {
        exit();
    }
    delete d_ptr;
}

bool DScannerSANE::init(int *versionCode)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        qCDebug(dscannerSANE) << "SANE already initialized";
        if (versionCode) {
            *versionCode = d->versionCode;
        }
        return true;
    }

    qCDebug(dscannerSANE) << "Initializing SANE subsystem";

    // 尝试加载SANE库
    if (!d->loadSANELibrary()) {
        qCWarning(dscannerSANE) << "Failed to load SANE library";
        return false;
    }

    // 初始化SANE
    if (!d->initializeSANE()) {
        qCWarning(dscannerSANE) << "Failed to initialize SANE";
        d->unloadSANELibrary();
        return false;
    }

    d->initialized = true;
    if (versionCode) {
        *versionCode = d->versionCode;
    }

    qCInfo(dscannerSANE) << "SANE subsystem initialized successfully, version:" 
                         << QString("0x%1").arg(d->versionCode, 0, 16);

    emit statusChanged(SANEStatus::Good);
    return true;
}

void DScannerSANE::exit()
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    qCDebug(dscannerSANE) << "Shutting down SANE subsystem";

    // 关闭所有打开的设备
    for (auto it = d->openDevices.begin(); it != d->openDevices.end(); ++it) {
        d->closeSANEDevice(it.value());
    }
    d->openDevices.clear();

    // 清理SANE
    d->shutdownSANE();
    d->unloadSANELibrary();

    d->initialized = false;
    qCInfo(dscannerSANE) << "SANE subsystem shutdown completed";

    emit statusChanged(SANEStatus::Good);
}

bool DScannerSANE::isInitialized() const
{
    Q_D(const DScannerSANE);
    QMutexLocker locker(&d->mutex);
    return d->initialized;
}

QList<SANEDevice> DScannerSANE::getDevices(bool localOnly)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    QList<SANEDevice> devices;

    if (!d->initialized) {
        qCWarning(dscannerSANE) << "SANE not initialized";
        return devices;
    }

    qCDebug(dscannerSANE) << "Getting SANE devices, localOnly:" << localOnly;

    // 调用SANE API获取设备列表
    devices = d->getSANEDevices(localOnly);

    qCInfo(dscannerSANE) << "Found" << devices.size() << "SANE devices";

    // 发出设备发现信号
    for (const auto &device : devices) {
        emit deviceDiscovered(device);
    }

    return devices;
}

void* DScannerSANE::openDevice(const QString &deviceName)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        qCWarning(dscannerSANE) << "SANE not initialized";
        return nullptr;
    }

    if (d->openDevices.contains(deviceName)) {
        qCWarning(dscannerSANE) << "Device already open:" << deviceName;
        return d->openDevices[deviceName];
    }

    qCDebug(dscannerSANE) << "Opening SANE device:" << deviceName;

    void *handle = d->openSANEDevice(deviceName);
    if (handle) {
        d->openDevices[deviceName] = handle;
        qCInfo(dscannerSANE) << "Device opened successfully:" << deviceName;
    } else {
        qCWarning(dscannerSANE) << "Failed to open device:" << deviceName;
    }

    return handle;
}

void DScannerSANE::closeDevice(void *handle)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !handle) {
        return;
    }

    // 查找设备名称
    QString deviceName;
    for (auto it = d->openDevices.begin(); it != d->openDevices.end(); ++it) {
        if (it.value() == handle) {
            deviceName = it.key();
            d->openDevices.erase(it);
            break;
        }
    }

    qCDebug(dscannerSANE) << "Closing SANE device:" << deviceName;
    d->closeSANEDevice(handle);
    qCInfo(dscannerSANE) << "Device closed:" << deviceName;
}

SANEOptionDescriptor DScannerSANE::getOptionDescriptor(void *handle, int option)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    SANEOptionDescriptor descriptor;
    if (!d->initialized || !handle) {
        return descriptor;
    }

    return d->getSANEOptionDescriptor(handle, option);
}

SANEStatus DScannerSANE::controlOption(void *handle, int option, SANEAction action, QVariant &value)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !handle) {
        return SANEStatus::Invalid;
    }

    return d->controlSANEOption(handle, option, action, value);
}

SANEParameters DScannerSANE::getParameters(void *handle)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    SANEParameters params;
    if (!d->initialized || !handle) {
        return params;
    }

    return d->getSANEParameters(handle);
}

SANEStatus DScannerSANE::startScan(void *handle)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !handle) {
        return SANEStatus::Invalid;
    }

    qCDebug(dscannerSANE) << "Starting SANE scan";
    SANEStatus status = d->startSANEScan(handle);
    
    if (status == SANEStatus::Good) {
        qCInfo(dscannerSANE) << "SANE scan started successfully";
    } else {
        qCWarning(dscannerSANE) << "Failed to start SANE scan, status:" << (int)status;
    }

    return status;
}

SANEStatus DScannerSANE::readData(void *handle, unsigned char *buffer, int maxLength, int *length)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !handle || !buffer || !length) {
        return SANEStatus::Invalid;
    }

    return d->readSANEData(handle, buffer, maxLength, length);
}

void DScannerSANE::cancelScan(void *handle)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !handle) {
        return;
    }

    qCDebug(dscannerSANE) << "Cancelling SANE scan";
    d->cancelSANEScan(handle);
    qCInfo(dscannerSANE) << "SANE scan cancelled";
}

SANEStatus DScannerSANE::setIOMode(void *handle, bool nonBlocking)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !handle) {
        return SANEStatus::Invalid;
    }

    return d->setSANEIOMode(handle, nonBlocking);
}

SANEStatus DScannerSANE::getSelectFd(void *handle, int *fd)
{
    Q_D(DScannerSANE);
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !handle || !fd) {
        return SANEStatus::Invalid;
    }

    return d->getSANESelectFd(handle, fd);
}

QString DScannerSANE::statusToString(SANEStatus status)
{
    switch (status) {
    case SANEStatus::Good:
        return QStringLiteral("Good");
    case SANEStatus::Unsupported:
        return QStringLiteral("Unsupported");
    case SANEStatus::Cancelled:
        return QStringLiteral("Cancelled");
    case SANEStatus::DeviceBusy:
        return QStringLiteral("Device Busy");
    case SANEStatus::Invalid:
        return QStringLiteral("Invalid");
    case SANEStatus::EOF_:
        return QStringLiteral("EOF");
    case SANEStatus::Jammed:
        return QStringLiteral("Jammed");
    case SANEStatus::NoDocs:
        return QStringLiteral("No Documents");
    case SANEStatus::CoverOpen:
        return QStringLiteral("Cover Open");
    case SANEStatus::IOError:
        return QStringLiteral("I/O Error");
    case SANEStatus::NoMem:
        return QStringLiteral("No Memory");
    case SANEStatus::AccessDenied:
        return QStringLiteral("Access Denied");
    default:
        return QStringLiteral("Unknown");
    }
}

SANEDevice DScannerSANE::deviceInfoToSANE(const DeviceInfo &deviceInfo)
{
    SANEDevice saneDevice;
    saneDevice.name = deviceInfo.deviceId;
    saneDevice.vendor = deviceInfo.manufacturer;
    saneDevice.model = deviceInfo.model;
    saneDevice.type = QStringLiteral("flatbed scanner");
    return saneDevice;
}

DeviceInfo DScannerSANE::saneToDeviceInfo(const SANEDevice &saneDevice)
{
    DeviceInfo deviceInfo;
    deviceInfo.deviceId = saneDevice.name;
    deviceInfo.name = QStringLiteral("%1 %2").arg(saneDevice.vendor, saneDevice.model);
    deviceInfo.manufacturer = saneDevice.vendor;
    deviceInfo.model = saneDevice.model;
    deviceInfo.driverType = DriverType::SANE;
    deviceInfo.protocol = CommunicationProtocol::USB; // 默认USB
    deviceInfo.connectionString = saneDevice.name;
    deviceInfo.isAvailable = true;
    return deviceInfo;
}

// DScannerSANEDriver implementation
DScannerSANEDriver::DScannerSANEDriver(QObject *parent)
    : DScannerDriver(parent)
    , d_ptr(new DScannerSANEDriverPrivate(this))
{
    Q_D(DScannerSANEDriver);
    
    // 连接SANE信号
    connect(d->sane, &DScannerSANE::statusChanged,
            this, &DScannerSANEDriver::onSANEStatusChanged);
    connect(d->sane, &DScannerSANE::deviceDiscovered,
            this, &DScannerSANEDriver::onSANEDeviceDiscovered);
    connect(d->sane, &DScannerSANE::errorOccurred,
            this, &DScannerSANEDriver::onSANEErrorOccurred);

    qCDebug(dscannerSANE) << "DScannerSANEDriver created";
}

DScannerSANEDriver::~DScannerSANEDriver()
{
    Q_D(DScannerSANEDriver);
    if (d->initialized) {
        shutdown();
    }
    delete d_ptr;
}

QString DScannerSANEDriver::driverName() const
{
    return QStringLiteral("SANE Backend Driver");
}

QString DScannerSANEDriver::driverVersion() const
{
    return QStringLiteral("1.0.0");
}

DriverType DScannerSANEDriver::driverType() const
{
    return DriverType::SANE;
}

QStringList DScannerSANEDriver::supportedDevices() const
{
    // SANE支持的设备列表很大，这里返回通用描述
    return QStringList() << QStringLiteral("SANE Compatible Scanners");
}

bool DScannerSANEDriver::initialize()
{
    Q_D(DScannerSANEDriver);
    
    if (d->initialized) {
        return true;
    }

    qCDebug(dscannerSANE) << "Initializing SANE driver";

    if (!d->sane->init(&d->saneVersion)) {
        d->lastError = QStringLiteral("Failed to initialize SANE subsystem");
        return false;
    }

    d->initialized = true;
    qCInfo(dscannerSANE) << "SANE driver initialized successfully";
    return true;
}

void DScannerSANEDriver::shutdown()
{
    Q_D(DScannerSANEDriver);
    
    if (!d->initialized) {
        return;
    }

    qCDebug(dscannerSANE) << "Shutting down SANE driver";

    if (d->currentDevice) {
        closeDevice();
    }

    d->sane->exit();
    d->initialized = false;
    
    qCInfo(dscannerSANE) << "SANE driver shutdown completed";
}

QList<DeviceInfo> DScannerSANEDriver::discoverDevices()
{
    Q_D(DScannerSANEDriver);
    
    QList<DeviceInfo> devices;
    
    if (!d->initialized) {
        d->lastError = QStringLiteral("Driver not initialized");
        return devices;
    }

    qCDebug(dscannerSANE) << "Discovering SANE devices";

    QList<SANEDevice> saneDevices = d->sane->getDevices(false);
    
    for (const auto &saneDevice : saneDevices) {
        DeviceInfo deviceInfo = DScannerSANE::saneToDeviceInfo(saneDevice);
        devices.append(deviceInfo);
    }

    qCInfo(dscannerSANE) << "Discovered" << devices.size() << "SANE devices";
    return devices;
}

bool DScannerSANEDriver::isDeviceSupported(const DeviceInfo &deviceInfo)
{
    // SANE驱动支持所有SANE兼容设备
    return deviceInfo.driverType == DriverType::SANE || 
           deviceInfo.connectionString.startsWith(QStringLiteral("sane:"));
}

bool DScannerSANEDriver::openDevice(const QString &deviceId)
{
    Q_D(DScannerSANEDriver);
    
    if (!d->initialized) {
        d->lastError = QStringLiteral("Driver not initialized");
        return false;
    }

    if (d->currentDevice) {
        closeDevice();
    }

    qCDebug(dscannerSANE) << "Opening SANE device:" << deviceId;

    d->currentDevice = d->sane->openDevice(deviceId);
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("Failed to open SANE device: %1").arg(deviceId);
        return false;
    }

    d->currentDeviceId = deviceId;
    qCInfo(dscannerSANE) << "SANE device opened successfully:" << deviceId;
    return true;
}

void DScannerSANEDriver::closeDevice()
{
    Q_D(DScannerSANEDriver);
    
    if (!d->currentDevice) {
        return;
    }

    qCDebug(dscannerSANE) << "Closing SANE device:" << d->currentDeviceId;

    d->sane->closeDevice(d->currentDevice);
    d->currentDevice = nullptr;
    d->currentDeviceId.clear();
    
    qCInfo(dscannerSANE) << "SANE device closed";
}

bool DScannerSANEDriver::isDeviceOpen() const
{
    Q_D(const DScannerSANEDriver);
    return d->currentDevice != nullptr;
}

ScannerCapabilities DScannerSANEDriver::getCapabilities()
{
    Q_D(DScannerSANEDriver);
    
    ScannerCapabilities caps;
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return caps;
    }

    // 从SANE选项中构建能力信息
    caps = d->buildCapabilitiesFromSANE();
    
    return caps;
}

bool DScannerSANEDriver::setScanParameters(const ScanParameters &params)
{
    Q_D(DScannerSANEDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    // 将扫描参数映射到SANE选项
    if (!d->applyScanParametersToSANE(params)) {
        return false;
    }

    d->currentScanParams = params;
    return true;
}

ScanParameters DScannerSANEDriver::getScanParameters()
{
    Q_D(const DScannerSANEDriver);
    return d->currentScanParams;
}

bool DScannerSANEDriver::startScan()
{
    Q_D(DScannerSANEDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    SANEStatus status = d->sane->startScan(d->currentDevice);
    if (status != SANEStatus::Good) {
        d->lastError = QStringLiteral("Failed to start scan: %1")
                       .arg(DScannerSANE::statusToString(status));
        return false;
    }

    d->isScanning = true;
    emit scanStarted();
    return true;
}

void DScannerSANEDriver::stopScan()
{
    Q_D(DScannerSANEDriver);
    
    if (!d->currentDevice || !d->isScanning) {
        return;
    }

    d->sane->cancelScan(d->currentDevice);
    d->isScanning = false;
    emit scanStopped();
}

bool DScannerSANEDriver::pauseScan()
{
    // SANE不直接支持暂停，返回false
    return false;
}

bool DScannerSANEDriver::resumeScan()
{
    // SANE不直接支持恢复，返回false
    return false;
}

QImage DScannerSANEDriver::getPreview()
{
    Q_D(DScannerSANEDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return QImage();
    }

    // 实现预览扫描
    return d->performPreviewScan();
}

bool DScannerSANEDriver::calibrateDevice()
{
    Q_D(DScannerSANEDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    // 查找校准按钮选项并触发
    return d->triggerCalibration();
}

bool DScannerSANEDriver::setParameter(const QString &name, const QVariant &value)
{
    Q_D(DScannerSANEDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    // 查找对应的SANE选项并设置
    return d->setSANEParameter(name, value);
}

QVariant DScannerSANEDriver::getParameter(const QString &name) const
{
    Q_D(const DScannerSANEDriver);
    
    if (!d->currentDevice) {
        return QVariant();
    }

    // 获取SANE选项值
    return d->getSANEParameter(name);
}

QStringList DScannerSANEDriver::getParameterNames() const
{
    Q_D(const DScannerSANEDriver);
    
    if (!d->currentDevice) {
        return QStringList();
    }

    // 获取所有SANE选项名称
    return d->getSANEParameterNames();
}

QString DScannerSANEDriver::lastError() const
{
    Q_D(const DScannerSANEDriver);
    return d->lastError;
}

void DScannerSANEDriver::onSANEStatusChanged(SANEStatus status)
{
    qCDebug(dscannerSANE) << "SANE status changed:" << DScannerSANE::statusToString(status);
}

void DScannerSANEDriver::onSANEDeviceDiscovered(const SANEDevice &device)
{
    qCDebug(dscannerSANE) << "SANE device discovered:" << device.name;
    
    DeviceInfo deviceInfo = DScannerSANE::saneToDeviceInfo(device);
    emit deviceDiscovered(deviceInfo);
}

void DScannerSANEDriver::onSANEErrorOccurred(SANEStatus status, const QString &message)
{
    Q_D(DScannerSANEDriver);
    
    QString errorMsg = QStringLiteral("SANE error: %1 - %2")
                       .arg(DScannerSANE::statusToString(status), message);
    
    qCWarning(dscannerSANE) << errorMsg;
    d->lastError = errorMsg;
    
    emit errorOccurred(errorMsg);
}

#include "dscannersane.moc" 