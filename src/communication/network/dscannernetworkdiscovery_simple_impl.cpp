// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerNetworkDiscovery_Simple.h"

#include <QDebug>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QHostAddress>
#include <QMutexLocker>
#include <QDateTime>
#include <QLoggingCategory>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerNetwork, "deepinscan.network")

DScannerNetworkDiscoverySimple::DScannerNetworkDiscoverySimple(QObject *parent)
    : QObject(parent)
    , m_isDiscovering(false)
    , m_discoveryInterval(30)
    , m_discoveryTimer(new QTimer(this))
    , m_networkManager(new QNetworkAccessManager(this))
{
    // 默认启用的协议
    m_enabledProtocols << NetworkProtocol::AirScan 
                       << NetworkProtocol::WSD 
                       << NetworkProtocol::IPP;
    
    // 连接定时器
    connect(m_discoveryTimer, &QTimer::timeout, this, &DScannerNetworkDiscoverySimple::performDiscovery);
}

DScannerNetworkDiscoverySimple::~DScannerNetworkDiscoverySimple()
{
    stopDiscovery();
}

void DScannerNetworkDiscoverySimple::startDiscovery()
{
    if (m_isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Starting network discovery";
    
    m_isDiscovering = true;
    m_discoveryTimer->start(m_discoveryInterval * 1000);
    
    // 立即执行一次发现
    performDiscovery();
}

void DScannerNetworkDiscoverySimple::stopDiscovery()
{
    if (!m_isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Stopping network discovery";
    
    m_isDiscovering = false;
    m_discoveryTimer->stop();
    
    emit discoveryFinished();
}

bool DScannerNetworkDiscoverySimple::isDiscovering() const
{
    return m_isDiscovering;
}

QList<NetworkDeviceInfo> DScannerNetworkDiscoverySimple::discoveredDevices() const
{
    QMutexLocker locker(&m_deviceMutex);
    return m_devices;
}

NetworkDeviceInfo DScannerNetworkDiscoverySimple::deviceInfo(const QString &deviceId) const
{
    QMutexLocker locker(&m_deviceMutex);
    return m_deviceMap.value(deviceId);
}

void DScannerNetworkDiscoverySimple::refreshDevice(const QString &deviceId)
{
    Q_UNUSED(deviceId)
    // 简化实现，重新执行发现
    performDiscovery();
}

void DScannerNetworkDiscoverySimple::setDiscoveryInterval(int seconds)
{
    m_discoveryInterval = seconds;
    
    if (m_isDiscovering) {
        m_discoveryTimer->setInterval(seconds * 1000);
    }
}

int DScannerNetworkDiscoverySimple::discoveryInterval() const
{
    return m_discoveryInterval;
}

void DScannerNetworkDiscoverySimple::setEnabledProtocols(const QList<NetworkProtocol> &protocols)
{
    m_enabledProtocols = protocols;
}

QList<NetworkProtocol> DScannerNetworkDiscoverySimple::enabledProtocols() const
{
    return m_enabledProtocols;
}

bool DScannerNetworkDiscoverySimple::addDevice(const QHostAddress &address, quint16 port, NetworkProtocol protocol)
{
    NetworkDeviceInfo device;
    device.deviceId = QString("manual_%1_%2").arg(address.toString()).arg(port);
    device.name = QString("Manual Device (%1)").arg(address.toString());
    device.ipAddress = address;
    device.port = port;
    device.protocol = protocol;
    device.isOnline = true;
    device.lastSeen = QDateTime::currentDateTime();
    
    QMutexLocker locker(&m_deviceMutex);
    m_devices.append(device);
    m_deviceMap.insert(device.deviceId, device);
    locker.unlock();
    
    emit deviceDiscovered(device);
    return true;
}

void DScannerNetworkDiscoverySimple::removeDevice(const QString &deviceId)
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (m_deviceMap.remove(deviceId) > 0) {
        for (int i = 0; i < m_devices.size(); ++i) {
            if (m_devices[i].deviceId == deviceId) {
                m_devices.removeAt(i);
                break;
            }
        }
        
        locker.unlock();
        emit deviceOffline(deviceId);
    }
}

void DScannerNetworkDiscoverySimple::performDiscovery()
{
    qCDebug(dscannerNetwork) << "Performing network discovery";
    
    // 模拟设备发现
    static int deviceCount = 0;
    
    if (deviceCount < 2) {
        NetworkDeviceInfo device;
        device.deviceId = QString("network_device_%1").arg(deviceCount);
        device.name = QString("Network Scanner %1").arg(deviceCount + 1);
        device.manufacturer = "Generic";
        device.model = "Network Scanner";
        device.ipAddress = QHostAddress(QString("192.168.1.%1").arg(100 + deviceCount));
        device.port = 8080;
        device.protocol = NetworkProtocol::AirScan;
        device.isOnline = true;
        device.lastSeen = QDateTime::currentDateTime();
        device.serviceUrl = QUrl(QString("http://192.168.1.%1:8080/eSCL").arg(100 + deviceCount));
        
        QMutexLocker locker(&m_deviceMutex);
        m_devices.append(device);
        m_deviceMap.insert(device.deviceId, device);
        locker.unlock();
        
        emit deviceDiscovered(device);
        deviceCount++;
    }
}

// 为了兼容性，创建一个别名
typedef DScannerNetworkDiscoverySimple DScannerNetworkDiscovery;

 