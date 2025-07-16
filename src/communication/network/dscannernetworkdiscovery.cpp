// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerNetworkDiscovery.h"
#include "dscannernetworkdiscovery_p.h"

#include <QDebug>
#include <QUuid>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>
#include <QRegularExpression>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerNetwork, "deepinscan.network")

// DScannerNetworkDiscoveryPrivate 实现

DScannerNetworkDiscoveryPrivate::DScannerNetworkDiscoveryPrivate(DScannerNetworkDiscovery *q)
    : q_ptr(q)
    , discoveryTimer(new QTimer(q))
    , isDiscovering(false)
    , discoveryInterval(30)
    , networkManager(new QNetworkAccessManager(q))
    , mdnsDiscovery(new DScannerMdnsDiscovery(q))
    , soapDiscovery(new DScannerSoapDiscovery(q))
{
    // 初始化统计信息
    stats.totalDevicesFound = 0;
    stats.mdnsDevicesFound = 0;
    stats.soapDevicesFound = 0;
    stats.activeProbes = 0;
    
    // 默认启用的协议
    enabledProtocols << NetworkProtocol::AirScan 
                     << NetworkProtocol::WSD 
                     << NetworkProtocol::IPP;
}

DScannerNetworkDiscoveryPrivate::~DScannerNetworkDiscoveryPrivate()
{
    cleanup();
}

void DScannerNetworkDiscoveryPrivate::init()
{
    qCDebug(dscannerNetwork) << "Initializing network discovery";
    
    // 连接定时器
    QObject::connect(discoveryTimer, &QTimer::timeout, q_ptr, [this]() {
        // 重新启动发现
        startMdnsDiscovery();
        startSoapDiscovery();
    });
    
    // 连接mDNS发现信号
    QObject::connect(mdnsDiscovery, &DScannerMdnsDiscovery::serviceDiscovered,
                     q_ptr, [this](const QString &serviceName, const QString &serviceType, 
                                  const QHostAddress &address, quint16 port) {
        qCDebug(dscannerNetwork) << "mDNS service discovered:" << serviceName << serviceType << address << port;
        
        // 根据服务类型确定协议
        NetworkProtocol protocol = NetworkProtocol::Unknown;
        if (serviceType == "_uscan._tcp") {
            protocol = NetworkProtocol::AirScan;
        } else if (serviceType == "_ipp._tcp") {
            protocol = NetworkProtocol::IPP;
        } else if (serviceType == "_scanner._tcp") {
            protocol = NetworkProtocol::AirScan;
        }
        
        if (protocol != NetworkProtocol::Unknown) {
            probeDevice(address, port, protocol);
        }
    });
    
    // 连接SOAP发现信号
    QObject::connect(soapDiscovery, &DScannerSoapDiscovery::wsdDeviceDiscovered,
                     q_ptr, [this](const NetworkDeviceInfo &device) {
        qCDebug(dscannerNetwork) << "WSD device discovered:" << device.name << device.ipAddress;
        addDevice(device);
        stats.soapDevicesFound++;
    });
    
    qCDebug(dscannerNetwork) << "Network discovery initialized";
}

void DScannerNetworkDiscoveryPrivate::cleanup()
{
    qCDebug(dscannerNetwork) << "Cleaning up network discovery";
    
    stopAllDiscovery();
    
    // 清理设备列表
    QMutexLocker locker(&deviceMutex);
    devices.clear();
    deviceMap.clear();
}

void DScannerNetworkDiscoveryPrivate::startMdnsDiscovery()
{
    qCDebug(dscannerNetwork) << "Starting mDNS discovery";
    
    if (enabledProtocols.contains(NetworkProtocol::AirScan) ||
        enabledProtocols.contains(NetworkProtocol::IPP)) {
        mdnsDiscovery->startDiscovery();
    }
}

void DScannerNetworkDiscoveryPrivate::startSoapDiscovery()
{
    qCDebug(dscannerNetwork) << "Starting SOAP/WSD discovery";
    
    if (enabledProtocols.contains(NetworkProtocol::WSD) ||
        enabledProtocols.contains(NetworkProtocol::SOAP)) {
        soapDiscovery->startDiscovery();
    }
}

void DScannerNetworkDiscoveryPrivate::stopAllDiscovery()
{
    qCDebug(dscannerNetwork) << "Stopping all discovery";
    
    discoveryTimer->stop();
    mdnsDiscovery->stopDiscovery();
    soapDiscovery->stopDiscovery();
    
    isDiscovering = false;
}

void DScannerNetworkDiscoveryPrivate::addDevice(const NetworkDeviceInfo &device)
{
    QMutexLocker locker(&deviceMutex);
    
    if (deviceMap.contains(device.deviceId)) {
        updateDevice(device);
        return;
    }
    
    devices.append(device);
    deviceMap.insert(device.deviceId, device);
    
    stats.totalDevicesFound++;
    
    locker.unlock();
    
    qCDebug(dscannerNetwork) << "Device added:" << device.name << device.ipAddress;
    emit q_ptr->deviceDiscovered(device);
}

void DScannerNetworkDiscoveryPrivate::updateDevice(const NetworkDeviceInfo &device)
{
    if (!deviceMap.contains(device.deviceId)) {
        return;
    }
    
    NetworkDeviceInfo &existing = deviceMap[device.deviceId];
    existing.lastSeen = QDateTime::currentDateTime();
    existing.isOnline = device.isOnline;
    
    // 更新设备列表中的对应项
    for (int i = 0; i < devices.size(); ++i) {
        if (devices[i].deviceId == device.deviceId) {
            devices[i] = existing;
            break;
        }
    }
    
    qCDebug(dscannerNetwork) << "Device updated:" << device.name;
    emit q_ptr->deviceUpdated(existing);
}

void DScannerNetworkDiscoveryPrivate::removeDevice(const QString &deviceId)
{
    QMutexLocker locker(&deviceMutex);
    
    if (!deviceMap.contains(deviceId)) {
        return;
    }
    
    deviceMap.remove(deviceId);
    
    for (int i = 0; i < devices.size(); ++i) {
        if (devices[i].deviceId == deviceId) {
            devices.removeAt(i);
            break;
        }
    }
    
    locker.unlock();
    
    qCDebug(dscannerNetwork) << "Device removed:" << deviceId;
    emit q_ptr->deviceOffline(deviceId);
}

NetworkDeviceInfo DScannerNetworkDiscoveryPrivate::findDevice(const QString &deviceId) const
{
    QMutexLocker locker(&deviceMutex);
    return deviceMap.value(deviceId);
}

void DScannerNetworkDiscoveryPrivate::probeDevice(const QHostAddress &address, quint16 port, NetworkProtocol protocol)
{
    qCDebug(dscannerNetwork) << "Probing device:" << address << port << static_cast<int>(protocol);
    
    stats.activeProbes++;
    
    switch (protocol) {
    case NetworkProtocol::AirScan:
        probeEsclDevice(address, port);
        break;
    case NetworkProtocol::WSD:
        probeWsdDevice(address, port);
        break;
    case NetworkProtocol::IPP:
        // IPP探测将在后续实现
        break;
    default:
        stats.activeProbes--;
        break;
    }
}

void DScannerNetworkDiscoveryPrivate::probeEsclDevice(const QHostAddress &address, quint16 port)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPort(port);
    url.setPath("/eSCL/ScannerCapabilities");
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "DeepinScan/1.0");
    request.setRawHeader("Accept", "application/xml");
    
    QNetworkReply *reply = networkManager->get(request);
    
    QObject::connect(reply, &QNetworkReply::finished, q_ptr, [this, reply, address, port]() {
        stats.activeProbes--;
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            NetworkDeviceInfo device = parseEsclCapabilities(data, address, port);
            
            if (!device.deviceId.isEmpty()) {
                addDevice(device);
                stats.mdnsDevicesFound++;
            }
        } else {
            qCWarning(dscannerNetwork) << "eSCL probe failed:" << reply->errorString();
        }
        
        reply->deleteLater();
    });
}

void DScannerNetworkDiscoveryPrivate::probeWsdDevice(const QHostAddress &address, quint16 port)
{
    // WSD探测通过UDP多播实现，这里只是占位
    Q_UNUSED(address)
    Q_UNUSED(port)
    stats.activeProbes--;
}

NetworkDeviceInfo DScannerNetworkDiscoveryPrivate::parseEsclCapabilities(const QByteArray &data, const QHostAddress &address, quint16 port)
{
    NetworkDeviceInfo device;
    device.ipAddress = address;
    device.port = port;
    device.protocol = NetworkProtocol::AirScan;
    device.isOnline = true;
    device.lastSeen = QDateTime::currentDateTime();
    
    QXmlStreamReader reader(data);
    
    while (!reader.atEnd()) {
        reader.readNext();
        
        if (reader.isStartElement()) {
            if (reader.name() == "ScannerCapabilities") {
                // 解析扫描仪能力
                while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "ScannerCapabilities")) {
                    reader.readNext();
                    
                    if (reader.isStartElement()) {
                        if (reader.name() == "MakeAndModel") {
                            QString makeModel = reader.readElementText();
                            QStringList parts = makeModel.split(" ", Qt::SkipEmptyParts);
                            if (parts.size() >= 2) {
                                device.manufacturer = parts.first();
                                device.model = parts.mid(1).join(" ");
                            } else {
                                device.manufacturer = makeModel;
                            }
                        } else if (reader.name() == "SerialNumber") {
                            device.serialNumber = reader.readElementText();
                        } else if (reader.name() == "UUID") {
                            device.protocolInfo.uuid = reader.readElementText();
                        } else if (reader.name() == "AdminURI") {
                            device.protocolInfo.presentationUrl = reader.readElementText();
                        } else if (reader.name() == "IconURI") {
                            device.protocolInfo.iconUrl = reader.readElementText();
                        } else if (reader.name() == "ColorModes") {
                            // 解析颜色模式
                            while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "ColorModes")) {
                                reader.readNext();
                                if (reader.isStartElement() && reader.name() == "ColorMode") {
                                    device.capabilities.append(reader.readElementText());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 生成设备ID
    if (device.protocolInfo.uuid.isEmpty()) {
        device.deviceId = QStringLiteral("escl_%1_%2").arg(address.toString()).arg(port);
    } else {
        device.deviceId = QStringLiteral("escl_%1").arg(device.protocolInfo.uuid);
    }
    
    // 设置设备名称
    if (device.name.isEmpty()) {
        device.name = QStringLiteral("%1 %2").arg(device.manufacturer, device.model);
    }
    
    // 设置服务URL
    device.serviceUrl = QUrl(QStringLiteral("http://%1:%2/eSCL").arg(address.toString()).arg(port));
    
    return device;
}

NetworkDeviceInfo DScannerNetworkDiscoveryPrivate::parseWsdProbeMatch(const QByteArray &data, const QHostAddress &address)
{
    NetworkDeviceInfo device;
    device.ipAddress = address;
    device.protocol = NetworkProtocol::WSD;
    device.isOnline = true;
    device.lastSeen = QDateTime::currentDateTime();
    
    QXmlStreamReader reader(data);
    
    // 解析WSD ProbeMatch响应
    while (!reader.atEnd()) {
        reader.readNext();
        
        if (reader.isStartElement()) {
            if (reader.name() == "ProbeMatch") {
                // 解析ProbeMatch内容
                while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "ProbeMatch")) {
                    reader.readNext();
                    
                    if (reader.isStartElement()) {
                        if (reader.name() == "EndpointReference") {
                            device.protocolInfo.uuid = reader.readElementText();
                        } else if (reader.name() == "Types") {
                            QString types = reader.readElementText();
                            device.capabilities = types.split(" ", Qt::SkipEmptyParts);
                        } else if (reader.name() == "XAddrs") {
                            QString xaddrs = reader.readElementText();
                            device.serviceUrl = QUrl(xaddrs.split(" ").first());
                        }
                    }
                }
            }
        }
    }
    
    // 生成设备ID
    device.deviceId = QStringLiteral("wsd_%1").arg(device.protocolInfo.uuid);
    
    return device;
}

QList<QHostAddress> DScannerNetworkDiscoveryPrivate::getLocalNetworkAddresses()
{
    QList<QHostAddress> addresses;
    
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            for (const QNetworkAddressEntry &entry : entries) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    addresses.append(entry.ip());
                }
            }
        }
    }
    
    return addresses;
}

QString DScannerNetworkDiscoveryPrivate::generateUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// DScannerNetworkDiscovery 实现

DScannerNetworkDiscovery::DScannerNetworkDiscovery(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerNetworkDiscoveryPrivate(this))
{
    Q_D(DScannerNetworkDiscovery);
    d->init();
}

DScannerNetworkDiscovery::~DScannerNetworkDiscovery()
{
    Q_D(DScannerNetworkDiscovery);
    d->cleanup();
}

void DScannerNetworkDiscovery::startDiscovery()
{
    Q_D(DScannerNetworkDiscovery);
    
    if (d->isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Starting network discovery";
    
    d->isDiscovering = true;
    d->stats.lastDiscoveryTime = QDateTime::currentDateTime();
    
    // 启动各种发现方式
    d->startMdnsDiscovery();
    d->startSoapDiscovery();
    
    // 启动定时发现
    d->discoveryTimer->start(d->discoveryInterval * 1000);
    
    // 立即执行一次发现
    d->startMdnsDiscovery();
    d->startSoapDiscovery();
}

void DScannerNetworkDiscovery::stopDiscovery()
{
    Q_D(DScannerNetworkDiscovery);
    
    if (!d->isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Stopping network discovery";
    
    d->stopAllDiscovery();
    
    emit discoveryFinished();
}

bool DScannerNetworkDiscovery::isDiscovering() const
{
    Q_D(const DScannerNetworkDiscovery);
    return d->isDiscovering;
}

QList<NetworkDeviceInfo> DScannerNetworkDiscovery::discoveredDevices() const
{
    Q_D(const DScannerNetworkDiscovery);
    QMutexLocker locker(&d->deviceMutex);
    return d->devices;
}

NetworkDeviceInfo DScannerNetworkDiscovery::deviceInfo(const QString &deviceId) const
{
    Q_D(const DScannerNetworkDiscovery);
    return d->findDevice(deviceId);
}

void DScannerNetworkDiscovery::refreshDevice(const QString &deviceId)
{
    Q_D(DScannerNetworkDiscovery);
    
    NetworkDeviceInfo device = d->findDevice(deviceId);
    if (device.deviceId.isEmpty()) {
        return;
    }
    
    // 重新探测设备
    d->probeDevice(device.ipAddress, device.port, device.protocol);
}

void DScannerNetworkDiscovery::setDiscoveryInterval(int seconds)
{
    Q_D(DScannerNetworkDiscovery);
    d->discoveryInterval = seconds;
    
    if (d->isDiscovering) {
        d->discoveryTimer->setInterval(seconds * 1000);
    }
}

int DScannerNetworkDiscovery::discoveryInterval() const
{
    Q_D(const DScannerNetworkDiscovery);
    return d->discoveryInterval;
}

void DScannerNetworkDiscovery::setEnabledProtocols(const QList<NetworkProtocol> &protocols)
{
    Q_D(DScannerNetworkDiscovery);
    d->enabledProtocols = protocols;
}

QList<NetworkProtocol> DScannerNetworkDiscovery::enabledProtocols() const
{
    Q_D(const DScannerNetworkDiscovery);
    return d->enabledProtocols;
}

bool DScannerNetworkDiscovery::addDevice(const QHostAddress &address, quint16 port, NetworkProtocol protocol)
{
    Q_D(DScannerNetworkDiscovery);
    
    // 手动探测设备
    d->probeDevice(address, port, protocol);
    
    return true;
}

void DScannerNetworkDiscovery::removeDevice(const QString &deviceId)
{
    Q_D(DScannerNetworkDiscovery);
    d->removeDevice(deviceId);
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

 