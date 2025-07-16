// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerNetworkDiscovery.h"
#include "dscannernetworkdiscovery_p.h"

#include <QDebug>
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHostAddress>
#include <QNetworkDatagram>

DSCANNER_USE_NAMESPACE

// DScannerSoapDiscoveryPrivate 实现

DScannerSoapDiscoveryPrivate::DScannerSoapDiscoveryPrivate(DScannerSoapDiscovery *q)
    : q_ptr(q)
    , isDiscovering(false)
    , probeTimer(new QTimer(q))
    , multicastSocket(new QUdpSocket(q))
    , networkManager(new QNetworkAccessManager(q))
{
    // 初始化统计信息
    stats.totalProbesSent = 0;
    stats.totalResponsesReceived = 0;
    stats.validDevicesFound = 0;
    
    // 设置探测定时器
    probeTimer->setInterval(30000); // 30秒间隔
    QObject::connect(probeTimer, &QTimer::timeout, q, [this]() {
        sendWsdProbe();
    });
}

DScannerSoapDiscoveryPrivate::~DScannerSoapDiscoveryPrivate()
{
    cleanup();
}

void DScannerSoapDiscoveryPrivate::init()
{
    qCDebug(dscannerNetwork) << "Initializing SOAP/WSD discovery";
    
    // 连接UDP socket信号
    QObject::connect(multicastSocket, &QUdpSocket::readyRead, q_ptr, &DScannerSoapDiscovery::onUdpDataReceived);
    
    setupMulticastSocket();
}

void DScannerSoapDiscoveryPrivate::cleanup()
{
    qCDebug(dscannerNetwork) << "Cleaning up SOAP/WSD discovery";
    
    stopWsdDiscovery();
    cleanupMulticastSocket();
    
    // 清理活动的SOAP查询
    for (auto it = activeSoapQueries.begin(); it != activeSoapQueries.end(); ++it) {
        it.value()->abort();
        it.value()->deleteLater();
    }
    activeSoapQueries.clear();
    
    // 清理发现的设备
    discoveredDevices.clear();
}

void DScannerSoapDiscoveryPrivate::startWsdDiscovery()
{
    qCDebug(dscannerNetwork) << "Starting WSD discovery";
    
    if (isDiscovering) {
        return;
    }
    
    isDiscovering = true;
    probeTimer->start();
    
    // 立即发送一次探测
    sendWsdProbe();
}

void DScannerSoapDiscoveryPrivate::stopWsdDiscovery()
{
    qCDebug(dscannerNetwork) << "Stopping WSD discovery";
    
    if (!isDiscovering) {
        return;
    }
    
    isDiscovering = false;
    probeTimer->stop();
}

void DScannerSoapDiscoveryPrivate::sendWsdProbe()
{
    qCDebug(dscannerNetwork) << "Sending WSD probe";
    
    // 生成唯一的消息ID
    QString messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // 构造WSD探测消息
    QString probeMessage = WSD::PROBE_MESSAGE.arg(messageId);
    
    QByteArray data = probeMessage.toUtf8();
    
    // 发送到WSD多播地址
    QHostAddress multicastAddr(WSD::MULTICAST_ADDRESS);
    qint64 sent = multicastSocket->writeDatagram(data, multicastAddr, WSD::MULTICAST_PORT);
    
    if (sent == data.size()) {
        stats.totalProbesSent++;
        stats.lastProbeTime = QDateTime::currentDateTime();
        qCDebug(dscannerNetwork) << "WSD probe sent successfully";
    } else {
        qCWarning(dscannerNetwork) << "Failed to send WSD probe";
    }
}

void DScannerSoapDiscoveryPrivate::processWsdResponse(const QByteArray &data, const QHostAddress &sender)
{
    qCDebug(dscannerNetwork) << "Processing WSD response from:" << sender;
    
    stats.totalResponsesReceived++;
    
    QXmlStreamReader reader(data);
    NetworkDeviceInfo device = parseWsdProbeMatch(reader, sender);
    
    if (!device.deviceId.isEmpty()) {
        // 检查是否是扫描仪设备
        bool isScanner = false;
        for (const QString &capability : device.capabilities) {
            if (capability.contains("scan", Qt::CaseInsensitive) ||
                capability.contains("scanner", Qt::CaseInsensitive)) {
                isScanner = true;
                break;
            }
        }
        
        if (isScanner) {
            discoveredDevices.insert(device.deviceId, device);
            stats.validDevicesFound++;
            
            qCDebug(dscannerNetwork) << "Valid scanner device found:" << device.name;
            emit q_ptr->wsdDeviceDiscovered(device);
        }
    }
}

void DScannerSoapDiscoveryPrivate::querySoapService(const QUrl &serviceUrl)
{
    qCDebug(dscannerNetwork) << "Querying SOAP service:" << serviceUrl;
    
    // 构造SOAP查询请求
    QNetworkRequest request(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/soap+xml; charset=utf-8");
    request.setHeader(QNetworkRequest::UserAgentHeader, "DeepinScan/1.0");
    request.setRawHeader("SOAPAction", "\"http://schemas.xmlsoap.org/ws/2004/09/transfer/Get\"");
    
    // SOAP查询消息
    QString soapMessage = 
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        "xmlns:wsx=\"http://schemas.xmlsoap.org/ws/2004/09/mex\">"
        "<soap:Header>"
        "<wsa:Action>http://schemas.xmlsoap.org/ws/2004/09/mex/GetMetadata/Request</wsa:Action>"
        "<wsa:MessageID>urn:uuid:%1</wsa:MessageID>"
        "<wsa:To>%2</wsa:To>"
        "</soap:Header>"
        "<soap:Body>"
        "<wsx:GetMetadata/>"
        "</soap:Body>"
        "</soap:Envelope>";
    
    QString messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QByteArray postData = soapMessage.arg(messageId, serviceUrl.toString()).toUtf8();
    
    QNetworkReply *reply = networkManager->post(request, postData);
    activeSoapQueries.insert(serviceUrl, reply);
    
    QObject::connect(reply, &QNetworkReply::finished, q_ptr, [this, reply, serviceUrl]() {
        activeSoapQueries.remove(serviceUrl);
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            processSoapResponse(data, serviceUrl);
        } else {
            qCWarning(dscannerNetwork) << "SOAP query failed:" << reply->errorString();
        }
        
        reply->deleteLater();
    });
}

void DScannerSoapDiscoveryPrivate::processSoapResponse(const QByteArray &data, const QUrl &serviceUrl)
{
    qCDebug(dscannerNetwork) << "Processing SOAP response from:" << serviceUrl;
    
    QXmlStreamReader reader(data);
    
    // 解析SOAP响应
    while (!reader.atEnd()) {
        reader.readNext();
        
        if (reader.isStartElement()) {
            if (reader.name() == "Metadata") {
                // 解析元数据
                while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "Metadata")) {
                    reader.readNext();
                    
                    if (reader.isStartElement()) {
                        if (reader.name() == "MetadataSection") {
                            // 处理元数据段
                            QString dialect = reader.attributes().value("Dialect").toString();
                            if (dialect.contains("scan", Qt::CaseInsensitive)) {
                                emit q_ptr->soapServiceDiscovered(serviceUrl, dialect);
                            }
                        }
                    }
                }
            }
        }
    }
}

NetworkDeviceInfo DScannerSoapDiscoveryPrivate::parseWsdProbeMatch(QXmlStreamReader &reader, const QHostAddress &address)
{
    NetworkDeviceInfo device;
    device.ipAddress = address;
    device.protocol = NetworkProtocol::WSD;
    device.isOnline = true;
    device.lastSeen = QDateTime::currentDateTime();
    
    while (!reader.atEnd()) {
        reader.readNext();
        
        if (reader.isStartElement()) {
            if (reader.name() == "ProbeMatch") {
                // 解析ProbeMatch响应
                while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "ProbeMatch")) {
                    reader.readNext();
                    
                    if (reader.isStartElement()) {
                        if (reader.name() == "EndpointReference") {
                            device.protocolInfo.uuid = parseWsdEndpointReference(reader);
                        } else if (reader.name() == "Types") {
                            device.capabilities = parseWsdTypes(reader);
                        } else if (reader.name() == "XAddrs") {
                            QString xaddrs = reader.readElementText();
                            QStringList addresses = xaddrs.split(" ", Qt::SkipEmptyParts);
                            if (!addresses.isEmpty()) {
                                device.serviceUrl = QUrl(addresses.first());
                                device.port = device.serviceUrl.port(80);
                            }
                        } else if (reader.name() == "MetadataVersion") {
                            device.protocolInfo.version = reader.readElementText().toInt();
                        }
                    }
                }
            }
        }
    }
    
    // 生成设备ID
    if (!device.protocolInfo.uuid.isEmpty()) {
        device.deviceId = QStringLiteral("wsd_%1").arg(device.protocolInfo.uuid);
    } else {
        device.deviceId = QStringLiteral("wsd_%1_%2").arg(address.toString()).arg(device.port);
    }
    
    // 从capabilities中提取设备信息
    for (const QString &capability : device.capabilities) {
        if (capability.contains("scan", Qt::CaseInsensitive)) {
            device.name = QStringLiteral("WSD Scanner (%1)").arg(address.toString());
            break;
        }
    }
    
    return device;
}

QStringList DScannerSoapDiscoveryPrivate::parseWsdTypes(QXmlStreamReader &reader)
{
    QStringList types;
    QString typesText = reader.readElementText();
    
    // 解析类型列表
    QStringList typeList = typesText.split(" ", Qt::SkipEmptyParts);
    for (const QString &type : typeList) {
        types.append(type.trimmed());
    }
    
    return types;
}

QString DScannerSoapDiscoveryPrivate::parseWsdEndpointReference(QXmlStreamReader &reader)
{
    QString endpointRef;
    
    while (!reader.atEnd() && !(reader.isEndElement() && reader.name() == "EndpointReference")) {
        reader.readNext();
        
        if (reader.isStartElement() && reader.name() == "Address") {
            endpointRef = reader.readElementText();
            break;
        }
    }
    
    return endpointRef;
}

void DScannerSoapDiscoveryPrivate::setupMulticastSocket()
{
    qCDebug(dscannerNetwork) << "Setting up multicast socket";
    
    // 绑定到多播端口
    if (!multicastSocket->bind(QHostAddress::AnyIPv4, WSD::MULTICAST_PORT, QUdpSocket::ShareAddress)) {
        qCWarning(dscannerNetwork) << "Failed to bind multicast socket:" << multicastSocket->errorString();
        return;
    }
    
    // 加入多播组
    QHostAddress multicastAddr(WSD::MULTICAST_ADDRESS);
    if (!multicastSocket->joinMulticastGroup(multicastAddr)) {
        qCWarning(dscannerNetwork) << "Failed to join multicast group:" << multicastSocket->errorString();
        return;
    }
    
    qCDebug(dscannerNetwork) << "Multicast socket setup successful";
}

void DScannerSoapDiscoveryPrivate::cleanupMulticastSocket()
{
    qCDebug(dscannerNetwork) << "Cleaning up multicast socket";
    
    if (multicastSocket->state() == QAbstractSocket::BoundState) {
        QHostAddress multicastAddr(WSD::MULTICAST_ADDRESS);
        multicastSocket->leaveMulticastGroup(multicastAddr);
        multicastSocket->close();
    }
}

// DScannerSoapDiscovery 实现

DScannerSoapDiscovery::DScannerSoapDiscovery(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerSoapDiscoveryPrivate(this))
{
    Q_D(DScannerSoapDiscovery);
    d->init();
}

DScannerSoapDiscovery::~DScannerSoapDiscovery()
{
    Q_D(DScannerSoapDiscovery);
    d->cleanup();
}

void DScannerSoapDiscovery::startDiscovery()
{
    Q_D(DScannerSoapDiscovery);
    d->startWsdDiscovery();
}

void DScannerSoapDiscovery::stopDiscovery()
{
    Q_D(DScannerSoapDiscovery);
    d->stopWsdDiscovery();
}

void DScannerSoapDiscovery::sendWsdProbe()
{
    Q_D(DScannerSoapDiscovery);
    d->sendWsdProbe();
}

void DScannerSoapDiscovery::parseWsdResponse(const QByteArray &data, const QHostAddress &sender)
{
    Q_D(DScannerSoapDiscovery);
    d->processWsdResponse(data, sender);
}

void DScannerSoapDiscovery::onUdpDataReceived()
{
    Q_D(DScannerSoapDiscovery);
    
    while (d->multicastSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = d->multicastSocket->receiveDatagram();
        
        if (datagram.isValid()) {
            QByteArray data = datagram.data();
            QHostAddress sender = datagram.senderAddress();
            
            qCDebug(dscannerNetwork) << "Received UDP data from:" << sender;
            
            // 检查是否是WSD响应
            if (data.contains("ProbeMatch") || data.contains("Hello")) {
                d->processWsdResponse(data, sender);
            }
        }
    }
}

void DScannerSoapDiscovery::onWsdProbeTimeout()
{
    Q_D(DScannerSoapDiscovery);
    
    qCDebug(dscannerNetwork) << "WSD probe timeout";
    
    // 重新发送探测
    d->sendWsdProbe();
}

 