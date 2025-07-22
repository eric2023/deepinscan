// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "network_complete_discovery.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QThread>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QEventLoop>
#include <QUuid>
#include <QSslSocket>
#include <QNetworkProxy>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(networkCompleteDiscovery, "deepinscan.network.complete")

// NetworkCompleteDiscovery 实现

NetworkCompleteDiscovery::NetworkCompleteDiscovery(QObject *parent)
    : QObject(parent)
    , m_isDiscovering(false)
    , m_discoveryInterval(30000)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_discoveryTimer(new QTimer(this))
    , m_activeProbes(0)
    , m_threadPool(new QThreadPool(this))
{
    qCDebug(networkCompleteDiscovery) << "初始化网络完整发现引擎";
    
    // 设置线程池大小
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount() * 2);
    
    // 初始化协议支持
    initializeProtocolSupport();
    
    // 初始化发现计时器
    m_discoveryTimer->setSingleShot(false);
    m_discoveryTimer->setInterval(m_discoveryInterval);
    connect(m_discoveryTimer, &QTimer::timeout, this, &NetworkCompleteDiscovery::performPeriodicDiscovery);
    
    // 初始化网络管理器
    // m_networkManager->setTransferTimeout(10000); // Qt 5.15+才支持，暂时注释
    connect(m_networkManager, &QNetworkAccessManager::finished, 
            this, &NetworkCompleteDiscovery::handleNetworkReply);
    
    // 初始化统计信息
    resetStatistics();
    
    qCDebug(networkCompleteDiscovery) << "网络完整发现引擎初始化完成";
}

NetworkCompleteDiscovery::~NetworkCompleteDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "销毁网络完整发现引擎";
    stopDiscovery();
    
    // 等待所有活动探测完成
    m_threadPool->waitForDone(5000);
}

void NetworkCompleteDiscovery::initializeProtocolSupport()
{
    qCDebug(networkCompleteDiscovery) << "初始化协议支持";
    
    // mDNS/Bonjour支持
    m_supportedProtocols[static_cast<int>(ProtocolType::MDNS)] = QStringList() <<
        "_ipp._tcp.local" << // AirScan/AirPrint
        "_scanner._tcp.local" << // 通用扫描仪服务
        "_ipp._tcp.local" << // IPP打印/扫描
        "_pdl-datastream._tcp.local" << // HP JetDirect
        "_http._tcp.local"; // HTTP服务
    
    // WS-Discovery (WSD) 支持
    m_supportedProtocols[static_cast<int>(ProtocolType::WSD)] = QStringList() <<
        "urn:schemas-xmlsoap-org:ws:2005:04:discovery" <<
        "http://schemas.microsoft.com/windows/2006/08/wdp/scan";
    
    // SOAP/XML-RPC支持
    m_supportedProtocols[static_cast<int>(ProtocolType::SOAP)] = QStringList() <<
        "/eSCL/ScannerCapabilities" <<
        "/WS/ScannerConfiguration" << 
        "/scan/status" <<
        "/scan/jobs";
    
    // SNMP支持
    m_supportedProtocols[static_cast<int>(ProtocolType::SNMP)] = QStringList() <<
        "1.3.6.1.2.1.25.3.2.1.3" << // hrDeviceDescr
        "1.3.6.1.2.1.1.1.0" << // sysDescr
        "1.3.6.1.4.1.1536" << // Canon MIB
        "1.3.6.1.4.1.11.2.3.9.4.2.1.1.3.3.0"; // HP LaserJet MIB
    
    // UPnP支持
    m_supportedProtocols[static_cast<int>(ProtocolType::UPNP)] = QStringList() <<
        "urn:schemas-upnp-org:device:Scanner:1" <<
        "urn:schemas-upnp-org:device:PrinterEnhanced:1" <<
        "urn:schemas-microsoft-com:device:Scanner:1";
    
    qCDebug(networkCompleteDiscovery) << "协议支持初始化完成，支持" 
                                      << m_supportedProtocols.size() << "种协议";
}

bool NetworkCompleteDiscovery::startDiscovery()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_isDiscovering) {
        qCWarning(networkCompleteDiscovery) << "发现过程已在进行中";
        return false;
    }
    
    qCDebug(networkCompleteDiscovery) << "开始网络发现";
    
    m_isDiscovering = true;
    resetStatistics();
    
    // 获取网络接口
    updateNetworkInterfaces();
    
    // 启动发现过程
    performCompleteDiscovery();
    
    // 启动定期发现计时器
    m_discoveryTimer->start();
    
    emit discoveryStarted();
    
    qCDebug(networkCompleteDiscovery) << "网络发现已启动";
    return true;
}

void NetworkCompleteDiscovery::stopDiscovery()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isDiscovering) {
        return;
    }
    
    qCDebug(networkCompleteDiscovery) << "停止网络发现";
    
    m_isDiscovering = false;
    m_discoveryTimer->stop();
    
    // 取消所有活动的网络请求
    foreach (QNetworkReply *reply, m_activeReplies) {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    }
    m_activeReplies.clear();
    
    emit discoveryStopped();
    
    qCDebug(networkCompleteDiscovery) << "网络发现已停止";
}

void NetworkCompleteDiscovery::performCompleteDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "执行完整网络发现";
    
    // 重置发现的设备列表
    {
        QMutexLocker locker(&m_deviceMutex);
        m_discoveredDevices.clear();
    }
    
    // 执行各种发现方法
    performMdnsDiscovery();
    performWsdDiscovery();
    performSoapDiscovery();
    performSnmpDiscovery();
    performUpnpDiscovery();
    performPortScanDiscovery();
    
    qCDebug(networkCompleteDiscovery) << "完整网络发现启动完成";
}

void NetworkCompleteDiscovery::performMdnsDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "执行mDNS发现";
    
    foreach (const QString &serviceType, m_supportedProtocols[static_cast<int>(ProtocolType::MDNS)]) {
        // 创建mDNS查询任务
        auto *task = new MdnsDiscoveryTask(serviceType, this);
        connect(task, &MdnsDiscoveryTask::deviceFound, 
                this, &NetworkCompleteDiscovery::onMdnsDeviceFound);
        connect(task, &MdnsDiscoveryTask::finished,
                this, &NetworkCompleteDiscovery::onMdnsDiscoveryFinished);
        
        m_threadPool->start(task);
        m_activeProbes++;
    }
    
    m_statistics.mdnsQueriesSent += m_supportedProtocols[static_cast<int>(ProtocolType::MDNS)].size();
}

void NetworkCompleteDiscovery::performWsdDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "执行WS-Discovery发现";
    
    // WS-Discovery多播消息
    const QString wsdMessage = QString(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        "xmlns:wsd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
        "<soap:Header>"
        "<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"
        "<wsa:MessageID>urn:uuid:%1</wsa:MessageID>"
        "<wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
        "</soap:Header>"
        "<soap:Body>"
        "<wsd:Probe>"
        "<wsd:Types>wsdp:Device</wsd:Types>"
        "</wsd:Probe>"
        "</soap:Body>"
        "</soap:Envelope>"
    ).arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    // 发送到WS-Discovery多播地址
    auto *task = new WsdDiscoveryTask(wsdMessage, this);
    connect(task, &WsdDiscoveryTask::deviceFound,
            this, &NetworkCompleteDiscovery::onWsdDeviceFound);
    connect(task, &WsdDiscoveryTask::finished,
            this, &NetworkCompleteDiscovery::onWsdDiscoveryFinished);
    
    m_threadPool->start(task);
    m_activeProbes++;
    m_statistics.wsdQueriesSent++;
}

void NetworkCompleteDiscovery::performSoapDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "执行SOAP/eSCL发现";
    
    // 对已知的IP范围进行SOAP探测
    foreach (const QNetworkInterface &interface, m_networkInterfaces) {
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                auto *task = new SoapDiscoveryTask(entry, this);
                connect(task, &SoapDiscoveryTask::deviceFound,
                        this, &NetworkCompleteDiscovery::onSoapDeviceFound);
                connect(task, &SoapDiscoveryTask::finished,
                        this, &NetworkCompleteDiscovery::onSoapDiscoveryFinished);
                
                m_threadPool->start(task);
                m_activeProbes++;
                m_statistics.soapQueriesSent++;
            }
        }
    }
}

void NetworkCompleteDiscovery::performSnmpDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "执行SNMP发现";
    
    // SNMP发现实现
    foreach (const QNetworkInterface &interface, m_networkInterfaces) {
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                auto *task = new SnmpDiscoveryTask(entry, this);
                connect(task, &SnmpDiscoveryTask::deviceFound,
                        this, &NetworkCompleteDiscovery::onSnmpDeviceFound);
                connect(task, &SnmpDiscoveryTask::finished,
                        this, &NetworkCompleteDiscovery::onSnmpDiscoveryFinished);
                
                m_threadPool->start(task);
                m_activeProbes++;
                m_statistics.snmpQueriesSent++;
            }
        }
    }
}

void NetworkCompleteDiscovery::performUpnpDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "执行UPnP发现";
    
    // UPnP SSDP发现
    const QString ssdpMessage = QString(
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "ST: upnp:rootdevice\r\n"
        "MX: 3\r\n\r\n"
    );
    
    auto *task = new UpnpDiscoveryTask(ssdpMessage, this);
    connect(task, &UpnpDiscoveryTask::deviceFound,
            this, &NetworkCompleteDiscovery::onUpnpDeviceFound);
    connect(task, &UpnpDiscoveryTask::finished,
            this, &NetworkCompleteDiscovery::onUpnpDiscoveryFinished);
    
    m_threadPool->start(task);
    m_activeProbes++;
    m_statistics.upnpQueriesSent++;
}

void NetworkCompleteDiscovery::performPortScanDiscovery()
{
    qCDebug(networkCompleteDiscovery) << "执行端口扫描发现";
    
    // 常见扫描仪端口
    QList<quint16> scannerPorts = {
        80,   // HTTP
        443,  // HTTPS  
        161,  // SNMP
        515,  // LPD
        631,  // IPP
        8080, // HTTP-Alt
        9100, // HP JetDirect
        9101, // Baird EthernetDirect
        9102, // Lexmark MarkNet
        9600  // Canon BJNP
    };
    
    foreach (const QNetworkInterface &interface, m_networkInterfaces) {
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                auto *task = new PortScanTask(entry, scannerPorts, this);
                connect(task, &PortScanTask::deviceFound,
                        this, &NetworkCompleteDiscovery::onPortScanDeviceFound);
                connect(task, &PortScanTask::finished,
                        this, &NetworkCompleteDiscovery::onPortScanFinished);
                
                m_threadPool->start(task);
                m_activeProbes++;
                m_statistics.portScansSent++;
            }
        }
    }
}

void NetworkCompleteDiscovery::updateNetworkInterfaces()
{
    m_networkInterfaces = QNetworkInterface::allInterfaces();
    
    qCDebug(networkCompleteDiscovery) << "更新网络接口，找到" 
                                      << m_networkInterfaces.size() << "个接口";
    
    foreach (const QNetworkInterface &interface, m_networkInterfaces) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            
            qCDebug(networkCompleteDiscovery) << "活动接口:" << interface.name() 
                                              << "类型:" << interface.type();
        }
    }
}

void NetworkCompleteDiscovery::resetStatistics()
{
    m_statistics = NetworkDiscoveryStatistics();
    m_statistics.discoveryStartTime = QDateTime::currentDateTime();
}

void NetworkCompleteDiscovery::performPeriodicDiscovery()
{
    if (!m_isDiscovering) {
        return;
    }
    
    qCDebug(networkCompleteDiscovery) << "执行定期网络发现";
    performCompleteDiscovery();
}

void NetworkCompleteDiscovery::handleNetworkReply(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }
    
    m_activeReplies.removeOne(reply);
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QUrl url = reply->url();
        
        // 处理HTTP响应数据
        processHttpResponse(url, data);
    } else {
        qCWarning(networkCompleteDiscovery) << "网络请求错误:" << reply->errorString()
                                            << "URL:" << reply->url().toString();
    }
    
    reply->deleteLater();
}

void NetworkCompleteDiscovery::processHttpResponse(const QUrl &url, const QByteArray &data)
{
    qCDebug(networkCompleteDiscovery) << "处理HTTP响应:" << url.toString();
    
    // 尝试解析为扫描仪设备信息
    NetworkScannerDevice device;
    
    if (parseEsclCapabilities(data, device)) {
        device.discoveryMethod = "eSCL";
        device.baseUrl = url;
        addDiscoveredDevice(device);
    } else if (parseWsdResponse(data, device)) {
        device.discoveryMethod = "WSD";
        device.baseUrl = url;
        addDiscoveredDevice(device);
    } else if (parseSnmpResponse(data, device)) {
        device.discoveryMethod = "SNMP";
        device.baseUrl = url;
        addDiscoveredDevice(device);
    }
}

bool NetworkCompleteDiscovery::parseEsclCapabilities(const QByteArray &data, NetworkScannerDevice &device)
{
    QXmlStreamReader xml(data);
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            if (xml.name() == "ScannerCapabilities") {
                device.protocol = "eSCL";
                device.capabilities["eSCL"] = true;
            } else if (xml.name() == "MakeAndModel") {
                device.makeAndModel = xml.readElementText();
            } else if (xml.name() == "SerialNumber") {
                device.serialNumber = xml.readElementText();
            } else if (xml.name() == "UUID") {
                device.uuid = xml.readElementText();
            } else if (xml.name() == "AdminURI") {
                device.adminUri = xml.readElementText();
            } else if (xml.name() == "IconURI") {
                device.iconUri = xml.readElementText();
            }
        }
    }
    
    return !device.makeAndModel.isEmpty();
}

bool NetworkCompleteDiscovery::parseWsdResponse(const QByteArray &data, NetworkScannerDevice &device)
{
    QXmlStreamReader xml(data);
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            if (xml.name() == "ProbeMatches" || xml.name() == "ResolveMatches") {
                device.protocol = "WSD";
                device.capabilities["WSD"] = true;
            } else if (xml.name() == "Types") {
                QString types = xml.readElementText();
                if (types.contains("Scanner") || types.contains("MFP")) {
                    device.deviceType = "Scanner";
                }
            } else if (xml.name() == "Scopes") {
                device.scopes = xml.readElementText();
            } else if (xml.name() == "XAddrs") {
                device.addresses = xml.readElementText().split(' ');
            }
        }
    }
    
    return device.protocol == "WSD";
}

bool NetworkCompleteDiscovery::parseSnmpResponse(const QByteArray &data, NetworkScannerDevice &device)
{
    // SNMP响应解析
    if (data.contains("Scanner") || data.contains("MFP") || data.contains("Printer")) {
        device.protocol = "SNMP";
        device.capabilities["SNMP"] = true;
        device.deviceType = "Scanner";
        return true;
    }
    
    return false;
}

void NetworkCompleteDiscovery::addDiscoveredDevice(const NetworkScannerDevice &device)
{
    QMutexLocker locker(&m_deviceMutex);
    
    // 检查设备是否已存在
    bool deviceExists = false;
    for (int i = 0; i < m_discoveredDevices.size(); ++i) {
        const NetworkScannerDevice &existing = m_discoveredDevices.at(i);
        if (existing.uuid == device.uuid && !device.uuid.isEmpty()) {
            deviceExists = true;
            break;
        }
        if (existing.addresses == device.addresses && !device.addresses.isEmpty()) {
            deviceExists = true;
            break;
        }
    }
    
    if (!deviceExists) {
        m_discoveredDevices.append(device);
        m_statistics.totalDevicesFound++;
        
        qCDebug(networkCompleteDiscovery) << "发现新设备:" << device.makeAndModel
                                          << "协议:" << device.protocol
                                          << "UUID:" << device.uuid;
        
        emit deviceDiscovered(device);
    }
}

QList<NetworkScannerDevice> NetworkCompleteDiscovery::getDiscoveredDevices() const
{
    QMutexLocker locker(&m_deviceMutex);
    return m_discoveredDevices;
}

NetworkDiscoveryStatistics NetworkCompleteDiscovery::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    NetworkDiscoveryStatistics stats = m_statistics;
    stats.discoveryDuration = m_statistics.discoveryStartTime.msecsTo(QDateTime::currentDateTime());
    stats.activeProbes = m_activeProbes;
    return stats;
}

void NetworkCompleteDiscovery::setDiscoveryInterval(int seconds)
{
    m_discoveryInterval = seconds * 1000;
    if (m_discoveryTimer->isActive()) {
        m_discoveryTimer->setInterval(m_discoveryInterval);
    }
}

bool NetworkCompleteDiscovery::isDiscovering() const
{
    QMutexLocker locker(&m_mutex);
    return m_isDiscovering;
}

// 事件处理槽函数实现
void NetworkCompleteDiscovery::onMdnsDeviceFound(const NetworkScannerDevice &device)
{
    m_statistics.mdnsDevicesFound++;
    addDiscoveredDevice(device);
}

void NetworkCompleteDiscovery::onMdnsDiscoveryFinished()
{
    m_activeProbes--;
    checkDiscoveryCompletion();
}

void NetworkCompleteDiscovery::onWsdDeviceFound(const NetworkScannerDevice &device)
{
    m_statistics.wsdDevicesFound++;
    addDiscoveredDevice(device);
}

void NetworkCompleteDiscovery::onWsdDiscoveryFinished()
{
    m_activeProbes--;
    checkDiscoveryCompletion();
}

void NetworkCompleteDiscovery::onSoapDeviceFound(const NetworkScannerDevice &device)
{
    m_statistics.soapDevicesFound++;
    addDiscoveredDevice(device);
}

void NetworkCompleteDiscovery::onSoapDiscoveryFinished()
{
    m_activeProbes--;
    checkDiscoveryCompletion();
}

void NetworkCompleteDiscovery::onSnmpDeviceFound(const NetworkScannerDevice &device)
{
    m_statistics.snmpDevicesFound++;
    addDiscoveredDevice(device);
}

void NetworkCompleteDiscovery::onSnmpDiscoveryFinished()
{
    m_activeProbes--;
    checkDiscoveryCompletion();
}

void NetworkCompleteDiscovery::onUpnpDeviceFound(const NetworkScannerDevice &device)
{
    m_statistics.upnpDevicesFound++;
    addDiscoveredDevice(device);
}

void NetworkCompleteDiscovery::onUpnpDiscoveryFinished()
{
    m_activeProbes--;
    checkDiscoveryCompletion();
}

void NetworkCompleteDiscovery::onPortScanDeviceFound(const NetworkScannerDevice &device)
{
    m_statistics.portScanDevicesFound++;
    addDiscoveredDevice(device);
}

void NetworkCompleteDiscovery::onPortScanFinished()
{
    m_activeProbes--;
    checkDiscoveryCompletion();
}

void NetworkCompleteDiscovery::checkDiscoveryCompletion()
{
    if (m_activeProbes <= 0) {
        qCDebug(networkCompleteDiscovery) << "网络发现周期完成，发现设备数量:" 
                                          << m_discoveredDevices.size();
        emit discoveryCompleted(m_discoveredDevices);
    }
} 