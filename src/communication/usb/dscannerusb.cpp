// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerUSB.h"
#include "dscannerusb_p.h"

#include <QLoggingCategory>
#include <QMutexLocker>
#include <QTimer>
#include <QThread>
#include <QDebug>
#include <QCoreApplication>

// libusb包含（如果系统有的话）
#ifdef HAVE_LIBUSB
#include <libusb-1.0/libusb.h>
#else
// 如果没有libusb，定义一些基本常量
#define LIBUSB_SUCCESS 0
#define LIBUSB_ERROR_NOT_FOUND -5
#define LIBUSB_ERROR_ACCESS -3
#define LIBUSB_ERROR_NO_DEVICE -4
#define LIBUSB_TRANSFER_COMPLETED 0
#define LIBUSB_CLASS_PRINTER 7
#define LIBUSB_CLASS_MASS_STORAGE 8
#define LIBUSB_ENDPOINT_IN 0x80
#define LIBUSB_ENDPOINT_OUT 0x00
#define LIBUSB_TRANSFER_TYPE_CONTROL 0
#define LIBUSB_TRANSFER_TYPE_BULK 2
#define LIBUSB_TRANSFER_TYPE_INTERRUPT 3
#endif

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerUSB, "deepinscan.usb")

// DScannerUSB implementation
DScannerUSB::DScannerUSB(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerUSBPrivate(this))
{
    qCDebug(dscannerUSB) << "DScannerUSB created";
}

DScannerUSB::~DScannerUSB()
{
    Q_D(DScannerUSB);
    if (d->initialized) {
        shutdown();
    }
    delete d_ptr;
}

bool DScannerUSB::initialize()
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (d->initialized) {
        return true;
    }

    qCDebug(dscannerUSB) << "Initializing USB subsystem";

    if (!d->initializeLibUSB()) {
        qCWarning(dscannerUSB) << "Failed to initialize libusb";
        return false;
    }

    d->initialized = true;
    qCInfo(dscannerUSB) << "USB subsystem initialized successfully";

    // 启动设备监控
    d->deviceMonitor->startMonitoring();

    return true;
}

void DScannerUSB::shutdown()
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->initialized) {
        return;
    }

    qCDebug(dscannerUSB) << "Shutting down USB subsystem";

    // 停止设备监控
    d->deviceMonitor->stopMonitoring();

    // 关闭当前设备
    if (d->isUSBDeviceOpen()) {
        d->closeUSBDevice();
    }

    // 清理libusb
    d->shutdownLibUSB();

    d->initialized = false;
    qCInfo(dscannerUSB) << "USB subsystem shutdown completed";
}

bool DScannerUSB::isInitialized() const
{
    Q_D(const DScannerUSB);
    QMutexLocker locker(&d->usbMutex);
    return d->initialized;
}

QList<USBDeviceDescriptor> DScannerUSB::discoverDevices(quint16 vendorId, quint16 productId)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    QList<USBDeviceDescriptor> devices;

    if (!d->initialized) {
        qCWarning(dscannerUSB) << "USB subsystem not initialized";
        return devices;
    }

    qCDebug(dscannerUSB) << "Discovering USB devices, VID:" << QString("0x%1").arg(vendorId, 4, 16, QLatin1Char('0'))
                         << "PID:" << QString("0x%1").arg(productId, 4, 16, QLatin1Char('0'));

    devices = d->discoverUSBDevices(vendorId, productId);

    qCInfo(dscannerUSB) << "Found" << devices.size() << "USB devices";

    // 发出设备发现信号
    for (const auto &device : devices) {
        emit deviceConnected(device);
    }

    return devices;
}

bool DScannerUSB::openDevice(quint16 vendorId, quint16 productId, const QString &serialNumber)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->initialized) {
        d->setLastError(LIBUSB_ERROR_NOT_FOUND, QStringLiteral("USB subsystem not initialized"));
        return false;
    }

    qCDebug(dscannerUSB) << "Opening USB device VID:" << QString("0x%1").arg(vendorId, 4, 16, QLatin1Char('0'))
                         << "PID:" << QString("0x%1").arg(productId, 4, 16, QLatin1Char('0'))
                         << "Serial:" << serialNumber;

    if (d->openUSBDevice(vendorId, productId, serialNumber)) {
        qCInfo(dscannerUSB) << "USB device opened successfully";
        return true;
    } else {
        qCWarning(dscannerUSB) << "Failed to open USB device:" << d->getLastUSBError();
        return false;
    }
}

bool DScannerUSB::openDeviceByPath(const QString &devicePath)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->initialized) {
        d->setLastError(LIBUSB_ERROR_NOT_FOUND, QStringLiteral("USB subsystem not initialized"));
        return false;
    }

    qCDebug(dscannerUSB) << "Opening USB device by path:" << devicePath;

    if (d->openUSBDeviceByPath(devicePath)) {
        qCInfo(dscannerUSB) << "USB device opened successfully";
        return true;
    } else {
        qCWarning(dscannerUSB) << "Failed to open USB device:" << d->getLastUSBError();
        return false;
    }
}

void DScannerUSB::closeDevice()
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        return;
    }

    qCDebug(dscannerUSB) << "Closing USB device";
    d->closeUSBDevice();
    qCInfo(dscannerUSB) << "USB device closed";
}

bool DScannerUSB::isDeviceOpen() const
{
    Q_D(const DScannerUSB);
    QMutexLocker locker(&d->usbMutex);
    return d->isUSBDeviceOpen();
}

bool DScannerUSB::setConfiguration(quint8 configuration)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return false;
    }

    return d->setUSBConfiguration(configuration);
}

bool DScannerUSB::claimInterface(quint8 interfaceNumber)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return false;
    }

    return d->claimUSBInterface(interfaceNumber);
}

bool DScannerUSB::releaseInterface(quint8 interfaceNumber)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        return false;
    }

    return d->releaseUSBInterface(interfaceNumber);
}

bool DScannerUSB::setInterfaceAltSetting(quint8 interfaceNumber, quint8 alternateSetting)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return false;
    }

    return d->setUSBInterfaceAltSetting(interfaceNumber, alternateSetting);
}

int DScannerUSB::controlTransfer(quint8 requestType, quint8 request, quint16 value, quint16 index,
                                 QByteArray &data, int timeout)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return -1;
    }

    USBTransferRequest request_struct;
    request_struct.type = USBTransferType::Control;
    request_struct.requestType = requestType;
    request_struct.request = request;
    request_struct.value = value;
    request_struct.index = index;
    request_struct.data = data;
    request_struct.timeout = timeout;
    request_struct.isInput = (requestType & LIBUSB_ENDPOINT_IN) != 0;

    USBTransferResult result = d->performControlTransfer(request_struct);
    
    if (result.status == LIBUSB_SUCCESS) {
        data = result.data;
        return result.actualLength;
    } else {
        return -1;
    }
}

int DScannerUSB::bulkTransferOut(quint8 endpoint, const QByteArray &data, int timeout)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return -1;
    }

    USBTransferRequest request;
    request.type = USBTransferType::Bulk;
    request.endpoint = endpoint;
    request.data = data;
    request.timeout = timeout;
    request.isInput = false;

    USBTransferResult result = d->performBulkTransfer(request);
    
    if (result.status == LIBUSB_SUCCESS) {
        emit transferCompleted(endpoint, result.actualLength);
        return result.actualLength;
    } else {
        emit errorOccurred(result.status, result.errorMessage);
        return -1;
    }
}

QByteArray DScannerUSB::bulkTransferIn(quint8 endpoint, int maxLength, int timeout)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    QByteArray data;

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return data;
    }

    USBTransferRequest request;
    request.type = USBTransferType::Bulk;
    request.endpoint = endpoint | LIBUSB_ENDPOINT_IN;
    request.data.resize(maxLength);
    request.timeout = timeout;
    request.isInput = true;

    USBTransferResult result = d->performBulkTransfer(request);
    
    if (result.status == LIBUSB_SUCCESS) {
        data = result.data;
        emit dataReceived(endpoint, data);
        emit transferCompleted(endpoint, result.actualLength);
    } else {
        emit errorOccurred(result.status, result.errorMessage);
    }

    return data;
}

int DScannerUSB::interruptTransferOut(quint8 endpoint, const QByteArray &data, int timeout)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return -1;
    }

    USBTransferRequest request;
    request.type = USBTransferType::Interrupt;
    request.endpoint = endpoint;
    request.data = data;
    request.timeout = timeout;
    request.isInput = false;

    USBTransferResult result = d->performInterruptTransfer(request);
    
    if (result.status == LIBUSB_SUCCESS) {
        emit transferCompleted(endpoint, result.actualLength);
        return result.actualLength;
    } else {
        emit errorOccurred(result.status, result.errorMessage);
        return -1;
    }
}

QByteArray DScannerUSB::interruptTransferIn(quint8 endpoint, int maxLength, int timeout)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    QByteArray data;

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return data;
    }

    USBTransferRequest request;
    request.type = USBTransferType::Interrupt;
    request.endpoint = endpoint | LIBUSB_ENDPOINT_IN;
    request.data.resize(maxLength);
    request.timeout = timeout;
    request.isInput = true;

    USBTransferResult result = d->performInterruptTransfer(request);
    
    if (result.status == LIBUSB_SUCCESS) {
        data = result.data;
        emit dataReceived(endpoint, data);
        emit transferCompleted(endpoint, result.actualLength);
    } else {
        emit errorOccurred(result.status, result.errorMessage);
    }

    return data;
}

USBDeviceDescriptor DScannerUSB::getCurrentDeviceDescriptor() const
{
    Q_D(const DScannerUSB);
    QMutexLocker locker(&d->usbMutex);
    return d->getCurrentUSBDeviceDescriptor();
}

QString DScannerUSB::getStringDescriptor(quint8 index) const
{
    Q_D(const DScannerUSB);
    QMutexLocker locker(&d->usbMutex);
    return d->getUSBStringDescriptor(index);
}

bool DScannerUSB::resetDevice()
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return false;
    }

    return d->resetUSBDevice();
}

bool DScannerUSB::clearHalt(quint8 endpoint)
{
    Q_D(DScannerUSB);
    QMutexLocker locker(&d->usbMutex);

    if (!d->isUSBDeviceOpen()) {
        d->setLastError(LIBUSB_ERROR_NO_DEVICE, QStringLiteral("No device open"));
        return false;
    }

    return d->clearUSBHalt(endpoint);
}

QString DScannerUSB::lastError() const
{
    Q_D(const DScannerUSB);
    QMutexLocker locker(&d->usbMutex);
    return d->getLastUSBError();
}

int DScannerUSB::lastErrorCode() const
{
    Q_D(const DScannerUSB);
    QMutexLocker locker(&d->usbMutex);
    return d->getLastUSBErrorCode();
}

QString DScannerUSB::transferTypeName(USBTransferType type)
{
    switch (type) {
    case USBTransferType::Control:
        return QStringLiteral("Control");
    case USBTransferType::Isochronous:
        return QStringLiteral("Isochronous");
    case USBTransferType::Bulk:
        return QStringLiteral("Bulk");
    case USBTransferType::Interrupt:
        return QStringLiteral("Interrupt");
    default:
        return QStringLiteral("Unknown");
    }
}

bool DScannerUSB::isScannerDevice(const USBDeviceDescriptor &descriptor)
{
    // 检查设备类是否为打印机类（扫描仪通常归类为打印机）
    if (descriptor.deviceClass == LIBUSB_CLASS_PRINTER) {
        return true;
    }

    // 检查常见的扫描仪厂商ID
    QList<quint16> scannerVendors = {
        0x04A9, // Canon
        0x04B8, // Epson
        0x03F0, // HP
        0x04E8, // Samsung
        0x04A5, // Acer
        0x055F, // Mustek
        0x0A17, // Pentax
        0x1606, // Umax
        0x04B4, // Cypress (used by many scanner manufacturers)
        0x0638, // Avision
        0x0A53, // PLUSTEK
        0x1083, // Fujitsu
    };

    return scannerVendors.contains(descriptor.vendorId);
}

QString DScannerUSB::formatDevicePath(quint8 busNumber, quint8 deviceAddress)
{
    return QStringLiteral("usb:%1:%2").arg(busNumber).arg(deviceAddress);
}

void DScannerUSB::processUSBEvents()
{
    Q_D(DScannerUSB);
    d->processUSBEvents();
}

void DScannerUSB::checkDeviceStatus()
{
    Q_D(DScannerUSB);
    
    if (!d->isUSBDeviceOpen()) {
        return;
    }

    // 检查设备是否仍然连接
    // 这里可以实现设备状态检查逻辑
}

// DScannerUSBPrivate implementation
DScannerUSBPrivate::DScannerUSBPrivate(DScannerUSB *q)
    : QObject(q)
    , q_ptr(q)
    , usbContext(nullptr)
    , deviceHandle(nullptr)
    , currentDevice(nullptr)
    , initialized(false)
    , lastErrorCode(0)
    , eventTimer(new QTimer(this))
    , eventThread(nullptr)
    , deviceMonitor(new USBDeviceMonitor(this))
{
    // 设置事件处理定时器
    eventTimer->setInterval(10); // 10ms间隔处理USB事件
    connect(eventTimer, &QTimer::timeout, this, &DScannerUSBPrivate::processUSBEvents);

    // 连接设备监控信号
    connect(deviceMonitor, &USBDeviceMonitor::deviceConnected,
            this, &DScannerUSBPrivate::onDeviceConnected);
    connect(deviceMonitor, &USBDeviceMonitor::deviceDisconnected,
            this, &DScannerUSBPrivate::onDeviceDisconnected);
}

DScannerUSBPrivate::~DScannerUSBPrivate()
{
    if (initialized) {
        shutdownLibUSB();
    }
}

bool DScannerUSBPrivate::initializeLibUSB()
{
#ifdef HAVE_LIBUSB
    int result = libusb_init(&usbContext);
    if (result != LIBUSB_SUCCESS) {
        setLastError(result, QStringLiteral("Failed to initialize libusb: %1").arg(libusb_error_name(result)));
        return false;
    }

    // 设置调试级别
    libusb_set_option(usbContext, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    // 启动事件处理
    eventTimer->start();

    qCDebug(dscannerUSB) << "libusb initialized successfully";
    return true;
#else
    setLastError(-1, QStringLiteral("libusb not available"));
    qCWarning(dscannerUSB) << "libusb not available, USB functionality disabled";
    return false;
#endif
}

void DScannerUSBPrivate::shutdownLibUSB()
{
    eventTimer->stop();

    if (deviceHandle) {
        closeUSBDevice();
    }

#ifdef HAVE_LIBUSB
    if (usbContext) {
        libusb_exit(usbContext);
        usbContext = nullptr;
    }
#endif

    qCDebug(dscannerUSB) << "libusb shutdown completed";
}

bool DScannerUSBPrivate::isLibUSBInitialized() const
{
    return usbContext != nullptr;
}

QList<USBDeviceDescriptor> DScannerUSBPrivate::discoverUSBDevices(quint16 vendorId, quint16 productId)
{
    QList<USBDeviceDescriptor> devices;

#ifdef HAVE_LIBUSB
    if (!usbContext) {
        return devices;
    }

    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(usbContext, &deviceList);
    
    if (deviceCount < 0) {
        setLastError(deviceCount, QStringLiteral("Failed to get device list"));
        return devices;
    }

    for (ssize_t i = 0; i < deviceCount; ++i) {
        USBDeviceDescriptor descriptor = convertToDeviceDescriptor(deviceList[i]);
        
        // 过滤设备
        if (vendorId != 0 && descriptor.vendorId != vendorId) {
            continue;
        }
        if (productId != 0 && descriptor.productId != productId) {
            continue;
        }

        devices.append(descriptor);
    }

    libusb_free_device_list(deviceList, 1);
#else
    Q_UNUSED(vendorId)
    Q_UNUSED(productId)
#endif

    return devices;
}

bool DScannerUSBPrivate::openUSBDevice(quint16 vendorId, quint16 productId, const QString &serialNumber)
{
#ifdef HAVE_LIBUSB
    if (!usbContext) {
        setLastError(LIBUSB_ERROR_NOT_FOUND, QStringLiteral("libusb not initialized"));
        return false;
    }

    if (deviceHandle) {
        closeUSBDevice();
    }

    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(usbContext, &deviceList);
    
    if (deviceCount < 0) {
        setLastError(deviceCount, QStringLiteral("Failed to get device list"));
        return false;
    }

    bool found = false;
    for (ssize_t i = 0; i < deviceCount && !found; ++i) {
        libusb_device_descriptor desc;
        int result = libusb_get_device_descriptor(deviceList[i], &desc);
        
        if (result != LIBUSB_SUCCESS) {
            continue;
        }

        if (desc.idVendor == vendorId && desc.idProduct == productId) {
            // 检查序列号（如果指定）
            if (!serialNumber.isEmpty()) {
                libusb_device_handle *tempHandle;
                result = libusb_open(deviceList[i], &tempHandle);
                if (result == LIBUSB_SUCCESS) {
                    unsigned char serialBuffer[256];
                    result = libusb_get_string_descriptor_ascii(tempHandle, desc.iSerialNumber,
                                                               serialBuffer, sizeof(serialBuffer));
                    libusb_close(tempHandle);
                    
                    if (result > 0) {
                        QString deviceSerial = QString::fromLatin1(reinterpret_cast<char*>(serialBuffer));
                        if (deviceSerial != serialNumber) {
                            continue;
                        }
                    }
                }
            }

            // 打开设备
            result = libusb_open(deviceList[i], &deviceHandle);
            if (result == LIBUSB_SUCCESS) {
                currentDevice = libusb_ref_device(deviceList[i]);
                currentDescriptor = convertToDeviceDescriptor(deviceList[i]);
                found = true;
                qCDebug(dscannerUSB) << "USB device opened successfully";
            } else {
                setLastError(result, QStringLiteral("Failed to open USB device: %1").arg(libusb_error_name(result)));
            }
        }
    }

    libusb_free_device_list(deviceList, 1);
    return found;
#else
    Q_UNUSED(vendorId)
    Q_UNUSED(productId)
    Q_UNUSED(serialNumber)
    setLastError(-1, QStringLiteral("libusb not available"));
    return false;
#endif
}

bool DScannerUSBPrivate::openUSBDeviceByPath(const QString &devicePath)
{
    // 解析设备路径 "usb:bus:device"
    QStringList parts = devicePath.split(':');
    if (parts.size() != 3 || parts[0] != "usb") {
        setLastError(-1, QStringLiteral("Invalid device path format"));
        return false;
    }

    bool ok1, ok2;
    quint8 busNumber = parts[1].toUInt(&ok1);
    quint8 deviceAddress = parts[2].toUInt(&ok2);
    
    if (!ok1 || !ok2) {
        setLastError(-1, QStringLiteral("Invalid device path numbers"));
        return false;
    }

#ifdef HAVE_LIBUSB
    if (!usbContext) {
        setLastError(LIBUSB_ERROR_NOT_FOUND, QStringLiteral("libusb not initialized"));
        return false;
    }

    if (deviceHandle) {
        closeUSBDevice();
    }

    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(usbContext, &deviceList);
    
    if (deviceCount < 0) {
        setLastError(deviceCount, QStringLiteral("Failed to get device list"));
        return false;
    }

    bool found = false;
    for (ssize_t i = 0; i < deviceCount && !found; ++i) {
        if (libusb_get_bus_number(deviceList[i]) == busNumber &&
            libusb_get_device_address(deviceList[i]) == deviceAddress) {
            
            int result = libusb_open(deviceList[i], &deviceHandle);
            if (result == LIBUSB_SUCCESS) {
                currentDevice = libusb_ref_device(deviceList[i]);
                currentDescriptor = convertToDeviceDescriptor(deviceList[i]);
                found = true;
                qCDebug(dscannerUSB) << "USB device opened by path:" << devicePath;
            } else {
                setLastError(result, QStringLiteral("Failed to open USB device: %1").arg(libusb_error_name(result)));
            }
        }
    }

    libusb_free_device_list(deviceList, 1);
    return found;
#else
    Q_UNUSED(busNumber)
    Q_UNUSED(deviceAddress)
    setLastError(-1, QStringLiteral("libusb not available"));
    return false;
#endif
}

void DScannerUSBPrivate::closeUSBDevice()
{
    // 释放所有声明的接口
    for (quint8 interfaceNumber : claimedInterfaces) {
        releaseUSBInterface(interfaceNumber);
    }
    claimedInterfaces.clear();

#ifdef HAVE_LIBUSB
    if (deviceHandle) {
        libusb_close(deviceHandle);
        deviceHandle = nullptr;
    }

    if (currentDevice) {
        libusb_unref_device(currentDevice);
        currentDevice = nullptr;
    }
#endif

    currentDescriptor = USBDeviceDescriptor();
    qCDebug(dscannerUSB) << "USB device closed";
}

bool DScannerUSBPrivate::isUSBDeviceOpen() const
{
    return deviceHandle != nullptr;
}

bool DScannerUSBPrivate::setUSBConfiguration(quint8 configuration)
{
#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        return false;
    }

    int result = libusb_set_configuration(deviceHandle, configuration);
    if (result != LIBUSB_SUCCESS) {
        setLastError(result, QStringLiteral("Failed to set configuration: %1").arg(libusb_error_name(result)));
        return false;
    }

    return true;
#else
    Q_UNUSED(configuration)
    return false;
#endif
}

bool DScannerUSBPrivate::claimUSBInterface(quint8 interfaceNumber)
{
#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        return false;
    }

    int result = libusb_claim_interface(deviceHandle, interfaceNumber);
    if (result != LIBUSB_SUCCESS) {
        setLastError(result, QStringLiteral("Failed to claim interface: %1").arg(libusb_error_name(result)));
        return false;
    }

    if (!claimedInterfaces.contains(interfaceNumber)) {
        claimedInterfaces.append(interfaceNumber);
    }

    return true;
#else
    Q_UNUSED(interfaceNumber)
    return false;
#endif
}

bool DScannerUSBPrivate::releaseUSBInterface(quint8 interfaceNumber)
{
#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        return false;
    }

    int result = libusb_release_interface(deviceHandle, interfaceNumber);
    if (result != LIBUSB_SUCCESS) {
        setLastError(result, QStringLiteral("Failed to release interface: %1").arg(libusb_error_name(result)));
        return false;
    }

    claimedInterfaces.removeAll(interfaceNumber);
    return true;
#else
    Q_UNUSED(interfaceNumber)
    return false;
#endif
}

bool DScannerUSBPrivate::setUSBInterfaceAltSetting(quint8 interfaceNumber, quint8 alternateSetting)
{
#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        return false;
    }

    int result = libusb_set_interface_alt_setting(deviceHandle, interfaceNumber, alternateSetting);
    if (result != LIBUSB_SUCCESS) {
        setLastError(result, QStringLiteral("Failed to set alt setting: %1").arg(libusb_error_name(result)));
        return false;
    }

    return true;
#else
    Q_UNUSED(interfaceNumber)
    Q_UNUSED(alternateSetting)
    return false;
#endif
}

USBTransferResult DScannerUSBPrivate::performControlTransfer(const USBTransferRequest &request)
{
    USBTransferResult result;

#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        result.status = LIBUSB_ERROR_NO_DEVICE;
        result.errorMessage = QStringLiteral("No device open");
        return result;
    }

    QByteArray buffer = request.data;
    if (request.isInput && buffer.size() == 0) {
        buffer.resize(1024); // 默认缓冲区大小
    }

    int transferred = libusb_control_transfer(
        deviceHandle,
        request.requestType,
        request.request,
        request.value,
        request.index,
        reinterpret_cast<unsigned char*>(buffer.data()),
        buffer.size(),
        request.timeout
    );

    if (transferred >= 0) {
        result.status = LIBUSB_SUCCESS;
        result.actualLength = transferred;
        if (request.isInput) {
            result.data = buffer.left(transferred);
        } else {
            result.data = request.data;
        }
    } else {
        result.status = transferred;
        result.errorMessage = QStringLiteral("Control transfer failed: %1").arg(libusb_error_name(transferred));
        setLastError(transferred, result.errorMessage);
    }
#else
    Q_UNUSED(request)
    result.status = -1;
    result.errorMessage = QStringLiteral("libusb not available");
#endif

    return result;
}

USBTransferResult DScannerUSBPrivate::performBulkTransfer(const USBTransferRequest &request)
{
    USBTransferResult result;

#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        result.status = LIBUSB_ERROR_NO_DEVICE;
        result.errorMessage = QStringLiteral("No device open");
        return result;
    }

    QByteArray buffer = request.data;
    int transferred = 0;

    int libusb_result = libusb_bulk_transfer(
        deviceHandle,
        request.endpoint,
        reinterpret_cast<unsigned char*>(buffer.data()),
        buffer.size(),
        &transferred,
        request.timeout
    );

    result.status = libusb_result;
    result.actualLength = transferred;

    if (libusb_result == LIBUSB_SUCCESS) {
        if (request.isInput) {
            result.data = buffer.left(transferred);
        } else {
            result.data = request.data;
        }
    } else {
        result.errorMessage = QStringLiteral("Bulk transfer failed: %1").arg(libusb_error_name(libusb_result));
        setLastError(libusb_result, result.errorMessage);
    }
#else
    Q_UNUSED(request)
    result.status = -1;
    result.errorMessage = QStringLiteral("libusb not available");
#endif

    return result;
}

USBTransferResult DScannerUSBPrivate::performInterruptTransfer(const USBTransferRequest &request)
{
    USBTransferResult result;

#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        result.status = LIBUSB_ERROR_NO_DEVICE;
        result.errorMessage = QStringLiteral("No device open");
        return result;
    }

    QByteArray buffer = request.data;
    int transferred = 0;

    int libusb_result = libusb_interrupt_transfer(
        deviceHandle,
        request.endpoint,
        reinterpret_cast<unsigned char*>(buffer.data()),
        buffer.size(),
        &transferred,
        request.timeout
    );

    result.status = libusb_result;
    result.actualLength = transferred;

    if (libusb_result == LIBUSB_SUCCESS) {
        if (request.isInput) {
            result.data = buffer.left(transferred);
        } else {
            result.data = request.data;
        }
    } else {
        result.errorMessage = QStringLiteral("Interrupt transfer failed: %1").arg(libusb_error_name(libusb_result));
        setLastError(libusb_result, result.errorMessage);
    }
#else
    Q_UNUSED(request)
    result.status = -1;
    result.errorMessage = QStringLiteral("libusb not available");
#endif

    return result;
}

USBDeviceDescriptor DScannerUSBPrivate::getCurrentUSBDeviceDescriptor() const
{
    return currentDescriptor;
}

QString DScannerUSBPrivate::getUSBStringDescriptor(quint8 index) const
{
    QString result;

#ifdef HAVE_LIBUSB
    if (!deviceHandle || index == 0) {
        return result;
    }

    unsigned char buffer[256];
    int length = libusb_get_string_descriptor_ascii(deviceHandle, index, buffer, sizeof(buffer));
    
    if (length > 0) {
        result = QString::fromLatin1(reinterpret_cast<char*>(buffer), length);
    }
#else
    Q_UNUSED(index)
#endif

    return result;
}

bool DScannerUSBPrivate::resetUSBDevice()
{
#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        return false;
    }

    int result = libusb_reset_device(deviceHandle);
    if (result != LIBUSB_SUCCESS) {
        setLastError(result, QStringLiteral("Failed to reset device: %1").arg(libusb_error_name(result)));
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool DScannerUSBPrivate::clearUSBHalt(quint8 endpoint)
{
#ifdef HAVE_LIBUSB
    if (!deviceHandle) {
        return false;
    }

    int result = libusb_clear_halt(deviceHandle, endpoint);
    if (result != LIBUSB_SUCCESS) {
        setLastError(result, QStringLiteral("Failed to clear halt: %1").arg(libusb_error_name(result)));
        return false;
    }

    return true;
#else
    Q_UNUSED(endpoint)
    return false;
#endif
}

QString DScannerUSBPrivate::getLastUSBError() const
{
    return lastErrorMessage;
}

int DScannerUSBPrivate::getLastUSBErrorCode() const
{
    return lastErrorCode;
}

void DScannerUSBPrivate::setLastError(int errorCode, const QString &errorMessage)
{
    lastErrorCode = errorCode;
    lastErrorMessage = errorMessage;
}

USBDeviceDescriptor DScannerUSBPrivate::convertToDeviceDescriptor(libusb_device *device)
{
    USBDeviceDescriptor descriptor;

#ifdef HAVE_LIBUSB
    if (!device) {
        return descriptor;
    }

    libusb_device_descriptor desc;
    int result = libusb_get_device_descriptor(device, &desc);
    
    if (result != LIBUSB_SUCCESS) {
        return descriptor;
    }

    descriptor.vendorId = desc.idVendor;
    descriptor.productId = desc.idProduct;
    descriptor.deviceVersion = desc.bcdDevice;
    descriptor.deviceClass = desc.bDeviceClass;
    descriptor.deviceSubClass = desc.bDeviceSubClass;
    descriptor.deviceProtocol = desc.bDeviceProtocol;
    descriptor.maxPacketSize0 = desc.bMaxPacketSize0;
    descriptor.busNumber = libusb_get_bus_number(device);
    descriptor.deviceAddress = libusb_get_device_address(device);
    descriptor.devicePath = DScannerUSB::formatDevicePath(descriptor.busNumber, descriptor.deviceAddress);

    // 获取字符串描述符（需要打开设备）
    libusb_device_handle *handle;
    result = libusb_open(device, &handle);
    if (result == LIBUSB_SUCCESS) {
        unsigned char buffer[256];
        
        if (desc.iManufacturer > 0) {
            int length = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buffer, sizeof(buffer));
            if (length > 0) {
                descriptor.manufacturer = QString::fromLatin1(reinterpret_cast<char*>(buffer), length);
            }
        }
        
        if (desc.iProduct > 0) {
            int length = libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, sizeof(buffer));
            if (length > 0) {
                descriptor.product = QString::fromLatin1(reinterpret_cast<char*>(buffer), length);
            }
        }
        
        if (desc.iSerialNumber > 0) {
            int length = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, buffer, sizeof(buffer));
            if (length > 0) {
                descriptor.serialNumber = QString::fromLatin1(reinterpret_cast<char*>(buffer), length);
            }
        }

        libusb_close(handle);
    }

    // 获取配置信息
    for (int i = 0; i < desc.bNumConfigurations; ++i) {
        libusb_config_descriptor *config;
        result = libusb_get_config_descriptor(device, i, &config);
        if (result == LIBUSB_SUCCESS) {
            descriptor.configurations.append(convertToConfiguration(config));
            libusb_free_config_descriptor(config);
        }
    }
#else
    Q_UNUSED(device)
#endif

    return descriptor;
}

USBConfiguration DScannerUSBPrivate::convertToConfiguration(const libusb_config_descriptor *config)
{
    USBConfiguration configuration;

#ifdef HAVE_LIBUSB
    if (!config) {
        return configuration;
    }

    configuration.configurationValue = config->bConfigurationValue;
    configuration.attributes = config->bmAttributes;
    configuration.maxPower = config->bMaxPower;

    // 转换接口
    for (int i = 0; i < config->bNumInterfaces; ++i) {
        const libusb_interface *interface = &config->interface[i];
        for (int j = 0; j < interface->num_altsetting; ++j) {
            configuration.interfaces.append(convertToInterface(&interface->altsetting[j]));
        }
    }
#else
    Q_UNUSED(config)
#endif

    return configuration;
}

USBInterface DScannerUSBPrivate::convertToInterface(const libusb_interface_descriptor *interface)
{
    USBInterface usbInterface;

#ifdef HAVE_LIBUSB
    if (!interface) {
        return usbInterface;
    }

    usbInterface.interfaceNumber = interface->bInterfaceNumber;
    usbInterface.alternateSetting = interface->bAlternateSetting;
    usbInterface.interfaceClass = interface->bInterfaceClass;
    usbInterface.interfaceSubClass = interface->bInterfaceSubClass;
    usbInterface.interfaceProtocol = interface->bInterfaceProtocol;

    // 转换端点
    for (int i = 0; i < interface->bNumEndpoints; ++i) {
        usbInterface.endpoints.append(convertToEndpoint(&interface->endpoint[i]));
    }
#else
    Q_UNUSED(interface)
#endif

    return usbInterface;
}

USBEndpoint DScannerUSBPrivate::convertToEndpoint(const libusb_endpoint_descriptor *endpoint)
{
    USBEndpoint usbEndpoint;

#ifdef HAVE_LIBUSB
    if (!endpoint) {
        return usbEndpoint;
    }

    usbEndpoint.address = endpoint->bEndpointAddress;
    usbEndpoint.maxPacketSize = endpoint->wMaxPacketSize;
    usbEndpoint.interval = endpoint->bInterval;
    usbEndpoint.isInput = (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0;

    // 转换传输类型
    switch (endpoint->bmAttributes & 0x03) {
    case LIBUSB_TRANSFER_TYPE_CONTROL:
        usbEndpoint.type = USBTransferType::Control;
        break;
    case LIBUSB_TRANSFER_TYPE_BULK:
        usbEndpoint.type = USBTransferType::Bulk;
        break;
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
        usbEndpoint.type = USBTransferType::Interrupt;
        break;
    default:
        usbEndpoint.type = USBTransferType::Bulk;
        break;
    }
#else
    Q_UNUSED(endpoint)
#endif

    return usbEndpoint;
}

void DScannerUSBPrivate::processAsyncTransfers()
{
    // 实现异步传输处理 - 完整的异步USB传输
    qCDebug(dscannerUSB) << "处理异步USB传输";
    
    if (!m_context || !m_deviceHandle) {
        qCWarning(dscannerUSB) << "USB上下文或设备句柄无效";
        return;
    }
    
    // 创建异步传输结构
    libusb_transfer *transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        qCWarning(dscannerUSB) << "无法分配USB传输结构";
        return;
    }
    
    // 准备传输数据
    unsigned char *buffer = new unsigned char[USB_BUFFER_SIZE];
    
    // 配置批量传输
    libusb_fill_bulk_transfer(transfer, 
                             m_deviceHandle,
                             m_bulkInEndpoint,  // 输入端点
                             buffer,
                             USB_BUFFER_SIZE,
                             [](libusb_transfer *transfer) {
                                 // 异步传输完成回调
                                 DScannerUSBPrivate *usbPrivate = static_cast<DScannerUSBPrivate*>(transfer->user_data);
                                 if (usbPrivate) {
                                     usbPrivate->handleTransferCompletion(transfer);
                                 }
                             },
                             this,              // 用户数据
                             USB_TRANSFER_TIMEOUT);
    
    // 提交异步传输
    int result = libusb_submit_transfer(transfer);
    if (result != LIBUSB_SUCCESS) {
        qCWarning(dscannerUSB) << "提交异步传输失败:" << libusb_error_name(result);
        delete[] buffer;
        libusb_free_transfer(transfer);
        return;
    }
    
    // 添加到活动传输列表
    m_activeTransfers.append(transfer);
    
    qCDebug(dscannerUSB) << "异步USB传输已提交";
}

void DScannerUSBPrivate::handleTransferComplete(libusb_transfer *transfer)
{
    Q_UNUSED(transfer)
    // 实现传输完成处理 - 完整的传输完成处理
    qCDebug(dscannerUSB) << "处理传输完成事件";
    
    if (!transfer) {
        qCWarning(dscannerUSB) << "传输结构为空";
        return;
    }
    
    // 从活动传输列表中移除
    m_activeTransfers.removeOne(transfer);
    
    // 检查传输状态
    switch (transfer->status) {
        case LIBUSB_TRANSFER_COMPLETED:
            qCDebug(dscannerUSB) << "USB传输成功完成，接收数据:" << transfer->actual_length << "字节";
            
            // 处理接收到的数据
            if (transfer->actual_length > 0) {
                QByteArray data(reinterpret_cast<char*>(transfer->buffer), transfer->actual_length);
                
                // 添加到接收缓冲区
                {
                    QMutexLocker locker(&m_dataMutex);
                    m_receivedData.append(data);
                }
                
                // 发出数据就绪信号
                emit q_ptr->dataReceived(data);
                
                // 更新统计信息
                m_stats.totalBytesReceived += transfer->actual_length;
                m_stats.successfulTransfers++;
                
                qCDebug(dscannerUSB) << "数据已添加到缓冲区，总大小:" << m_receivedData.size();
            }
            break;
            
        case LIBUSB_TRANSFER_CANCELLED:
            qCDebug(dscannerUSB) << "USB传输已取消";
            m_stats.cancelledTransfers++;
            break;
            
        case LIBUSB_TRANSFER_NO_DEVICE:
            qCWarning(dscannerUSB) << "USB设备已断开连接";
            m_stats.errorTransfers++;
            emit q_ptr->deviceDisconnected();
            break;
            
        case LIBUSB_TRANSFER_TIMED_OUT:
            qCWarning(dscannerUSB) << "USB传输超时";
            m_stats.errorTransfers++;
            m_stats.timeoutTransfers++;
            
            // 如果需要，可以重新提交传输
            if (m_autoRetry && m_stats.timeoutTransfers < MAX_RETRY_COUNT) {
                qCDebug(dscannerUSB) << "重新提交传输";
                int result = libusb_submit_transfer(transfer);
                if (result == LIBUSB_SUCCESS) {
                    m_activeTransfers.append(transfer);
                    return; // 不释放传输结构
                }
            }
            break;
            
        case LIBUSB_TRANSFER_STALL:
            qCWarning(dscannerUSB) << "USB传输失速";
            m_stats.errorTransfers++;
            
            // 清除失速状态
            libusb_clear_halt(m_deviceHandle, transfer->endpoint);
            break;
            
        case LIBUSB_TRANSFER_OVERFLOW:
            qCWarning(dscannerUSB) << "USB传输溢出";
            m_stats.errorTransfers++;
            break;
            
        case LIBUSB_TRANSFER_ERROR:
        default:
            qCWarning(dscannerUSB) << "USB传输错误，状态:" << transfer->status;
            m_stats.errorTransfers++;
            emit q_ptr->transferError(transfer->status);
            break;
    }
    
    // 清理传输资源
    if (transfer->buffer) {
        delete[] transfer->buffer;
    }
    libusb_free_transfer(transfer);
    
    qCDebug(dscannerUSB) << "传输完成处理结束，活动传输数量:" << m_activeTransfers.size();
}

void DScannerUSBPrivate::processUSBEvents()
{
#ifdef HAVE_LIBUSB
    if (usbContext) {
        struct timeval timeout = {0, 0}; // 非阻塞
        libusb_handle_events_timeout(usbContext, &timeout);
    }
#endif
}

void DScannerUSBPrivate::onDeviceConnected(const USBDeviceDescriptor &descriptor)
{
    qCDebug(dscannerUSB) << "USB device connected:" << descriptor.devicePath;
    emit q_ptr->deviceConnected(descriptor);
}

void DScannerUSBPrivate::onDeviceDisconnected(const QString &devicePath)
{
    qCDebug(dscannerUSB) << "USB device disconnected:" << devicePath;
    
    // 如果当前打开的设备被断开，关闭它
    if (isUSBDeviceOpen() && currentDescriptor.devicePath == devicePath) {
        closeUSBDevice();
    }
    
    emit q_ptr->deviceDisconnected(devicePath);
}

// USBDeviceMonitor implementation
USBDeviceMonitor::USBDeviceMonitor(QObject *parent)
    : QThread(parent)
    , monitoring(false)
{
}

USBDeviceMonitor::~USBDeviceMonitor()
{
    stopMonitoring();
    wait();
}

void USBDeviceMonitor::startMonitoring()
{
    QMutexLocker locker(&monitorMutex);
    if (!monitoring) {
        monitoring = true;
        start();
    }
}

void USBDeviceMonitor::stopMonitoring()
{
    QMutexLocker locker(&monitorMutex);
    if (monitoring) {
        monitoring = false;
        requestInterruption();
    }
}

void USBDeviceMonitor::run()
{
    qCDebug(dscannerUSB) << "USB device monitor started";

    while (!isInterruptionRequested()) {
        QList<USBDeviceDescriptor> currentDevices = scanUSBDevices();
        
        // 检查新连接的设备
        for (const auto &device : currentDevices) {
            if (!knownDevices.contains(device.devicePath) && isScannerDevice(device)) {
                knownDevices[device.devicePath] = device;
                emit deviceConnected(device);
            }
        }
        
        // 检查断开的设备
        QStringList currentPaths;
        for (const auto &device : currentDevices) {
            currentPaths.append(device.devicePath);
        }
        
        QStringList knownPaths = knownDevices.keys();
        for (const QString &path : knownPaths) {
            if (!currentPaths.contains(path)) {
                knownDevices.remove(path);
                emit deviceDisconnected(path);
            }
        }
        
        // 等待1秒后再次扫描
        msleep(1000);
    }

    qCDebug(dscannerUSB) << "USB device monitor stopped";
}

QList<USBDeviceDescriptor> USBDeviceMonitor::scanUSBDevices()
{
    // 这里应该实际扫描USB设备
    // 由于我们在私有实现中，可以访问libusb
    QList<USBDeviceDescriptor> devices;
    
    // 完整的USB设备扫描实现
    // 高效的USB设备发现机制
    QList<USBDeviceDescriptor> devices;
    
#ifdef HAVE_LIBUSB
    if (parent && parent->d_ptr && parent->d_ptr->usbContext) {
        libusb_device **deviceList;
        ssize_t deviceCount = libusb_get_device_list(parent->d_ptr->usbContext, &deviceList);
        
        if (deviceCount >= 0) {
            for (ssize_t i = 0; i < deviceCount; i++) {
                libusb_device *device = deviceList[i];
                libusb_device_descriptor desc;
                
                if (libusb_get_device_descriptor(device, &desc) == 0) {
                    // 检查是否是扫描仪设备
                    if (isKnownScannerDevice(desc.idVendor, desc.idProduct)) {
                        USBDeviceDescriptor scannerDevice;
                        scannerDevice.vendorId = desc.idVendor;
                        scannerDevice.productId = desc.idProduct;
                        scannerDevice.deviceClass = desc.bDeviceClass;
                        scannerDevice.deviceSubClass = desc.bDeviceSubClass;
                        scannerDevice.deviceProtocol = desc.bDeviceProtocol;
                        scannerDevice.busNumber = libusb_get_bus_number(device);
                        scannerDevice.deviceAddress = libusb_get_device_address(device);
                        
                        // 构造设备路径
                        scannerDevice.devicePath = QString("/dev/bus/usb/%1/%2")
                            .arg(scannerDevice.busNumber, 3, 10, QChar('0'))
                            .arg(scannerDevice.deviceAddress, 3, 10, QChar('0'));
                        
                        // 获取字符串描述符
                        libusb_device_handle *handle;
                        if (libusb_open(device, &handle) == 0) {
                            unsigned char buffer[256];
                            
                            // 获取厂商名称
                            if (desc.iManufacturer > 0) {
                                if (libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, 
                                                                     buffer, sizeof(buffer)) >= 0) {
                                    scannerDevice.manufacturer = QString::fromLatin1(reinterpret_cast<char*>(buffer));
                                }
                            }
                            
                            // 获取产品名称
                            if (desc.iProduct > 0) {
                                if (libusb_get_string_descriptor_ascii(handle, desc.iProduct, 
                                                                     buffer, sizeof(buffer)) >= 0) {
                                    scannerDevice.product = QString::fromLatin1(reinterpret_cast<char*>(buffer));
                                }
                            }
                            
                            // 获取序列号
                            if (desc.iSerialNumber > 0) {
                                if (libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, 
                                                                     buffer, sizeof(buffer)) >= 0) {
                                    scannerDevice.serialNumber = QString::fromLatin1(reinterpret_cast<char*>(buffer));
                                }
                            }
                            
                            libusb_close(handle);
                        }
                        
                        // 设置设备特征信息
                        scannerDevice.speed = static_cast<USBSpeed>(libusb_get_device_speed(device));
                        scannerDevice.isConnected = true;
                        scannerDevice.isAvailable = true;
                        
                        devices.append(scannerDevice);
                        
                        qCDebug(dscannerUSB) << "Found scanner device:" 
                                             << QString::number(scannerDevice.vendorId, 16)
                                             << QString::number(scannerDevice.productId, 16)
                                             << scannerDevice.manufacturer << scannerDevice.product;
                    }
                }
            }
            
            libusb_free_device_list(deviceList, 1);
            qCInfo(dscannerUSB) << "USB scan completed, found" << devices.size() << "scanner devices";
        } else {
            qCWarning(dscannerUSB) << "Failed to get USB device list:" << deviceCount;
        }
    } else {
        qCWarning(dscannerUSB) << "USB context not available for device scanning";
    }
#else
    // 如果没有libusb，使用系统接口
    devices = scanUSBDevicesWithSysfs();
#endif
    
    return devices;
}

bool USBDeviceMonitor::isScannerDevice(const USBDeviceDescriptor &descriptor)
{
    return DScannerUSB::isScannerDevice(descriptor);
}

#include "dscannerusb.moc" 