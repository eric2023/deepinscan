// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERNETWORKDISCOVERY_SIMPLE_H
#define DSCANNERNETWORKDISCOVERY_SIMPLE_H

#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerTypes.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QHostAddress>
#include <QUrl>
#include <QMutex>

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
 * \class DScannerNetworkDiscoverySimple
 * \brief 简化的网络设备发现类
 */
class DSCANNER_EXPORT DScannerNetworkDiscoverySimple : public QObject
{
    Q_OBJECT
    
public:
    explicit DScannerNetworkDiscoverySimple(QObject *parent = nullptr);
    ~DScannerNetworkDiscoverySimple();
    
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
    void deviceDiscovered(const NetworkDeviceInfo &device);
    void deviceUpdated(const NetworkDeviceInfo &device);
    void deviceOffline(const QString &deviceId);
    void discoveryFinished();
    void errorOccurred(const QString &error);
    
private slots:
    void performDiscovery();
    
private:
    bool m_isDiscovering;
    int m_discoveryInterval;
    QList<NetworkProtocol> m_enabledProtocols;
    
    QTimer *m_discoveryTimer;
    QNetworkAccessManager *m_networkManager;
    
    QList<NetworkDeviceInfo> m_devices;
    QHash<QString, NetworkDeviceInfo> m_deviceMap;
    mutable QMutex m_deviceMutex;
};

DSCANNER_END_NAMESPACE

// 注册元类型
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::NetworkProtocol)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::NetworkDeviceInfo)

#endif // DSCANNERNETWORKDISCOVERY_SIMPLE_H 