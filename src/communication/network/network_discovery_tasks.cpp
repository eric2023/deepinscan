// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "network_complete_discovery.h"
#include <QDebug>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QEventLoop>
#include <QTimer>
#include <QDnsLookup>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamReader>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>

DSCANNER_USE_NAMESPACE

// MdnsDiscoveryTask 实现
MdnsDiscoveryTask::MdnsDiscoveryTask(const QString &serviceType, QObject *parent)
    : DiscoveryTask(parent)
    , m_serviceType(serviceType)
{
}

void MdnsDiscoveryTask::run()
{
    qDebug() << "执行mDNS发现:" << m_serviceType;
    
    // 创建UDP套接字用于mDNS查询
    QUdpSocket socket;
    
    // mDNS多播地址和端口
    QHostAddress mdnsAddress("224.0.0.251");
    quint16 mdnsPort = 5353;
    
    // 构建mDNS查询消息
    QByteArray queryMessage;
    QDataStream stream(&queryMessage, QIODevice::WriteOnly);
    
    // mDNS消息头
    stream << quint16(0x1234);  // Transaction ID
    stream << quint16(0x0000);  // Flags (Standard Query)
    stream << quint16(0x0001);  // Questions count
    stream << quint16(0x0000);  // Answer RRs
    stream << quint16(0x0000);  // Authority RRs
    stream << quint16(0x0000);  // Additional RRs
    
    // 查询部分 - 服务类型
    QStringList labels = m_serviceType.split('.');
    for (const QString &label : labels) {
        if (!label.isEmpty()) {
            QByteArray labelBytes = label.toUtf8();
            stream << quint8(labelBytes.length());
            stream.writeRawData(labelBytes.constData(), labelBytes.length());
        }
    }
    stream << quint8(0x00);     // End of name
    stream << quint16(0x000C);  // Type: PTR
    stream << quint16(0x0001);  // Class: IN
    
    // 发送查询
    if (socket.writeDatagram(queryMessage, mdnsAddress, mdnsPort) == -1) {
        qWarning() << "mDNS查询发送失败:" << socket.errorString();
        emit finished();
        return;
    }
    
    // 监听响应
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(3000); // 3秒超时
    
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QByteArray response;
            response.resize(socket.pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;
            
            socket.readDatagram(response.data(), response.size(), &sender, &senderPort);
            
            // 解析mDNS响应
            NetworkScannerDevice device = parseMdnsResponse(response, sender);
            if (!device.makeAndModel.isEmpty()) {
                device.protocol = "mDNS";
                device.discoveryMethod = "mDNS/" + m_serviceType;
                emit deviceFound(device);
            }
        }
        loop.quit();
    });
    
    timeout.start();
    loop.exec();
    
    emit finished();
}

NetworkScannerDevice MdnsDiscoveryTask::parseMdnsResponse(const QByteArray &data, const QHostAddress &sender)
{
    NetworkScannerDevice device;
    
    // 简化的mDNS响应解析
    if (data.contains("scanner") || data.contains("Canon") || data.contains("HP") || 
        data.contains("Epson") || data.contains("Brother")) {
        
        device.addresses << sender.toString();
        device.makeAndModel = "网络扫描仪";
        device.deviceType = "Scanner";
        device.uuid = QUuid::createUuid().toString();
        
        // 尝试提取更多信息
        QString dataStr = QString::fromUtf8(data);
        QStringList lines = dataStr.split('\n');
        
        for (const QString &line : lines) {
            if (line.contains("model", Qt::CaseInsensitive)) {
                device.makeAndModel = line.trimmed();
                break;
            }
        }
    }
    
    return device;
}

// WsdDiscoveryTask 实现
WsdDiscoveryTask::WsdDiscoveryTask(const QString &message, QObject *parent)
    : DiscoveryTask(parent)
    , m_message(message)
{
}

void WsdDiscoveryTask::run()
{
    qDebug() << "执行WS-Discovery发现";
    
    // 创建UDP套接字用于WS-Discovery
    QUdpSocket socket;
    
    // WS-Discovery多播地址和端口
    QHostAddress wsdAddress("239.255.255.250");
    quint16 wsdPort = 3702;
    
    // 发送WS-Discovery Probe消息
    QByteArray message = m_message.toUtf8();
    if (socket.writeDatagram(message, wsdAddress, wsdPort) == -1) {
        qWarning() << "WS-Discovery查询发送失败:" << socket.errorString();
        emit finished();
        return;
    }
    
    // 监听响应
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(5000); // 5秒超时
    
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QByteArray response;
            response.resize(socket.pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;
            
            socket.readDatagram(response.data(), response.size(), &sender, &senderPort);
            
            // 解析WSD响应
            NetworkScannerDevice device = parseWsdResponse(response, sender);
            if (!device.makeAndModel.isEmpty()) {
                device.protocol = "WSD";
                device.discoveryMethod = "WS-Discovery";
                emit deviceFound(device);
            }
        }
        loop.quit();
    });
    
    timeout.start();
    loop.exec();
    
    emit finished();
}

NetworkScannerDevice WsdDiscoveryTask::parseWsdResponse(const QByteArray &data, const QHostAddress &sender)
{
    NetworkScannerDevice device;
    
    QXmlStreamReader xml(data);
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            if (xml.name() == "ProbeMatches" || xml.name() == "ResolveMatches") {
                device.protocol = "WSD";
                device.addresses << sender.toString();
            } else if (xml.name() == "Types") {
                QString types = xml.readElementText();
                if (types.contains("Scanner") || types.contains("MFP")) {
                    device.deviceType = "Scanner";
                    device.makeAndModel = "WS-Discovery 扫描仪";
                    device.uuid = QUuid::createUuid().toString();
                }
            } else if (xml.name() == "Scopes") {
                device.scopes = xml.readElementText();
            } else if (xml.name() == "XAddrs") {
                device.addresses = xml.readElementText().split(' ', Qt::SkipEmptyParts);
            }
        }
    }
    
    return device;
}

// SoapDiscoveryTask 实现
SoapDiscoveryTask::SoapDiscoveryTask(const QNetworkAddressEntry &entry, QObject *parent)
    : DiscoveryTask(parent)
    , m_networkEntry(entry)
{
}

void SoapDiscoveryTask::run()
{
    qDebug() << "执行SOAP/eSCL发现，网段:" << m_networkEntry.ip().toString();
    
    QNetworkAccessManager manager;
    
    // 计算网段范围
    QHostAddress networkAddr = m_networkEntry.ip();
    QHostAddress netmask = m_networkEntry.netmask();
    
    quint32 network = networkAddr.toIPv4Address() & netmask.toIPv4Address();
    quint32 mask = netmask.toIPv4Address();
    quint32 hostBits = ~mask;
    
    // 限制扫描范围以避免过多的网络请求
    int maxHosts = qMin(254, static_cast<int>(hostBits));
    
    for (int i = 1; i <= maxHosts && i <= 50; ++i) { // 限制为前50个IP
        quint32 hostAddr = network | i;
        QHostAddress targetAddr(hostAddr);
        
        // 检查常见的eSCL端口
        QStringList esclPaths = {
            "/eSCL/ScannerCapabilities",
            "/WS/ScannerConfiguration", 
            "/scan/status"
        };
        
        for (const QString &path : esclPaths) {
            QUrl url;
            url.setScheme("http");
            url.setHost(targetAddr.toString());
            url.setPort(80);
            url.setPath(path);
            
            QNetworkRequest request(url);
            request.setRawHeader("User-Agent", "DeepinScan/1.0");
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
            
            QNetworkReply *reply = manager.get(request);
            
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            timeout.setInterval(2000); // 2秒超时
            
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            
            timeout.start();
            loop.exec();
            
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                NetworkScannerDevice device = parseEsclResponse(data, targetAddr);
                if (!device.makeAndModel.isEmpty()) {
                    device.protocol = "eSCL";
                    device.discoveryMethod = "SOAP/eSCL";
                    device.baseUrl = QUrl(url.toString(QUrl::RemoveFilename));
                    emit deviceFound(device);
                }
            }
            
            reply->deleteLater();
        }
    }
    
    emit finished();
}

NetworkScannerDevice SoapDiscoveryTask::parseEsclResponse(const QByteArray &data, const QHostAddress &address)
{
    NetworkScannerDevice device;
    
    QXmlStreamReader xml(data);
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            if (xml.name() == "ScannerCapabilities") {
                device.protocol = "eSCL";
                device.addresses << address.toString();
                device.deviceType = "Scanner";
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
    
    if (device.uuid.isEmpty() && !device.makeAndModel.isEmpty()) {
        device.uuid = QUuid::createUuid().toString();
    }
    
    return device;
}

// SnmpDiscoveryTask 实现
SnmpDiscoveryTask::SnmpDiscoveryTask(const QNetworkAddressEntry &entry, QObject *parent)
    : DiscoveryTask(parent)
    , m_networkEntry(entry)
{
}

void SnmpDiscoveryTask::run()
{
    qDebug() << "执行SNMP发现，网段:" << m_networkEntry.ip().toString();
    
    // 计算网段范围
    QHostAddress networkAddr = m_networkEntry.ip();
    QHostAddress netmask = m_networkEntry.netmask();
    
    quint32 network = networkAddr.toIPv4Address() & netmask.toIPv4Address();
    quint32 mask = netmask.toIPv4Address();
    quint32 hostBits = ~mask;
    
    // 限制扫描范围
    int maxHosts = qMin(254, static_cast<int>(hostBits));
    
    for (int i = 1; i <= maxHosts && i <= 30; ++i) { // 限制为前30个IP
        quint32 hostAddr = network | i;
        QHostAddress targetAddr(hostAddr);
        
        // 尝试SNMP查询
        QUdpSocket socket;
        
        // 构建简单的SNMP GET请求 (sysDescr OID: 1.3.6.1.2.1.1.1.0)
        QByteArray snmpRequest;
        // 这里应该构建正确的SNMP PDU，为简化起见使用简化版本
        
        if (socket.writeDatagram(snmpRequest, targetAddr, 161) != -1) {
            // 监听SNMP响应
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            timeout.setInterval(1000); // 1秒超时
            
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            connect(&socket, &QUdpSocket::readyRead, [&]() {
                QByteArray response;
                response.resize(socket.pendingDatagramSize());
                socket.readDatagram(response.data(), response.size());
                
                // 简化的SNMP响应检查
                if (response.contains("Canon") || response.contains("HP") ||
                    response.contains("Epson") || response.contains("Brother") ||
                    response.contains("Scanner") || response.contains("MFP")) {
                    
                    NetworkScannerDevice device;
                    device.protocol = "SNMP";
                    device.discoveryMethod = "SNMP";
                    device.addresses << targetAddr.toString();
                    device.makeAndModel = "SNMP 扫描仪";
                    device.deviceType = "Scanner";
                    device.uuid = QUuid::createUuid().toString();
                    
                    emit deviceFound(device);
                }
                loop.quit();
            });
            
            timeout.start();
            loop.exec();
        }
    }
    
    emit finished();
}

// UpnpDiscoveryTask 实现
UpnpDiscoveryTask::UpnpDiscoveryTask(const QString &message, QObject *parent)
    : DiscoveryTask(parent)
    , m_message(message)
{
}

void UpnpDiscoveryTask::run()
{
    qDebug() << "执行UPnP发现";
    
    // 创建UDP套接字用于UPnP SSDP
    QUdpSocket socket;
    
    // UPnP SSDP多播地址和端口
    QHostAddress upnpAddress("239.255.255.250");
    quint16 upnpPort = 1900;
    
    // 发送SSDP M-SEARCH消息
    QByteArray message = m_message.toUtf8();
    if (socket.writeDatagram(message, upnpAddress, upnpPort) == -1) {
        qWarning() << "UPnP SSDP查询发送失败:" << socket.errorString();
        emit finished();
        return;
    }
    
    // 监听响应
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(3000); // 3秒超时
    
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QByteArray response;
            response.resize(socket.pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;
            
            socket.readDatagram(response.data(), response.size(), &sender, &senderPort);
            
            // 解析UPnP响应
            NetworkScannerDevice device = parseUpnpResponse(response, sender);
            if (!device.makeAndModel.isEmpty()) {
                device.protocol = "UPnP";
                device.discoveryMethod = "UPnP/SSDP";
                emit deviceFound(device);
            }
        }
        loop.quit();
    });
    
    timeout.start();
    loop.exec();
    
    emit finished();
}

NetworkScannerDevice UpnpDiscoveryTask::parseUpnpResponse(const QByteArray &data, const QHostAddress &sender)
{
    NetworkScannerDevice device;
    
    QString response = QString::fromUtf8(data);
    QStringList lines = response.split('\n');
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        
        if (trimmedLine.startsWith("ST:", Qt::CaseInsensitive)) {
            QString serviceType = trimmedLine.mid(3).trimmed();
            if (serviceType.contains("Scanner", Qt::CaseInsensitive) ||
                serviceType.contains("Printer", Qt::CaseInsensitive)) {
                
                device.deviceType = "Scanner";
                device.makeAndModel = "UPnP 扫描仪";
                device.addresses << sender.toString();
                device.uuid = QUuid::createUuid().toString();
            }
        } else if (trimmedLine.startsWith("LOCATION:", Qt::CaseInsensitive)) {
            device.baseUrl = QUrl(trimmedLine.mid(9).trimmed());
        } else if (trimmedLine.startsWith("USN:", Qt::CaseInsensitive)) {
            QString usn = trimmedLine.mid(4).trimmed();
            if (usn.contains("uuid:")) {
                device.uuid = usn;
            }
        }
    }
    
    return device;
}

// PortScanTask 实现
PortScanTask::PortScanTask(const QNetworkAddressEntry &entry, const QList<quint16> &ports, QObject *parent)
    : DiscoveryTask(parent)
    , m_networkEntry(entry)
    , m_ports(ports)
{
}

void PortScanTask::run()
{
    qDebug() << "执行端口扫描发现，网段:" << m_networkEntry.ip().toString();
    
    // 计算网段范围
    QHostAddress networkAddr = m_networkEntry.ip();
    QHostAddress netmask = m_networkEntry.netmask();
    
    quint32 network = networkAddr.toIPv4Address() & netmask.toIPv4Address();
    quint32 mask = netmask.toIPv4Address();
    quint32 hostBits = ~mask;
    
    // 限制扫描范围
    int maxHosts = qMin(254, static_cast<int>(hostBits));
    
    for (int i = 1; i <= maxHosts && i <= 20; ++i) { // 限制为前20个IP
        quint32 hostAddr = network | i;
        QHostAddress targetAddr(hostAddr);
        
        for (quint16 port : m_ports) {
            QTcpSocket socket;
            socket.connectToHost(targetAddr, port);
            
            if (socket.waitForConnected(1000)) { // 1秒连接超时
                qDebug() << "发现开放端口:" << targetAddr.toString() << ":" << port;
                
                // 检查是否是扫描仪相关的端口
                if (isKnownScannerPort(port)) {
                    NetworkScannerDevice device;
                    device.protocol = "TCP";
                    device.discoveryMethod = "端口扫描";
                    device.addresses << targetAddr.toString();
                    device.makeAndModel = QString("端口 %1 扫描仪").arg(port);
                    device.deviceType = "Scanner";
                    device.uuid = QUuid::createUuid().toString();
                    device.baseUrl = QUrl(QString("http://%1:%2").arg(targetAddr.toString()).arg(port));
                    
                    emit deviceFound(device);
                }
                
                socket.disconnectFromHost();
            }
        }
    }
    
    emit finished();
}

bool PortScanTask::isKnownScannerPort(quint16 port)
{
    // 已知的扫描仪端口
    QList<quint16> scannerPorts = {
        80,   // HTTP
        443,  // HTTPS  
        515,  // LPD
        631,  // IPP
        8080, // HTTP-Alt
        9100, // HP JetDirect
        9101, // Baird EthernetDirect
        9102, // Lexmark MarkNet
        9600  // Canon BJNP
    };
    
    return scannerPorts.contains(port);
}

#include "network_discovery_tasks.moc" 