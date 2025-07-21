// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERNETWORKDISCOVERY_H
#define DSCANNERNETWORKDISCOVERY_H

#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerTypes.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QUrl>
#include <QXmlStreamReader>
#include <QDnsLookup>

DSCANNER_BEGIN_NAMESPACE

/**
 * \enum NetworkProtocol
 * \brief 网络扫描协议类型
 */
enum class NetworkProtocol {
    AirScan,        ///< Apple AirScan (eSCL)
    WSD,            ///< Web Services for Devices
    SOAP,           ///< Simple Object Access Protocol
    IPP,            ///< Internet Printing Protocol
    SNMP,           ///< Simple Network Management Protocol
    Unknown         ///< 未知协议
};

/**
 * \struct NetworkDeviceInfo
 * \brief 网络设备信息结构
 */
struct NetworkDeviceInfo {
    QString deviceId;           ///< 设备ID
    QString name;               ///< 设备名称
    QString manufacturer;       ///< 制造商
    QString model;              ///< 型号
    QString serialNumber;       ///< 序列号
    QHostAddress ipAddress;     ///< IP地址
    quint16 port;              ///< 端口号
    NetworkProtocol protocol;   ///< 支持的协议
    QUrl serviceUrl;           ///< 服务URL
    QStringList capabilities;   ///< 设备能力
    QDateTime lastSeen;         ///< 最后发现时间
    bool isOnline;             ///< 是否在线
    
    // 协议特定信息
    struct {
        QString uuid;           ///< 设备UUID
        QString presentationUrl; ///< 展示URL
        QString iconUrl;        ///< 图标URL
        int version;            ///< 协议版本
    } protocolInfo;
};

/**
 * \class DScannerNetworkDiscovery
 * \brief 网络设备发现类
 * 
 * 负责通过mDNS、SOAP等协议发现网络扫描设备
 */
class DSCANNER_EXPORT DScannerNetworkDiscovery : public QObject
{
    Q_OBJECT
    
public:
    explicit DScannerNetworkDiscovery(QObject *parent = nullptr);
    ~DScannerNetworkDiscovery();
    
    // 发现控制
    void startDiscovery();
    void stopDiscovery();
    bool isDiscovering() const;
    
    // 设备管理
    QList<NetworkDeviceInfo> discoveredDevices() const;
    NetworkDeviceInfo deviceInfo(const QString &deviceId) const;
    void refreshDevice(const QString &deviceId);
    
    // 配置
    void setDiscoveryInterval(int seconds);
    int discoveryInterval() const;
    
    void setEnabledProtocols(const QList<NetworkProtocol> &protocols);
    QList<NetworkProtocol> enabledProtocols() const;
    
    // 手动添加设备
    bool addDevice(const QHostAddress &address, quint16 port, NetworkProtocol protocol);
    void removeDevice(const QString &deviceId);
    
signals:
    /**
     * \brief 发现新设备
     * \param device 设备信息
     */
    void deviceDiscovered(const NetworkDeviceInfo &device);
    
    /**
     * \brief 设备状态更新
     * \param device 设备信息
     */
    void deviceUpdated(const NetworkDeviceInfo &device);
    
    /**
     * \brief 设备离线
     * \param deviceId 设备ID
     */
    void deviceOffline(const QString &deviceId);
    
    /**
     * \brief 发现完成
     */
    void discoveryFinished();
    
    /**
     * \brief 发生错误
     * \param error 错误信息
     */
    void errorOccurred(const QString &error);
    
private slots:
    void onMdnsLookupFinished();
    void onSoapQueryFinished();
    void onDeviceProbeFinished();
    void onNetworkReplyFinished();
    
private:
    class DScannerNetworkDiscoveryPrivate;
    QScopedPointer<DScannerNetworkDiscoveryPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DScannerNetworkDiscovery)
    Q_DISABLE_COPY(DScannerNetworkDiscovery)
};

/**
 * \class DScannerMdnsDiscovery
 * \brief mDNS设备发现类
 * 
 * 专门处理mDNS/Bonjour协议的设备发现
 */
class DSCANNER_EXPORT DScannerMdnsDiscovery : public QObject
{
    Q_OBJECT
    
public:
    explicit DScannerMdnsDiscovery(QObject *parent = nullptr);
    ~DScannerMdnsDiscovery();
    
    // 发现控制
    void startDiscovery();
    void stopDiscovery();
    
    // 服务类型
    static QStringList supportedServiceTypes();
    
signals:
    void serviceDiscovered(const QString &serviceName, const QString &serviceType, const QHostAddress &address, quint16 port);
    void serviceRemoved(const QString &serviceName, const QString &serviceType);
    
private slots:
    void onServiceDiscovered();
    void onServiceRemoved();
    
private:
    class DScannerMdnsDiscoveryPrivate;
    QScopedPointer<DScannerMdnsDiscoveryPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DScannerMdnsDiscovery)
    Q_DISABLE_COPY(DScannerMdnsDiscovery)
};

/**
 * \class DScannerSoapDiscovery
 * \brief SOAP/WSD设备发现类
 * 
 * 专门处理SOAP和WSD协议的设备发现
 */
class DSCANNER_EXPORT DScannerSoapDiscovery : public QObject
{
    Q_OBJECT
    
public:
    explicit DScannerSoapDiscovery(QObject *parent = nullptr);
    ~DScannerSoapDiscovery();
    
    // 发现控制
    void startDiscovery();
    void stopDiscovery();
    
    // WSD探测
    void sendWsdProbe();
    void parseWsdResponse(const QByteArray &data, const QHostAddress &sender);
    
signals:
    void wsdDeviceDiscovered(const NetworkDeviceInfo &device);
    void soapServiceDiscovered(const QUrl &serviceUrl, const QString &deviceType);
    
private slots:
    void onUdpDataReceived();
    void onWsdProbeTimeout();
    
private:
    class DScannerSoapDiscoveryPrivate;
    QScopedPointer<DScannerSoapDiscoveryPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DScannerSoapDiscovery)
    Q_DISABLE_COPY(DScannerSoapDiscovery)
};

DSCANNER_END_NAMESPACE

// 注册元类型
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::NetworkProtocol)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::NetworkDeviceInfo)

#endif // DSCANNERNETWORKDISCOVERY_H 