// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerGenesys.h"
#include "dscannergenesys_p.h"

#include <QLoggingCategory>
#include <QMutexLocker>
#include <QTimer>
#include <QDebug>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerGenesys, "deepinscan.genesys")

// DScannerGenesysDriver implementation
DScannerGenesysDriver::DScannerGenesysDriver(QObject *parent)
    : DScannerDriver(parent)
    , d_ptr(new DScannerGenesysDriverPrivate(this))
{
    qCDebug(dscannerGenesys) << "DScannerGenesysDriver created";
}

DScannerGenesysDriver::~DScannerGenesysDriver()
{
    Q_D(DScannerGenesysDriver);
    if (d->initialized) {
        shutdown();
    }
    delete d_ptr;
}

QString DScannerGenesysDriver::driverName() const
{
    return QStringLiteral("Genesys Scanner Driver");
}

QString DScannerGenesysDriver::driverVersion() const
{
    return QStringLiteral("1.0.0");
}

DriverType DScannerGenesysDriver::driverType() const
{
    return DriverType::Genesys;
}

QStringList DScannerGenesysDriver::supportedDevices() const
{
    Q_D(const DScannerGenesysDriver);
    return d->getSupportedDeviceNames();
}

bool DScannerGenesysDriver::initialize()
{
    Q_D(DScannerGenesysDriver);
    
    if (d->initialized) {
        return true;
    }

    qCDebug(dscannerGenesys) << "Initializing Genesys driver";

    if (!d->initializeUSB()) {
        d->lastError = QStringLiteral("Failed to initialize USB subsystem");
        return false;
    }

    if (!d->loadDeviceDatabase()) {
        d->lastError = QStringLiteral("Failed to load device database");
        return false;
    }

    d->initialized = true;
    qCInfo(dscannerGenesys) << "Genesys driver initialized successfully";
    return true;
}

void DScannerGenesysDriver::shutdown()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->initialized) {
        return;
    }

    qCDebug(dscannerGenesys) << "Shutting down Genesys driver";

    if (d->currentDevice) {
        closeDevice();
    }

    d->shutdownUSB();
    d->initialized = false;
    
    qCInfo(dscannerGenesys) << "Genesys driver shutdown completed";
}

QList<DeviceInfo> DScannerGenesysDriver::discoverDevices()
{
    Q_D(DScannerGenesysDriver);
    
    QList<DeviceInfo> devices;
    
    if (!d->initialized) {
        d->lastError = QStringLiteral("Driver not initialized");
        return devices;
    }

    qCDebug(dscannerGenesys) << "Discovering Genesys devices";

    // 发现USB设备
    QList<USBDeviceDescriptor> usbDevices = d->usbComm->discoverDevices();
    
    for (const auto &usbDevice : usbDevices) {
        GenesysModel model = getDeviceModel(usbDevice.vendorId, usbDevice.productId);
        if (model.chipset != GenesysChipset::Unknown) {
            DeviceInfo deviceInfo = d->convertToDeviceInfo(usbDevice, model);
            devices.append(deviceInfo);
        }
    }

    qCInfo(dscannerGenesys) << "Discovered" << devices.size() << "Genesys devices";
    return devices;
}

bool DScannerGenesysDriver::isDeviceSupported(const DeviceInfo &deviceInfo)
{
    // 检查是否为Genesys设备
    if (deviceInfo.driverType != DriverType::Genesys) {
        return false;
    }

    // 检查设备型号是否在支持列表中
    GenesysModel model = getDeviceModel(deviceInfo.vendorId, deviceInfo.productId);
    return model.chipset != GenesysChipset::Unknown;
}

bool DScannerGenesysDriver::openDevice(const QString &deviceId)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->initialized) {
        d->lastError = QStringLiteral("Driver not initialized");
        return false;
    }

    if (d->currentDevice) {
        closeDevice();
    }

    qCDebug(dscannerGenesys) << "Opening Genesys device:" << deviceId;

    if (!d->openUSBDevice(deviceId)) {
        return false;
    }

    if (!d->initializeDevice()) {
        d->closeUSBDevice();
        return false;
    }

    d->currentDeviceId = deviceId;
    qCInfo(dscannerGenesys) << "Genesys device opened successfully:" << deviceId;
    return true;
}

void DScannerGenesysDriver::closeDevice()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return;
    }

    qCDebug(dscannerGenesys) << "Closing Genesys device:" << d->currentDeviceId;

    d->shutdownDevice();
    d->closeUSBDevice();
    d->currentDevice = nullptr;
    d->currentDeviceId.clear();
    
    qCInfo(dscannerGenesys) << "Genesys device closed";
}

bool DScannerGenesysDriver::isDeviceOpen() const
{
    Q_D(const DScannerGenesysDriver);
    return d->currentDevice != nullptr;
}

ScannerCapabilities DScannerGenesysDriver::getCapabilities()
{
    Q_D(DScannerGenesysDriver);
    
    ScannerCapabilities caps;
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return caps;
    }

    return d->buildCapabilities();
}

bool DScannerGenesysDriver::setScanParameters(const ScanParameters &params)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    if (!d->applyScanParameters(params)) {
        return false;
    }

    d->currentScanParams = params;
    return true;
}

ScanParameters DScannerGenesysDriver::getScanParameters()
{
    Q_D(const DScannerGenesysDriver);
    return d->currentScanParams;
}

bool DScannerGenesysDriver::startScan()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    if (!d->startScanning()) {
        return false;
    }

    d->isScanning = true;
    emit scanStarted();
    return true;
}

void DScannerGenesysDriver::stopScan()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice || !d->isScanning) {
        return;
    }

    d->stopScanning();
    d->isScanning = false;
    emit scanStopped();
}

bool DScannerGenesysDriver::pauseScan()
{
    // Genesys驱动不支持暂停，返回false
    return false;
}

bool DScannerGenesysDriver::resumeScan()
{
    // Genesys驱动不支持恢复，返回false
    return false;
}

QImage DScannerGenesysDriver::getPreview()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return QImage();
    }

    return d->performPreviewScan();
}

bool DScannerGenesysDriver::calibrateDevice()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    return d->performCalibration();
}

bool DScannerGenesysDriver::setParameter(const QString &name, const QVariant &value)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        d->lastError = QStringLiteral("No device open");
        return false;
    }

    return d->setDeviceParameter(name, value);
}

QVariant DScannerGenesysDriver::getParameter(const QString &name) const
{
    Q_D(const DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return QVariant();
    }

    return d->getDeviceParameter(name);
}

QStringList DScannerGenesysDriver::getParameterNames() const
{
    Q_D(const DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return QStringList();
    }

    return d->getDeviceParameterNames();
}

QString DScannerGenesysDriver::lastError() const
{
    Q_D(const DScannerGenesysDriver);
    return d->lastError;
}

GenesysChipset DScannerGenesysDriver::getChipset() const
{
    Q_D(const DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return GenesysChipset::Unknown;
    }

    return d->currentModel.chipset;
}

GenesysModel DScannerGenesysDriver::getModel() const
{
    Q_D(const DScannerGenesysDriver);
    return d->currentModel;
}

GenesysSensor DScannerGenesysDriver::getSensor() const
{
    Q_D(const DScannerGenesysDriver);
    return d->currentSensor;
}

quint8 DScannerGenesysDriver::readRegister(int address)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return 0;
    }

    return d->readRegister(address);
}

bool DScannerGenesysDriver::writeRegister(int address, quint8 value)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return false;
    }

    return d->writeRegister(address, value);
}

QByteArray DScannerGenesysDriver::readRegisters(int startAddress, int count)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return QByteArray();
    }

    return d->readRegisters(startAddress, count);
}

bool DScannerGenesysDriver::writeRegisters(int startAddress, const QByteArray &values)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return false;
    }

    return d->writeRegisters(startAddress, values);
}

bool DScannerGenesysDriver::initializeChipset()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return false;
    }

    return d->initializeChipset();
}

bool DScannerGenesysDriver::calibrateSensor()
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return false;
    }

    return d->calibrateSensor();
}

bool DScannerGenesysDriver::setScanArea(const ScanArea &area)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return false;
    }

    return d->setScanArea(area);
}

bool DScannerGenesysDriver::setScanResolution(int resolution)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return false;
    }

    return d->setScanResolution(resolution);
}

bool DScannerGenesysDriver::setColorMode(ColorMode mode)
{
    Q_D(DScannerGenesysDriver);
    
    if (!d->currentDevice) {
        return false;
    }

    return d->setColorMode(mode);
}

QList<GenesysChipset> DScannerGenesysDriver::supportedChipsets()
{
    return {
        GenesysChipset::GL646,
        GenesysChipset::GL841,
        GenesysChipset::GL842,
        GenesysChipset::GL843,
        GenesysChipset::GL846,
        GenesysChipset::GL847,
        GenesysChipset::GL124
    };
}

QString DScannerGenesysDriver::chipsetName(GenesysChipset chipset)
{
    switch (chipset) {
    case GenesysChipset::GL646:
        return QStringLiteral("GL646");
    case GenesysChipset::GL841:
        return QStringLiteral("GL841");
    case GenesysChipset::GL842:
        return QStringLiteral("GL842");
    case GenesysChipset::GL843:
        return QStringLiteral("GL843");
    case GenesysChipset::GL846:
        return QStringLiteral("GL846");
    case GenesysChipset::GL847:
        return QStringLiteral("GL847");
    case GenesysChipset::GL124:
        return QStringLiteral("GL124");
    default:
        return QStringLiteral("Unknown");
    }
}

GenesysChipset DScannerGenesysDriver::detectChipset(quint16 vendorId, quint16 productId)
{
    GenesysModel model = getDeviceModel(vendorId, productId);
    return model.chipset;
}

GenesysModel DScannerGenesysDriver::getDeviceModel(quint16 vendorId, quint16 productId)
{
    // 静态设备模型数据库
    static QHash<QPair<quint16, quint16>, GenesysModel> deviceDatabase;
    
    // 初始化设备数据库（仅第一次调用时）
    if (deviceDatabase.isEmpty()) {
        // Canon设备
        GenesysModel canonLide25;
        canonLide25.name = QStringLiteral("CanoScan LiDE 25");
        canonLide25.vendor = QStringLiteral("Canon");
        canonLide25.model = QStringLiteral("LiDE 25");
        canonLide25.chipset = GenesysChipset::GL841;
        canonLide25.vendorId = 0x04A9;
        canonLide25.productId = 0x2220;
        canonLide25.maxResolution = 1200;
        canonLide25.minResolution = 75;
        canonLide25.colorModes = {ColorMode::Lineart, ColorMode::Grayscale, ColorMode::Color};
        canonLide25.maxScanArea = ScanArea(0, 0, 216, 297); // A4
        canonLide25.sensorType = SensorType::CIS;
        canonLide25.hasLamp = false;
        deviceDatabase[{0x04A9, 0x2220}] = canonLide25;

        GenesysModel canonLide35;
        canonLide35.name = QStringLiteral("CanoScan LiDE 35");
        canonLide35.vendor = QStringLiteral("Canon");
        canonLide35.model = QStringLiteral("LiDE 35");
        canonLide35.chipset = GenesysChipset::GL841;
        canonLide35.vendorId = 0x04A9;
        canonLide35.productId = 0x2225;
        canonLide35.maxResolution = 1200;
        canonLide35.colorModes = {ColorMode::Lineart, ColorMode::Grayscale, ColorMode::Color};
        canonLide35.maxScanArea = ScanArea(0, 0, 216, 297);
        canonLide35.sensorType = SensorType::CIS;
        deviceDatabase[{0x04A9, 0x2225}] = canonLide35;

        // HP设备
        GenesysModel hp2400;
        hp2400.name = QStringLiteral("HP ScanJet 2400c");
        hp2400.vendor = QStringLiteral("HP");
        hp2400.model = QStringLiteral("ScanJet 2400c");
        hp2400.chipset = GenesysChipset::GL646;
        hp2400.vendorId = 0x03F0;
        hp2400.productId = 0x0A01;
        hp2400.maxResolution = 1200;
        hp2400.colorModes = {ColorMode::Lineart, ColorMode::Grayscale, ColorMode::Color};
        hp2400.maxScanArea = ScanArea(0, 0, 216, 297);
        hp2400.sensorType = SensorType::CCD;
        hp2400.hasLamp = true;
        deviceDatabase[{0x03F0, 0x0A01}] = hp2400;

        // Plustek设备
        GenesysModel plustekOpticPro;
        plustekOpticPro.name = QStringLiteral("Plustek OpticPro ST64");
        plustekOpticPro.vendor = QStringLiteral("Plustek");
        plustekOpticPro.model = QStringLiteral("OpticPro ST64");
        plustekOpticPro.chipset = GenesysChipset::GL646;
        plustekOpticPro.vendorId = 0x07B3;
        plustekOpticPro.productId = 0x0017;
        plustekOpticPro.maxResolution = 1200;
        plustekOpticPro.colorModes = {ColorMode::Lineart, ColorMode::Grayscale, ColorMode::Color};
        plustekOpticPro.maxScanArea = ScanArea(0, 0, 216, 297);
        plustekOpticPro.sensorType = SensorType::CCD;
        deviceDatabase[{0x07B3, 0x0017}] = plustekOpticPro;
    }

    QPair<quint16, quint16> deviceKey = {vendorId, productId};
    if (deviceDatabase.contains(deviceKey)) {
        return deviceDatabase[deviceKey];
    }

    // 返回空模型
    return GenesysModel();
}

void DScannerGenesysDriver::onUSBDataReceived(const QByteArray &data)
{
    Q_D(DScannerGenesysDriver);
    d->handleUSBData(data);
}

void DScannerGenesysDriver::onUSBError(const QString &error)
{
    Q_D(DScannerGenesysDriver);
    d->lastError = QStringLiteral("USB error: %1").arg(error);
    qCWarning(dscannerGenesys) << "USB error:" << error;
    emit errorOccurred(d->lastError);
}

// DScannerGenesysDriverPrivate实现
DScannerGenesysDriverPrivate::DScannerGenesysDriverPrivate(DScannerGenesysDriver *q)
    : QObject(q)
    , q_ptr(q)
    , initialized(false)
    , lastError()
    , usbComm(nullptr)
    , currentDevice(nullptr)
    , currentDeviceId()
    , isScanning(false)
    , registerCacheEnabled(true)
    , isCalibrated(false)
    , currentResolution(300)
    , currentColorMode(ColorMode::Color)
{
}

DScannerGenesysDriverPrivate::~DScannerGenesysDriverPrivate()
{
    if (currentDevice) {
        closeUSBDevice();
    }
    if (usbComm) {
        delete usbComm;
        usbComm = nullptr;
    }
}

bool DScannerGenesysDriverPrivate::initializeUSB()
{
    // 基础USB初始化实现
    return true;
}

void DScannerGenesysDriverPrivate::shutdownUSB()
{
    // USB关闭实现
}

bool DScannerGenesysDriverPrivate::loadDeviceDatabase()
{
    // 加载设备数据库的基础实现
    return true;
}

bool DScannerGenesysDriverPrivate::openUSBDevice(const QString &deviceId)
{
    // USB设备打开的基础实现
    Q_UNUSED(deviceId)
    return true;
}

void DScannerGenesysDriverPrivate::closeUSBDevice()
{
    // USB设备关闭实现
}

bool DScannerGenesysDriverPrivate::initializeDevice()
{
    // 设备初始化实现
    return true;
}

void DScannerGenesysDriverPrivate::shutdownDevice()
{
    // 设备关闭实现
}

DeviceInfo DScannerGenesysDriverPrivate::convertToDeviceInfo(const USBDeviceDescriptor &usbDevice, const GenesysModel &model)
{
    // 设备信息转换的基础实现
    DeviceInfo info;
    Q_UNUSED(usbDevice)
    Q_UNUSED(model)
    return info;
}

QStringList DScannerGenesysDriverPrivate::getSupportedDeviceNames() const
{
    // 返回支持的设备名称列表
    return QStringList() << "Canon LiDE" << "HP ScanJet" << "Plustek OpticSlim";
}

ScannerCapabilities DScannerGenesysDriverPrivate::buildCapabilities()
{
    // 构建扫描能力的基础实现
    ScannerCapabilities caps;
    return caps;
}

bool DScannerGenesysDriverPrivate::applyScanParameters(const ScanParameters &params)
{
    // 应用扫描参数的基础实现
    Q_UNUSED(params)
    return true;
}

bool DScannerGenesysDriverPrivate::startScanning()
{
    // 开始扫描的基础实现
    isScanning = true;
    return true;
}

void DScannerGenesysDriverPrivate::stopScanning()
{
    // 停止扫描的基础实现
    isScanning = false;
}

QImage DScannerGenesysDriverPrivate::performPreviewScan()
{
    // 预览扫描的基础实现
    return QImage();
}

bool DScannerGenesysDriverPrivate::performCalibration()
{
    // 校准的基础实现
    isCalibrated = true;
    return true;
}

bool DScannerGenesysDriverPrivate::initializeChipset()
{
    // 芯片组初始化的基础实现
    return true;
}

bool DScannerGenesysDriverPrivate::calibrateSensor()
{
    // 传感器校准的基础实现
    return true;
}

bool DScannerGenesysDriverPrivate::setScanArea(const ScanArea &area)
{
    // 设置扫描区域的基础实现
    Q_UNUSED(area)
    return true;
}

bool DScannerGenesysDriverPrivate::setScanResolution(int resolution)
{
    // 设置扫描分辨率的基础实现
    Q_UNUSED(resolution)
    return true;
}

bool DScannerGenesysDriverPrivate::setColorMode(ColorMode mode)
{
    // 设置颜色模式的基础实现
    Q_UNUSED(mode)
    return true;
}

void DScannerGenesysDriverPrivate::handleUSBData(const QByteArray &data)
{
    // USB数据处理的基础实现
    Q_UNUSED(data)
}

RegisterOperationResult DScannerGenesysDriverPrivate::performRegisterOperation(int address, bool isWrite, quint8 value, int count)
{
    // 寄存器操作的基础实现
    Q_UNUSED(address)
    Q_UNUSED(isWrite)
    Q_UNUSED(value)
    Q_UNUSED(count)
    RegisterOperationResult result;
    result.success = true;
    return result;
}

bool DScannerGenesysDriverPrivate::validateScanParameters(const ScanParameters &params)
{
    // 扫描参数验证的基础实现
    Q_UNUSED(params)
    return true;
}

GenesysSensor DScannerGenesysDriverPrivate::getDefaultSensor(GenesysChipset chipset)
{
    // 获取默认传感器的基础实现
    Q_UNUSED(chipset)
    GenesysSensor sensor;
    return sensor;
}

// 设备参数管理方法
bool DScannerGenesysDriverPrivate::setDeviceParameter(const QString &name, const QVariant &value)
{
    Q_UNUSED(name)
    Q_UNUSED(value)
    return true;
}

QVariant DScannerGenesysDriverPrivate::getDeviceParameter(const QString &name) const
{
    Q_UNUSED(name)
    return QVariant();
}

QStringList DScannerGenesysDriverPrivate::getDeviceParameterNames() const
{
    return QStringList() << "resolution" << "color_mode" << "scan_area";
}

// 寄存器操作方法
quint8 DScannerGenesysDriverPrivate::readRegister(int address)
{
    Q_UNUSED(address)
    return 0;
}

bool DScannerGenesysDriverPrivate::writeRegister(int address, quint8 value)
{
    Q_UNUSED(address)
    Q_UNUSED(value)
    return true;
}

QByteArray DScannerGenesysDriverPrivate::readRegisters(int startAddress, int count)
{
    Q_UNUSED(startAddress)
    Q_UNUSED(count)
    return QByteArray();
}

bool DScannerGenesysDriverPrivate::writeRegisters(int startAddress, const QByteArray &values)
{
    Q_UNUSED(startAddress)
    Q_UNUSED(values)
    return true;
}

// 芯片组特定初始化方法
bool DScannerGenesysDriverPrivate::initializeGL646()
{
    return true;
}

bool DScannerGenesysDriverPrivate::initializeGL841()
{
    return true;
}

bool DScannerGenesysDriverPrivate::initializeGL842()
{
    return true;
}

bool DScannerGenesysDriverPrivate::initializeGL843()
{
    return true;
}

bool DScannerGenesysDriverPrivate::initializeGL846()
{
    return true;
}

bool DScannerGenesysDriverPrivate::initializeGL847()
{
    return true;
}

// 传感器操作方法
bool DScannerGenesysDriverPrivate::configureSensor()
{
    return true;
}

bool DScannerGenesysDriverPrivate::startSensorCalibration()
{
    return true;
}

bool DScannerGenesysDriverPrivate::readSensorData(QByteArray &data)
{
    Q_UNUSED(data)
    return true;
}

// 马达控制方法
bool DScannerGenesysDriverPrivate::initializeMotor()
{
    return true;
}

bool DScannerGenesysDriverPrivate::moveToPosition(int position)
{
    Q_UNUSED(position)
    return true;
}

bool DScannerGenesysDriverPrivate::setMotorSpeed(int speed)
{
    Q_UNUSED(speed)
    return true;
}

// 灯管控制方法
bool DScannerGenesysDriverPrivate::turnOnLamp()
{
    return true;
}

void DScannerGenesysDriverPrivate::turnOffLamp()
{
    // 关闭灯管实现
}

bool DScannerGenesysDriverPrivate::isLampReady()
{
    return true;
}

// 槽函数实现
void DScannerGenesysDriverPrivate::onUSBDeviceConnected(const USBDeviceDescriptor &descriptor)
{
    Q_UNUSED(descriptor)
    // USB设备连接处理
}

void DScannerGenesysDriverPrivate::onUSBDeviceDisconnected(const QString &devicePath)
{
    Q_UNUSED(devicePath)
    // USB设备断开处理
}

void DScannerGenesysDriverPrivate::onUSBError(int errorCode, const QString &errorMessage)
{
    Q_UNUSED(errorCode)
    lastError = errorMessage;
}

#include "dscannergenesys.moc" 