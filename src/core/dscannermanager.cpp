#include "Scanner/DScannerManager.h"
#include "dscannermanager_p.h"
#include "Scanner/DScannerException.h"

#include <QDebug>
#include <QMutexLocker>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerCore, "deepinscan.core")

// DScannerManagerPrivate 实现

DScannerManagerPrivate::DScannerManagerPrivate(DScannerManager *q)
    : q_ptr(q)
    , discoveryTimer(new QTimer(q))
    , autoDiscovery(true)
    , discoveryInterval(30000)
    , settings(nullptr)
    , configPath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/deepinscan/config.ini"))
    , usbContext(nullptr)
    , usbInitialized(false)
    , saneLibrary(nullptr)
    , saneInitialized(false)
    , networkDiscovery(new DScannerNetworkDiscoverySimple(q))
    , m_networkCompleteDiscovery(nullptr)
{
    // 初始化统计信息
    stats.totalDevicesFound = 0;
    stats.activeDevices = 0;
    stats.failedConnections = 0;
    stats.discoveryTime = 0;
}

DScannerManagerPrivate::~DScannerManagerPrivate()
{
    cleanup();
    delete settings;
}

void DScannerManagerPrivate::init()
{
    qCDebug(dscannerCore) << "Initializing DScannerManager";
    
    // 加载配置
    loadConfiguration();
    
    // 初始化USB子系统
    if (!initUSB()) {
        qCWarning(dscannerCore) << "Failed to initialize USB subsystem";
    }
    
    // 初始化SANE子系统
    if (!initSANE()) {
        qCWarning(dscannerCore) << "Failed to initialize SANE subsystem";
    }
    
    // 初始化网络发现
    if (networkDiscovery) {
        QObject::connect(networkDiscovery, &DScannerNetworkDiscoverySimple::deviceDiscovered,
                        q_ptr, [this](const NetworkDeviceInfo &netDevice) {
            qCDebug(dscannerCore) << "Network device discovered:" << netDevice.name;
            // 转换为DeviceInfo并添加到设备列表
            DeviceInfo device;
            device.deviceId = netDevice.deviceId;
            device.name = netDevice.name;
            device.manufacturer = netDevice.manufacturer;
            device.model = netDevice.model;
            
            // 创建设备对象
            DScannerDevice *scannerDevice = new DScannerDevice(device, q_ptr);
            addDevice(scannerDevice);
        });
        
        // 启动网络发现
        networkDiscovery->startDiscovery();
    }
    
    // 加载设备数据库
    loadDeviceDatabase();
    
    // 加载驱动
    loadDrivers();
    
    // 设置自动发现定时器
    QObject::connect(discoveryTimer, &QTimer::timeout, [this]() {
        discoverDevices();
    });
    
    if (autoDiscovery) {
        discoveryTimer->start(discoveryInterval);
    }
    
    qCDebug(dscannerCore) << "DScannerManager initialized successfully";
}

void DScannerManagerPrivate::cleanup()
{
    qCDebug(dscannerCore) << "Cleaning up DScannerManager";
    
    // 停止自动发现
    discoveryTimer->stop();
    
    // 清理设备
    qDeleteAll(devices);
    devices.clear();
    deviceMap.clear();
    
    // 卸载驱动
    unloadDrivers();
    
    // 清理USB子系统
    cleanupUSB();
    
    // 清理SANE子系统
    cleanupSANE();
    
    // 保存配置
    saveConfiguration();
    
    qCDebug(dscannerCore) << "DScannerManager cleanup completed";
}

void DScannerManagerPrivate::discoverDevices()
{
    QMutexLocker locker(&mutex);
    
    qCDebug(dscannerCore) << "Starting device discovery";
    
    QElapsedTimer timer;
    timer.start();
    
    // 发现USB设备
    discoverUSBDevices();
    
    // 发现SANE设备
    discoverSANEDevices();
    
    // 发现网络设备
    discoverNetworkDevices();
    
    stats.lastDiscoveryTime = QDateTime::currentDateTime();
    stats.discoveryTime = timer.elapsed();
    
    qCDebug(dscannerCore) << "Device discovery completed in" << stats.discoveryTime << "ms";
    
    emit q_ptr->deviceListRefreshed();
}

void DScannerManagerPrivate::discoverUSBDevices()
{
    if (!usbInitialized) {
        return;
    }
    
    qCDebug(dscannerCore) << "Discovering USB devices";
    
    QList<USBDeviceInfo> usbDevices = getUSBDevices();
    
    for (const USBDeviceInfo &usbInfo : usbDevices) {
        if (isKnownUSBDevice(usbInfo.vendorId, usbInfo.productId)) {
            DeviceInfo info;
            info.deviceId = QStringLiteral("usb:%1:%2").arg(usbInfo.vendorId, 4, 16, QLatin1Char('0')).arg(usbInfo.productId, 4, 16, QLatin1Char('0'));
            info.name = QStringLiteral("%1 %2").arg(usbInfo.manufacturer, usbInfo.product);
            info.manufacturer = usbInfo.manufacturer;
            info.model = usbInfo.product;
            info.protocol = CommunicationProtocol::USB;
            info.isAvailable = true;
            
            // 检查设备是否已存在
            if (!deviceMap.contains(info.deviceId)) {
                DScannerDevice *device = new DScannerDevice(info, q_ptr);
                addDevice(device);
                
                emit q_ptr->deviceDiscovered(info);
                stats.totalDevicesFound++;
            }
        }
    }
}

void DScannerManagerPrivate::discoverSANEDevices()
{
    if (!saneInitialized) {
        return;
    }
    
    qCDebug(dscannerCore) << "Discovering SANE devices";
    
    QList<DeviceInfo> saneDevices = getSANEDevices();
    
    for (const DeviceInfo &info : saneDevices) {
        if (!deviceMap.contains(info.deviceId)) {
            DScannerDevice *device = new DScannerDevice(info, q_ptr);
            addDevice(device);
            
            emit q_ptr->deviceDiscovered(info);
            stats.totalDevicesFound++;
        }
    }
}

void DScannerManagerPrivate::discoverNetworkDevices()
{
    qCDebug(dscannerCore) << "Discovering network devices";
    
    QList<DeviceInfo> networkDevices = getNetworkDevices();
    
    for (const DeviceInfo &info : networkDevices) {
        if (!deviceMap.contains(info.deviceId)) {
            DScannerDevice *device = new DScannerDevice(info, q_ptr);
            addDevice(device);
            
            emit q_ptr->deviceDiscovered(info);
            stats.totalDevicesFound++;
        }
    }
}

void DScannerManagerPrivate::addDevice(DScannerDevice *device)
{
    if (!device) {
        return;
    }
    
    const QString deviceId = device->deviceInfo().deviceId;
    
    devices.append(device);
    deviceMap.insert(deviceId, device);
    
    // 连接设备信号
          QObject::connect(device, &DScannerDevice::statusChanged, q_ptr, [this, deviceId](DScannerDevice::Status status) {
         // Device status change - emit appropriate signal based on status
         if (status == DScannerDevice::Status::Ready) {
             emit q_ptr->deviceOpened(deviceId);
         } else if (status == DScannerDevice::Status::Offline) {
             emit q_ptr->deviceClosed(deviceId);
         }
      });
    
    QObject::connect(device, static_cast<void(DScannerDevice::*)(int, const QString&)>(&DScannerDevice::errorOccurred), 
                     q_ptr, [this, deviceId](int code, const QString &message) {
        emit q_ptr->errorOccurred(QStringLiteral("Device %1 error: %2").arg(deviceId, message));
    });
    
    qCDebug(dscannerCore) << "Device added:" << deviceId;
    
    emit q_ptr->deviceListRefreshed();
}

void DScannerManagerPrivate::removeDevice(const QString &deviceId)
{
    DScannerDevice *device = deviceMap.value(deviceId);
    if (!device) {
        qCWarning(dscannerCore) << "Device not found:" << deviceId;
        return;
    }
    
    devices.removeAll(device);
    deviceMap.remove(deviceId);
    
    // 断开设备信号
    QObject::disconnect(device, nullptr, q_ptr, nullptr);
    
    delete device;
    
    qCDebug(dscannerCore) << "Device removed:" << deviceId;
    
    emit q_ptr->deviceRemoved(deviceId);
    emit q_ptr->deviceListRefreshed();
}

DScannerDevice *DScannerManagerPrivate::findDevice(const QString &deviceId) const
{
    return deviceMap.value(deviceId);
}

bool DScannerManagerPrivate::initUSB()
{
    qCDebug(dscannerCore) << "Initializing USB subsystem";
    
    int result = libusb_init(&usbContext);
    if (result != LIBUSB_SUCCESS) {
        qCWarning(dscannerCore) << "Failed to initialize libusb:" << libusb_strerror(static_cast<libusb_error>(result));
        return false;
    }
    
    usbInitialized = true;
    qCDebug(dscannerCore) << "USB subsystem initialized successfully";
    
    return true;
}

void DScannerManagerPrivate::cleanupUSB()
{
    if (usbInitialized) {
        qCDebug(dscannerCore) << "Cleaning up USB subsystem";
        libusb_exit(usbContext);
        usbContext = nullptr;
        usbInitialized = false;
    }
}

QList<USBDeviceInfo> DScannerManagerPrivate::getUSBDevices()
{
    QList<USBDeviceInfo> devices;
    
    if (!usbInitialized) {
        return devices;
    }
    
    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(usbContext, &deviceList);
    
    if (deviceCount < 0) {
        qCWarning(dscannerCore) << "Failed to get USB device list";
        return devices;
    }
    
    for (ssize_t i = 0; i < deviceCount; ++i) {
        libusb_device *device = deviceList[i];
        libusb_device_descriptor desc;
        
        if (libusb_get_device_descriptor(device, &desc) == LIBUSB_SUCCESS) {
            USBDeviceInfo usbInfo;
            usbInfo.vendorId = desc.idVendor;
            usbInfo.productId = desc.idProduct;
            usbInfo.deviceClass = desc.bDeviceClass;
            usbInfo.deviceSubClass = desc.bDeviceSubClass;
            usbInfo.deviceProtocol = desc.bDeviceProtocol;
            usbInfo.busNumber = libusb_get_bus_number(device);
            usbInfo.deviceAddress = libusb_get_device_address(device);
            
            // 获取字符串描述符
            libusb_device_handle *handle;
            if (libusb_open(device, &handle) == LIBUSB_SUCCESS) {
                unsigned char buffer[256];
                if (desc.iManufacturer && libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buffer, sizeof(buffer)) > 0) {
                    usbInfo.manufacturer = QString::fromLatin1(reinterpret_cast<char*>(buffer));
                }
                if (desc.iProduct && libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, sizeof(buffer)) > 0) {
                    usbInfo.product = QString::fromLatin1(reinterpret_cast<char*>(buffer));
                }
                libusb_close(handle);
            }
            
            devices.append(usbInfo);
        }
    }
    
    libusb_free_device_list(deviceList, 1);
    
    qCDebug(dscannerCore) << "Found" << devices.size() << "USB devices";
    
    return devices;
}

bool DScannerManagerPrivate::isKnownUSBDevice(quint16 vendorId, quint16 productId)
{
    // 检查已知设备列表
    for (const KnownDevice &known : knownDevices) {
        if (known.vendorId == vendorId && known.productId == productId) {
            return true;
        }
    }
    
    // 查询设备数据库
    QList<DeviceInfo> dbDevices = queryDeviceDatabase(vendorId, productId);
    return !dbDevices.isEmpty();
}

bool DScannerManagerPrivate::initSANE()
{
    qCDebug(dscannerCore) << "Initializing SANE subsystem";
    
    // 尝试加载SANE库
    saneLibrary = new QLibrary(QStringLiteral("sane"), q_ptr);
    if (!saneLibrary->load()) {
        qCWarning(dscannerCore) << "Failed to load SANE library:" << saneLibrary->errorString();
        delete saneLibrary;
        saneLibrary = nullptr;
        return false;
    }
    
    // 完整的SANE API初始化实现
    SANEAPIManager *manager = SANEAPIManager::instance();
    if (manager) {
        int versionCode = 0;
        int status = manager->sane_init_impl(&versionCode, nullptr);
        if (status == 0) { // SANE_STATUS_GOOD
            saneInitialized = true;
            qCInfo(dscannerCore) << "SANE API initialized with version:" 
                                 << QString("0x%1").arg(versionCode, 0, 16);
        } else {
            qCWarning(dscannerCore) << "Failed to initialize SANE API, status:" << status;
            return false;
        }
    } else {
        qCWarning(dscannerCore) << "Failed to get SANE API manager instance";
        return false;
    }
    
    saneInitialized = true;
    qCDebug(dscannerCore) << "SANE subsystem initialized successfully";
    
    return true;
}

void DScannerManagerPrivate::cleanupSANE()
{
    if (saneInitialized) {
        qCDebug(dscannerCore) << "Cleaning up SANE subsystem";
        
        // 完整的SANE API清理实现
        SANEAPIManager *manager = SANEAPIManager::instance();
        if (manager && manager->isInitialized()) {
            manager->sane_exit_impl();
            qCDebug(dscannerCore) << "SANE API shutdown completed";
        }
        
        if (saneLibrary) {
            saneLibrary->unload();
            delete saneLibrary;
            saneLibrary = nullptr;
        }
        
        saneInitialized = false;
    }
}

QList<DeviceInfo> DScannerManagerPrivate::getSANEDevices()
{
    QList<DeviceInfo> devices;
    
    if (!saneInitialized) {
        return devices;
    }
    
    // 完整的SANE设备发现实现
    SANEAPIManager *manager = SANEAPIManager::instance();
    if (manager && manager->isInitialized()) {
        const void **deviceList = nullptr;
        int status = manager->sane_get_devices_impl(&deviceList, 1); // local_only = true
        
        if (status == 0 && deviceList) { // SANE_STATUS_GOOD
            // 转换SANE设备列表为我们的DeviceInfo格式
            int deviceIndex = 0;
            while (deviceList[deviceIndex] != nullptr) {
                const SANEDeviceInfo *saneDevice = static_cast<const SANEDeviceInfo*>(deviceList[deviceIndex]);
                
                DeviceInfo deviceInfo;
                deviceInfo.deviceId = saneDevice->name;
                deviceInfo.name = saneDevice->model;
                deviceInfo.vendor = saneDevice->vendor;
                deviceInfo.model = saneDevice->model;
                deviceInfo.deviceType = convertToDeviceType(saneDevice->type);
                deviceInfo.connectionType = ConnectionType::USB; // 大多数SANE设备是USB
                deviceInfo.driverType = saneDevice->driverType;
                deviceInfo.devicePath = saneDevice->devicePath;
                deviceInfo.isAvailable = true;
                deviceInfo.isConnected = true;
                
                devices.append(deviceInfo);
                deviceIndex++;
            }
            
            qCInfo(dscannerCore) << "Discovered" << devices.size() << "SANE devices";
        } else {
            qCWarning(dscannerCore) << "Failed to get SANE devices, status:" << status;
        }
    }
    
    return devices;
}

QList<DeviceInfo> DScannerManagerPrivate::getNetworkDevices()
{
    QList<DeviceInfo> devices;
    
    // 实现网络设备发现 - 使用完整网络发现引擎
    qCDebug(dscannerManager) << "开始网络设备发现";
    
    if (!m_networkCompleteDiscovery) {
        m_networkCompleteDiscovery = new NetworkCompleteDiscovery(this);
        
        // 连接网络发现信号
        connect(m_networkCompleteDiscovery, &NetworkCompleteDiscovery::deviceDiscovered,
                this, [this](const NetworkScannerDevice &networkDevice) {
            qCDebug(dscannerManager) << "发现网络设备:" << networkDevice.makeAndModel
                                      << "协议:" << networkDevice.protocol;
            
            // 将网络设备转换为标准设备信息
            DScannerDeviceInfo deviceInfo;
            deviceInfo.id = networkDevice.uuid;
            deviceInfo.name = networkDevice.makeAndModel;
            deviceInfo.manufacturer = networkDevice.makeAndModel.split(" ").first();
            deviceInfo.model = networkDevice.makeAndModel;
            deviceInfo.type = DScannerDeviceInfo::NetworkScanner;
            deviceInfo.connectionType = "Network";
            deviceInfo.isAvailable = true;
            
            // 设置网络特定属性
            deviceInfo.properties["protocol"] = networkDevice.protocol;
            deviceInfo.properties["discoveryMethod"] = networkDevice.discoveryMethod;
            deviceInfo.properties["baseUrl"] = networkDevice.baseUrl.toString();
            deviceInfo.properties["addresses"] = networkDevice.addresses;
            deviceInfo.properties["capabilities"] = networkDevice.capabilities;
            
            // 添加到设备列表
            QMutexLocker locker(&m_deviceMutex);
            bool deviceExists = false;
            for (const auto &existingDevice : m_availableDevices) {
                if (existingDevice.id == deviceInfo.id) {
                    deviceExists = true;
                    break;
                }
            }
            
            if (!deviceExists) {
                m_availableDevices.append(deviceInfo);
                emit deviceListChanged();
                emit deviceFound(deviceInfo);
            }
        });
        
        connect(m_networkCompleteDiscovery, &NetworkCompleteDiscovery::discoveryCompleted,
                this, [this](const QList<NetworkScannerDevice> &devices) {
            qCDebug(dscannerManager) << "网络发现完成，发现设备数量:" << devices.size();
            
            // 获取并记录发现统计信息
            auto stats = m_networkCompleteDiscovery->getStatistics();
            qCDebug(dscannerManager) << "网络发现统计:"
                                      << "总设备:" << stats.totalDevicesFound
                                      << "mDNS:" << stats.mdnsDevicesFound
                                      << "WSD:" << stats.wsdDevicesFound
                                      << "SOAP:" << stats.soapDevicesFound
                                      << "SNMP:" << stats.snmpDevicesFound
                                      << "UPnP:" << stats.upnpDevicesFound
                                      << "端口扫描:" << stats.portScanDevicesFound;
        });
    }
    
    // 启动网络发现
    if (!m_networkCompleteDiscovery->isDiscovering()) {
        bool started = m_networkCompleteDiscovery->startDiscovery();
        if (started) {
            qCDebug(dscannerManager) << "网络发现引擎启动成功";
        } else {
            qCWarning(dscannerManager) << "网络发现引擎启动失败";
        }
    } else {
        qCDebug(dscannerManager) << "网络发现引擎已在运行中";
    }
    
    return devices;
}

void DScannerManagerPrivate::loadConfiguration()
{
    if (!settings) {
        settings = new QSettings(configPath, QSettings::IniFormat);
    }
    
    qCDebug(dscannerCore) << "Loading configuration from:" << configPath;
    
    // 加载发现设置
    settings->beginGroup(QStringLiteral("Discovery"));
    autoDiscovery = settings->value(QStringLiteral("autoDiscovery"), true).toBool();
    discoveryInterval = settings->value(QStringLiteral("discoveryInterval"), 30000).toInt();
    settings->endGroup();
    
    // 加载统计信息
    settings->beginGroup(QStringLiteral("Statistics"));
    stats.totalDevicesFound = settings->value(QStringLiteral("totalDevicesFound"), 0).toInt();
    settings->endGroup();
}

void DScannerManagerPrivate::saveConfiguration()
{
    if (!settings) {
        return;
    }
    
    qCDebug(dscannerCore) << "Saving configuration to:" << configPath;
    
    // 保存发现设置
    settings->beginGroup(QStringLiteral("Discovery"));
    settings->setValue(QStringLiteral("autoDiscovery"), autoDiscovery);
    settings->setValue(QStringLiteral("discoveryInterval"), discoveryInterval);
    settings->endGroup();
    
    // 保存统计信息
    settings->beginGroup(QStringLiteral("Statistics"));
    settings->setValue(QStringLiteral("totalDevicesFound"), stats.totalDevicesFound);
    settings->endGroup();
    
    settings->sync();
}

void DScannerManagerPrivate::loadDeviceDatabase()
{
    qCDebug(dscannerCore) << "Loading device database";
    
    // 加载设备数据库
    QString dbPath = QStringLiteral(":/data/device_database.json");
    QFile dbFile(dbPath);
    
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qCWarning(dscannerCore) << "Failed to open device database:" << dbPath;
        return;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(dbFile.readAll(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dscannerCore) << "Failed to parse device database:" << error.errorString();
        return;
    }
    
    deviceDatabase = doc.object();
    
    // 加载已知设备列表
    QJsonArray deviceArray = deviceDatabase[QStringLiteral("devices")].toArray();
    
    for (const QJsonValue &value : deviceArray) {
        QJsonObject obj = value.toObject();
        KnownDevice known;
        known.vendorId = static_cast<quint16>(obj[QStringLiteral("vendor_id")].toInt());
        known.productId = static_cast<quint16>(obj[QStringLiteral("product_id")].toInt());
        known.manufacturer = obj[QStringLiteral("manufacturer")].toString();
        known.model = obj[QStringLiteral("model")].toString();
        known.driverType = static_cast<DriverType>(obj[QStringLiteral("driver_type")].toInt());
        known.protocol = static_cast<CommunicationProtocol>(obj[QStringLiteral("protocol")].toInt());
        
        knownDevices.append(known);
    }
    
    qCInfo(dscannerCore) << "Device database loaded successfully -" << knownDevices.size() << "known devices";
}

QList<DeviceInfo> DScannerManagerPrivate::queryDeviceDatabase(quint16 vendorId, quint16 productId)
{
    QList<DeviceInfo> devices;
    
    QJsonArray deviceArray = deviceDatabase[QStringLiteral("devices")].toArray();
    
    for (const QJsonValue &value : deviceArray) {
        QJsonObject obj = value.toObject();
        if (obj[QStringLiteral("vendor_id")].toInt() == vendorId && obj[QStringLiteral("product_id")].toInt() == productId) {
            DeviceInfo info;
            info.name = obj[QStringLiteral("name")].toString();
            info.manufacturer = obj[QStringLiteral("manufacturer")].toString();
            info.model = obj[QStringLiteral("model")].toString();
            info.protocol = static_cast<CommunicationProtocol>(obj[QStringLiteral("protocol")].toInt());
            info.isAvailable = true;
            
            devices.append(info);
        }
    }
    
    return devices;
}

void DScannerManagerPrivate::loadDrivers()
{
    qCDebug(dscannerCore) << "Loading scanner drivers";
    
    // 查找驱动目录
    QString driverPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("drivers"), QStandardPaths::LocateDirectory);
    
    if (driverPath.isEmpty()) {
        qCWarning(dscannerCore) << "Driver directory not found";
        return;
    }
    
    QDir driverDir(driverPath);
    QStringList driverFiles = driverDir.entryList(QStringList() << QStringLiteral("*.so") << QStringLiteral("*.dll"), QDir::Files);
    
    for (const QString &fileName : driverFiles) {
        QString fullPath = driverDir.absoluteFilePath(fileName);
        loadDriver(fullPath);
    }
    
    qCInfo(dscannerCore) << "Loaded" << drivers.size() << "scanner drivers";
}

void DScannerManagerPrivate::unloadDrivers()
{
    qCDebug(dscannerCore) << "Unloading scanner drivers";
    
    qDeleteAll(drivers);
    drivers.clear();
    driverMap.clear();
}

bool DScannerManagerPrivate::loadDriver(const QString &driverPath)
{
    qCDebug(dscannerCore) << "Loading driver:" << driverPath;
    
    QPluginLoader loader(driverPath);
    QObject *plugin = loader.instance();
    
    if (!plugin) {
        qCWarning(dscannerCore) << "Failed to load driver plugin:" << loader.errorString();
        return false;
    }
    
    DScannerDriver *driver = qobject_cast<DScannerDriver*>(plugin);
    if (!driver) {
        qCWarning(dscannerCore) << "Plugin is not a valid scanner driver:" << driverPath;
        return false;
    }
    
    drivers.append(driver);
    DriverType type = driver->driverType();
    driverMap[type].append(driver);
    
    qCDebug(dscannerCore) << "Driver loaded successfully:" << driver->driverName();
    
    return true;
}

void DScannerManagerPrivate::logDeviceInfo(const DeviceInfo &info) const
{
    qCInfo(dscannerCore) << "Device Info:";
    qCInfo(dscannerCore) << "  ID:" << info.deviceId;
    qCInfo(dscannerCore) << "  Name:" << info.name;
    qCInfo(dscannerCore) << "  Manufacturer:" << info.manufacturer;
    qCInfo(dscannerCore) << "  Model:" << info.model;
    qCInfo(dscannerCore) << "  Protocol:" << static_cast<int>(info.protocol);
}

void DScannerManagerPrivate::logManagerStats() const
{
    qCInfo(dscannerCore) << "Manager Statistics:";
    qCInfo(dscannerCore) << "  Total Devices Found:" << stats.totalDevicesFound;
    qCInfo(dscannerCore) << "  Active Devices:" << stats.activeDevices;
    qCInfo(dscannerCore) << "  Failed Connections:" << stats.failedConnections;
    qCInfo(dscannerCore) << "  Last Discovery:" << stats.lastDiscoveryTime.toString();
    qCInfo(dscannerCore) << "  Discovery Time:" << stats.discoveryTime << "ms";
}

// DScannerManager 公共接口实现

DScannerManager::DScannerManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerManagerPrivate(this))
{
    qCDebug(dscannerCore) << "DScannerManager created";
}

DScannerManager::~DScannerManager()
{
    qCDebug(dscannerCore) << "DScannerManager destroyed";
}

DScannerManager *DScannerManager::instance()
{
    static DScannerManager *manager = nullptr;
    static QMutex mutex;
    
    QMutexLocker locker(&mutex);
    if (!manager) {
        manager = new DScannerManager();
        manager->d_ptr->init();
    }
    
    return manager;
}

DScannerDevice *DScannerManager::device(const QString &deviceId) const
{
    Q_D(const DScannerManager);
    QMutexLocker locker(&d->mutex);
    return d->findDevice(deviceId);
}


QString DScannerManager::version()
{
    return QStringLiteral("1.0.0");
}
