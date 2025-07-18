// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERMANAGER_P_H
#define DSCANNERMANAGER_P_H

#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerDevice.h"
#include "Scanner/DScannerDriver.h"
#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerNetworkDiscovery_Simple.h"
#include "../communication/network/network_complete_discovery.h"

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLibrary>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QLoggingCategory>
#include <QDateTime>
#include <QElapsedTimer>
#include <QScopedPointer>
#include <memory>

#include <libusb-1.0/libusb.h>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dscannerCore)

/**
 * \class DScannerManagerPrivate
 * \brief DScannerManager的私有实现类
 * 
 * 负责设备发现、管理和驱动加载等核心功能
 */
class DScannerManagerPrivate
{
    Q_DECLARE_PUBLIC(DScannerManager)
    
public:
    explicit DScannerManagerPrivate(DScannerManager *q);
    ~DScannerManagerPrivate();
    
    // 初始化和清理
    void init();
    void cleanup();
    
    // 设备发现
    void discoverDevices();
    void discoverUSBDevices();
    void discoverSANEDevices();
    void discoverNetworkDevices();
    
    // 设备管理
    void addDevice(DScannerDevice *device);
    void removeDevice(const QString &deviceId);
    DScannerDevice *findDevice(const QString &deviceId) const;
    QList<DeviceInfo> availableDevices() const;
    DScannerDevice *openDevice(const QString &deviceId);
    void closeDevice(const QString &deviceId);
    bool isDeviceAvailable(const QString &deviceId) const;
    DeviceInfo deviceInfo(const QString &deviceId) const;
    
    // 驱动管理
    void loadDrivers();
    void unloadDrivers();
    bool loadDriver(const QString &driverPath);
    DScannerDriver *createDriver(DriverType type, const DeviceInfo &info);
    
    // USB 相关
    bool initUSB();
    void cleanupUSB();
    QList<USBDeviceInfo> getUSBDevices();
    bool isKnownUSBDevice(quint16 vendorId, quint16 productId);
    
    // SANE 相关
    bool initSANE();
    void cleanupSANE();
    QList<DeviceInfo> getSANEDevices();
    
    // 网络设备
    QList<DeviceInfo> getNetworkDevices();
    
    // 配置管理
    void loadConfiguration();
    void saveConfiguration();
    
    // 设备数据库
    void loadDeviceDatabase();
    QList<DeviceInfo> queryDeviceDatabase(quint16 vendorId, quint16 productId);
    
    // 公共成员
    DScannerManager *q_ptr;
    
    // 设备管理
    QList<DScannerDevice*> devices;
    QHash<QString, DScannerDevice*> deviceMap;
    
    // 驱动管理
    QList<DScannerDriver*> drivers;
    QMap<DriverType, QList<DScannerDriver*>> driverMap;
    
    // 发现和配置
    QTimer *discoveryTimer;
    bool autoDiscovery;
    int discoveryInterval;
    
    // 线程同步
    mutable QMutex mutex;
    
    // 配置管理
    QSettings *settings;
    QString configPath;
    
    // USB 支持
    libusb_context *usbContext;
    bool usbInitialized;
    
    // SANE 支持
    QLibrary *saneLibrary;
    bool saneInitialized;
    
    // 设备数据库
    QJsonObject deviceDatabase;
    
    // 已知设备列表
    struct KnownDevice {
        quint16 vendorId;
        quint16 productId;
        QString manufacturer;
        QString model;
        DriverType driverType;
        CommunicationProtocol protocol;
    };
    QList<KnownDevice> knownDevices;
    
    // 性能统计
    struct ManagerStats {
        int totalDevicesFound;
        int activeDevices;
        int failedConnections;
        QDateTime lastDiscoveryTime;
        qint64 discoveryTime;
    } stats;
    
    // 网络发现
    DScannerNetworkDiscoverySimple *networkDiscovery;
    NetworkCompleteDiscovery *m_networkCompleteDiscovery;
    
    // 辅助方法
    void logDeviceInfo(const DeviceInfo &info) const;
    void logManagerStats() const;
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERMANAGER_P_H 