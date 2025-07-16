// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERNETWORKDISCOVERY_P_H
#define DSCANNERNETWORKDISCOVERY_P_H

#include "Scanner/DScannerNetworkDiscovery.h"

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUdpSocket>
#include <QDnsLookup>
#include <QXmlStreamReader>
#include <QMutex>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dscannerNetwork)

// WSD相关常量
namespace WSD {
    const quint16 MULTICAST_PORT = 3702;
    const QString MULTICAST_ADDRESS = "239.255.255.250";
    const QString PROBE_MESSAGE = 
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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
        "<wsd:Types>scan:ScannerServiceType</wsd:Types>"
        "</wsd:Probe>"
        "</soap:Body>"
        "</soap:Envelope>";
}

// mDNS相关常量
namespace MDNS {
    const QStringList SERVICE_TYPES = {
        "_uscan._tcp",          // AirScan/eSCL
        "_ipp._tcp",            // IPP
        "_scanner._tcp",        // Generic scanner
        "_pdl-datastream._tcp", // PDL datastream
        "_privet._tcp"          // Google Cloud Print
    };
}

/**
 * \class DScannerNetworkDiscoveryPrivate
 * \brief 网络设备发现私有实现类
 */
class DScannerNetworkDiscoveryPrivate
{
    Q_DECLARE_PUBLIC(DScannerNetworkDiscovery)
    
public:
    explicit DScannerNetworkDiscoveryPrivate(DScannerNetworkDiscovery *q);
    ~DScannerNetworkDiscoveryPrivate();
    
    // 初始化和清理
    void init();
    void cleanup();
    
    // 发现控制
    void startMdnsDiscovery();
    void startSoapDiscovery();
    void stopAllDiscovery();
    
    // 设备管理
    void addDevice(const NetworkDeviceInfo &device);
    void updateDevice(const NetworkDeviceInfo &device);
    void removeDevice(const QString &deviceId);
    NetworkDeviceInfo findDevice(const QString &deviceId) const;
    
    // 设备探测
    void probeDevice(const QHostAddress &address, quint16 port, NetworkProtocol protocol);
    void probeEsclDevice(const QHostAddress &address, quint16 port);
    void probeWsdDevice(const QHostAddress &address, quint16 port);
    
    // 协议解析
    NetworkDeviceInfo parseEsclCapabilities(const QByteArray &data, const QHostAddress &address, quint16 port);
    NetworkDeviceInfo parseWsdProbeMatch(const QByteArray &data, const QHostAddress &address);
    
    // 网络工具
    QList<QHostAddress> getLocalNetworkAddresses();
    QString generateUuid();
    
    // 公共成员
    DScannerNetworkDiscovery *q_ptr;
    
    // 发现控制
    QTimer *discoveryTimer;
    bool isDiscovering;
    int discoveryInterval;
    QList<NetworkProtocol> enabledProtocols;
    
    // 设备数据
    QList<NetworkDeviceInfo> devices;
    QHash<QString, NetworkDeviceInfo> deviceMap;
    mutable QMutex deviceMutex;
    
    // 网络组件
    QNetworkAccessManager *networkManager;
    DScannerMdnsDiscovery *mdnsDiscovery;
    DScannerSoapDiscovery *soapDiscovery;
    
    // 统计信息
    struct {
        int totalDevicesFound;
        int mdnsDevicesFound;
        int soapDevicesFound;
        int activeProbes;
        QDateTime lastDiscoveryTime;
    } stats;
};

/**
 * \class DScannerMdnsDiscoveryPrivate
 * \brief mDNS设备发现私有实现类
 */
class DScannerMdnsDiscoveryPrivate
{
    Q_DECLARE_PUBLIC(DScannerMdnsDiscovery)
    
public:
    explicit DScannerMdnsDiscoveryPrivate(DScannerMdnsDiscovery *q);
    ~DScannerMdnsDiscoveryPrivate();
    
    // 初始化
    void init();
    void cleanup();
    
    // 发现控制
    void startServiceDiscovery();
    void stopServiceDiscovery();
    
    // DNS查询
    void lookupService(const QString &serviceType);
    void processServiceRecord(const QDnsServiceRecord &record);
    void processHostRecord(const QDnsHostAddressRecord &record, const QString &serviceName);
    
    // 服务管理
    void addService(const QString &serviceName, const QString &serviceType, 
                   const QHostAddress &address, quint16 port);
    void removeService(const QString &serviceName, const QString &serviceType);
    
    // 公共成员
    DScannerMdnsDiscovery *q_ptr;
    
    // 发现状态
    bool isDiscovering;
    QTimer *discoveryTimer;
    
    // DNS查询
    QList<QDnsLookup*> activeLookups;
    QHash<QString, QDnsLookup*> serviceLookups;
    
    // 服务数据
    struct ServiceInfo {
        QString name;
        QString type;
        QHostAddress address;
        quint16 port;
        QDateTime lastSeen;
    };
    QHash<QString, ServiceInfo> services;
    
    // 统计信息
    struct {
        int totalServicesFound;
        int activeQueries;
        QDateTime lastQueryTime;
    } stats;
};

/**
 * \class DScannerSoapDiscoveryPrivate
 * \brief SOAP/WSD设备发现私有实现类
 */
class DScannerSoapDiscoveryPrivate
{
    Q_DECLARE_PUBLIC(DScannerSoapDiscovery)
    
public:
    explicit DScannerSoapDiscoveryPrivate(DScannerSoapDiscovery *q);
    ~DScannerSoapDiscoveryPrivate();
    
    // 初始化
    void init();
    void cleanup();
    
    // WSD发现
    void startWsdDiscovery();
    void stopWsdDiscovery();
    void sendWsdProbe();
    void processWsdResponse(const QByteArray &data, const QHostAddress &sender);
    
    // SOAP查询
    void querySoapService(const QUrl &serviceUrl);
    void processSoapResponse(const QByteArray &data, const QUrl &serviceUrl);
    
    // XML解析
    NetworkDeviceInfo parseWsdProbeMatch(QXmlStreamReader &reader, const QHostAddress &address);
    QStringList parseWsdTypes(QXmlStreamReader &reader);
    QString parseWsdEndpointReference(QXmlStreamReader &reader);
    
    // 网络工具
    void setupMulticastSocket();
    void cleanupMulticastSocket();
    
    // 公共成员
    DScannerSoapDiscovery *q_ptr;
    
    // 发现状态
    bool isDiscovering;
    QTimer *probeTimer;
    
    // 网络组件
    QUdpSocket *multicastSocket;
    QNetworkAccessManager *networkManager;
    
    // WSD数据
    QHash<QString, NetworkDeviceInfo> discoveredDevices;
    QHash<QUrl, QNetworkReply*> activeSoapQueries;
    
    // 统计信息
    struct {
        int totalProbesSent;
        int totalResponsesReceived;
        int validDevicesFound;
        QDateTime lastProbeTime;
    } stats;
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERNETWORKDISCOVERY_P_H 