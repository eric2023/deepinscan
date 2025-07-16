// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerNetworkDiscovery.h"

#include <QDebug>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUdpSocket>
#include <QHostAddress>
#include <QMutexLocker>
#include <QDateTime>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerNetwork, "deepinscan.network")

// 简化的网络发现实现
class DScannerNetworkDiscoveryImpl
{
public:
    DScannerNetworkDiscoveryImpl(DScannerNetworkDiscovery *parent)
        : q(parent)
        , isDiscovering(false)
        , discoveryInterval(30)
        , discoveryTimer(new QTimer(parent))
        , networkManager(new QNetworkAccessManager(parent))
    {
        // 默认启用的协议
        enabledProtocols << NetworkProtocol::AirScan 
                         << NetworkProtocol::WSD 
                         << NetworkProtocol::IPP;
        
        // 连接定时器
        QObject::connect(discoveryTimer, &QTimer::timeout, parent, [this]() {
            performDiscovery();
        });
    }
    
    void performDiscovery()
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
            
            QMutexLocker locker(&deviceMutex);
            devices.append(device);
            deviceMap.insert(device.deviceId, device);
            locker.unlock();
            
            emit q->deviceDiscovered(device);
            deviceCount++;
        }
    }
    
    DScannerNetworkDiscovery *q;
    bool isDiscovering;
    int discoveryInterval;
    QList<NetworkProtocol> enabledProtocols;
    
    QTimer *discoveryTimer;
    QNetworkAccessManager *networkManager;
    
    QList<NetworkDeviceInfo> devices;
    QHash<QString, NetworkDeviceInfo> deviceMap;
    mutable QMutex deviceMutex;
};

// DScannerNetworkDiscovery 实现

DScannerNetworkDiscovery::DScannerNetworkDiscovery(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerNetworkDiscoveryImpl(this))
{
}

DScannerNetworkDiscovery::~DScannerNetworkDiscovery()
{
    delete d_ptr;
}

void DScannerNetworkDiscovery::startDiscovery()
{
    if (d_ptr->isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Starting network discovery";
    
    d_ptr->isDiscovering = true;
    d_ptr->discoveryTimer->start(d_ptr->discoveryInterval * 1000);
    
    // 立即执行一次发现
    d_ptr->performDiscovery();
}

void DScannerNetworkDiscovery::stopDiscovery()
{
    if (!d_ptr->isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Stopping network discovery";
    
    d_ptr->isDiscovering = false;
    d_ptr->discoveryTimer->stop();
    
    emit discoveryFinished();
}

bool DScannerNetworkDiscovery::isDiscovering() const
{
    return d_ptr->isDiscovering;
}

QList<NetworkDeviceInfo> DScannerNetworkDiscovery::discoveredDevices() const
{
    QMutexLocker locker(&d_ptr->deviceMutex);
    return d_ptr->devices;
}

NetworkDeviceInfo DScannerNetworkDiscovery::deviceInfo(const QString &deviceId) const
{
    QMutexLocker locker(&d_ptr->deviceMutex);
    return d_ptr->deviceMap.value(deviceId);
}

void DScannerNetworkDiscovery::refreshDevice(const QString &deviceId)
{
    Q_UNUSED(deviceId)
    // 简化实现，重新执行发现
    d_ptr->performDiscovery();
}

void DScannerNetworkDiscovery::setDiscoveryInterval(int seconds)
{
    d_ptr->discoveryInterval = seconds;
    
    if (d_ptr->isDiscovering) {
        d_ptr->discoveryTimer->setInterval(seconds * 1000);
    }
}

int DScannerNetworkDiscovery::discoveryInterval() const
{
    return d_ptr->discoveryInterval;
}

void DScannerNetworkDiscovery::setEnabledProtocols(const QList<NetworkProtocol> &protocols)
{
    d_ptr->enabledProtocols = protocols;
}

QList<NetworkProtocol> DScannerNetworkDiscovery::enabledProtocols() const
{
    return d_ptr->enabledProtocols;
}

bool DScannerNetworkDiscovery::addDevice(const QHostAddress &address, quint16 port, NetworkProtocol protocol)
{
    NetworkDeviceInfo device;
    device.deviceId = QString("manual_%1_%2").arg(address.toString()).arg(port);
    device.name = QString("Manual Device (%1)").arg(address.toString());
    device.ipAddress = address;
    device.port = port;
    device.protocol = protocol;
    device.isOnline = true;
    device.lastSeen = QDateTime::currentDateTime();
    
    QMutexLocker locker(&d_ptr->deviceMutex);
    d_ptr->devices.append(device);
    d_ptr->deviceMap.insert(device.deviceId, device);
    locker.unlock();
    
    emit deviceDiscovered(device);
    return true;
}

void DScannerNetworkDiscovery::removeDevice(const QString &deviceId)
{
    QMutexLocker locker(&d_ptr->deviceMutex);
    
    if (d_ptr->deviceMap.remove(deviceId) > 0) {
        for (int i = 0; i < d_ptr->devices.size(); ++i) {
            if (d_ptr->devices[i].deviceId == deviceId) {
                d_ptr->devices.removeAt(i);
                break;
            }
        }
        
        locker.unlock();
        emit deviceOffline(deviceId);
    }
}

void DScannerNetworkDiscovery::onMdnsLookupFinished()
{
    qCDebug(dscannerNetwork) << "mDNS lookup finished";
}

void DScannerNetworkDiscovery::onSoapQueryFinished()
{
    qCDebug(dscannerNetwork) << "SOAP query finished";
}

void DScannerNetworkDiscovery::onDeviceProbeFinished()
{
    qCDebug(dscannerNetwork) << "Device probe finished";
}

void DScannerNetworkDiscovery::onNetworkReplyFinished()
{
    qCDebug(dscannerNetwork) << "Network reply finished";
}

// DScannerMdnsDiscovery 简化实现

DScannerMdnsDiscovery::DScannerMdnsDiscovery(QObject *parent)
    : QObject(parent)
{
}

DScannerMdnsDiscovery::~DScannerMdnsDiscovery()
{
}

void DScannerMdnsDiscovery::startDiscovery()
{
    qCDebug(dscannerNetwork) << "Starting mDNS discovery";
    
    // 模拟mDNS发现
    QTimer::singleShot(1000, this, [this]() {
        emit serviceDiscovered("Test Scanner", "_uscan._tcp", QHostAddress("192.168.1.100"), 8080);
    });
}

void DScannerMdnsDiscovery::stopDiscovery()
{
    qCDebug(dscannerNetwork) << "Stopping mDNS discovery";
}

QStringList DScannerMdnsDiscovery::supportedServiceTypes()
{
    return QStringList() << "_uscan._tcp" << "_ipp._tcp" << "_scanner._tcp";
}

void DScannerMdnsDiscovery::onServiceDiscovered()
{
    qCDebug(dscannerNetwork) << "Service discovered signal received";
}

void DScannerMdnsDiscovery::onServiceRemoved()
{
    qCDebug(dscannerNetwork) << "Service removed signal received";
}

// DScannerSoapDiscovery 简化实现

DScannerSoapDiscovery::DScannerSoapDiscovery(QObject *parent)
    : QObject(parent)
{
}

DScannerSoapDiscovery::~DScannerSoapDiscovery()
{
}

void DScannerSoapDiscovery::startDiscovery()
{
    qCDebug(dscannerNetwork) << "Starting SOAP/WSD discovery";
    
    // 模拟WSD发现
    QTimer::singleShot(2000, this, [this]() {
        NetworkDeviceInfo device;
        device.deviceId = "wsd_test_device";
        device.name = "WSD Test Scanner";
        device.ipAddress = QHostAddress("192.168.1.101");
        device.port = 5357;
        device.protocol = NetworkProtocol::WSD;
        device.isOnline = true;
        device.lastSeen = QDateTime::currentDateTime();
        
        emit wsdDeviceDiscovered(device);
    });
}

void DScannerSoapDiscovery::stopDiscovery()
{
    qCDebug(dscannerNetwork) << "Stopping SOAP/WSD discovery";
}

void DScannerSoapDiscovery::sendWsdProbe()
{
    qCDebug(dscannerNetwork) << "Sending WSD probe";
}

void DScannerSoapDiscovery::parseWsdResponse(const QByteArray &data, const QHostAddress &sender)
{
    Q_UNUSED(data)
    Q_UNUSED(sender)
    qCDebug(dscannerNetwork) << "Parsing WSD response";
}

void DScannerSoapDiscovery::onUdpDataReceived()
{
    qCDebug(dscannerNetwork) << "UDP data received";
}

void DScannerSoapDiscovery::onWsdProbeTimeout()
{
    qCDebug(dscannerNetwork) << "WSD probe timeout";
} 