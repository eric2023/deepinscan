// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORK_COMPLETE_DISCOVERY_H
#define NETWORK_COMPLETE_DISCOVERY_H

#include "Scanner/DScannerGlobal.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QDateTime>
#include <QLoggingCategory>
#include <QHash>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(networkCompleteDiscovery)

// 协议类型枚举
enum class ProtocolType {
    MDNS,    // mDNS/Bonjour
    WSD,     // WS-Discovery
    SOAP,    // SOAP/eSCL
    SNMP,    // SNMP
    UPNP,    // UPnP
    HTTP     // HTTP直接访问
};

// 网络扫描仪设备信息结构
struct NetworkScannerDevice {
    QString uuid;                    // 设备唯一标识
    QString makeAndModel;           // 制造商和型号
    QString serialNumber;           // 序列号
    QString deviceType;             // 设备类型 (Scanner/MFP/Printer)
    QString protocol;               // 发现协议
    QString discoveryMethod;        // 发现方法
    QStringList addresses;          // 网络地址列表
    QUrl baseUrl;                   // 基础URL
    QString adminUri;               // 管理界面URI
    QString iconUri;                // 图标URI
    QString scopes;                 // WSD作用域
    QVariantMap capabilities;       // 设备能力
    QVariantMap properties;         // 其他属性
    QDateTime discoveryTime;        // 发现时间
    
    NetworkScannerDevice() : discoveryTime(QDateTime::currentDateTime()) {}
};

// 网络发现统计信息
struct NetworkDiscoveryStatistics {
    int totalDevicesFound = 0;
    int mdnsDevicesFound = 0;
    int wsdDevicesFound = 0;
    int soapDevicesFound = 0;
    int snmpDevicesFound = 0;
    int upnpDevicesFound = 0;
    int portScanDevicesFound = 0;
    
    int mdnsQueriesSent = 0;
    int wsdQueriesSent = 0;
    int soapQueriesSent = 0;
    int snmpQueriesSent = 0;
    int upnpQueriesSent = 0;
    int portScansSent = 0;
    
    int activeProbes = 0;
    QDateTime discoveryStartTime;
    qint64 discoveryDuration = 0; // 毫秒
};

// 前向声明发现任务类
class MdnsDiscoveryTask;
class WsdDiscoveryTask;
class SoapDiscoveryTask;
class SnmpDiscoveryTask;
class UpnpDiscoveryTask;
class PortScanTask;

/**
 * @brief 网络完整发现引擎
 * 
 * 这个类实现了完整的网络扫描仪发现功能，支持多种网络协议和发现机制，
 * 包括mDNS、WS-Discovery、SOAP/eSCL、SNMP、UPnP和端口扫描。
 */
class DSCANNER_EXPORT NetworkCompleteDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit NetworkCompleteDiscovery(QObject *parent = nullptr);
    ~NetworkCompleteDiscovery();

    /**
     * @brief 开始网络发现
     * @return 是否成功启动发现过程
     */
    bool startDiscovery();

    /**
     * @brief 停止网络发现
     */
    void stopDiscovery();

    /**
     * @brief 设置发现间隔
     * @param seconds 间隔秒数
     */
    void setDiscoveryInterval(int seconds);

    /**
     * @brief 检查是否正在发现
     * @return 是否正在发现
     */
    bool isDiscovering() const;

    /**
     * @brief 获取已发现的设备列表
     * @return 设备列表
     */
    QList<NetworkScannerDevice> getDiscoveredDevices() const;

    /**
     * @brief 获取发现统计信息
     * @return 统计信息
     */
    NetworkDiscoveryStatistics getStatistics() const;

Q_SIGNALS:
    /**
     * @brief 发现开始信号
     */
    void discoveryStarted();

    /**
     * @brief 发现停止信号
     */
    void discoveryStopped();

    /**
     * @brief 发现新设备信号
     * @param device 发现的设备
     */
    void deviceDiscovered(const NetworkScannerDevice &device);

    /**
     * @brief 发现完成信号
     * @param devices 所有发现的设备
     */
    void discoveryCompleted(const QList<NetworkScannerDevice> &devices);

private Q_SLOTS:
    void performPeriodicDiscovery();
    void handleNetworkReply(QNetworkReply *reply);
    
    // 各协议发现任务完成处理
    void onMdnsDeviceFound(const NetworkScannerDevice &device);
    void onMdnsDiscoveryFinished();
    void onWsdDeviceFound(const NetworkScannerDevice &device);
    void onWsdDiscoveryFinished();
    void onSoapDeviceFound(const NetworkScannerDevice &device);
    void onSoapDiscoveryFinished();
    void onSnmpDeviceFound(const NetworkScannerDevice &device);
    void onSnmpDiscoveryFinished();
    void onUpnpDeviceFound(const NetworkScannerDevice &device);
    void onUpnpDiscoveryFinished();
    void onPortScanDeviceFound(const NetworkScannerDevice &device);
    void onPortScanFinished();

private:
    /**
     * @brief 初始化协议支持
     */
    void initializeProtocolSupport();

    /**
     * @brief 执行完整网络发现
     */
    void performCompleteDiscovery();

    /**
     * @brief 执行mDNS发现
     */
    void performMdnsDiscovery();

    /**
     * @brief 执行WS-Discovery发现
     */
    void performWsdDiscovery();

    /**
     * @brief 执行SOAP/eSCL发现
     */
    void performSoapDiscovery();

    /**
     * @brief 执行SNMP发现
     */
    void performSnmpDiscovery();

    /**
     * @brief 执行UPnP发现
     */
    void performUpnpDiscovery();

    /**
     * @brief 执行端口扫描发现
     */
    void performPortScanDiscovery();

    /**
     * @brief 更新网络接口信息
     */
    void updateNetworkInterfaces();

    /**
     * @brief 重置统计信息
     */
    void resetStatistics();

    /**
     * @brief 处理HTTP响应
     * @param url 请求URL
     * @param data 响应数据
     */
    void processHttpResponse(const QUrl &url, const QByteArray &data);

    /**
     * @brief 解析eSCL能力响应
     * @param data XML数据
     * @param device 设备信息输出
     * @return 是否成功解析
     */
    bool parseEsclCapabilities(const QByteArray &data, NetworkScannerDevice &device);

    /**
     * @brief 解析WS-Discovery响应
     * @param data XML数据
     * @param device 设备信息输出
     * @return 是否成功解析
     */
    bool parseWsdResponse(const QByteArray &data, NetworkScannerDevice &device);

    /**
     * @brief 解析SNMP响应
     * @param data 响应数据
     * @param device 设备信息输出
     * @return 是否成功解析
     */
    bool parseSnmpResponse(const QByteArray &data, NetworkScannerDevice &device);

    /**
     * @brief 添加发现的设备
     * @param device 设备信息
     */
    void addDiscoveredDevice(const NetworkScannerDevice &device);

    /**
     * @brief 检查发现完成状态
     */
    void checkDiscoveryCompletion();

private:
    mutable QMutex m_mutex;                          // 主互斥锁
    mutable QMutex m_deviceMutex;                    // 设备列表互斥锁
    
    bool m_isDiscovering;                            // 是否正在发现
    int m_discoveryInterval;                         // 发现间隔(毫秒)
    int m_activeProbes;                              // 活动探测数量
    
    QNetworkAccessManager *m_networkManager;        // 网络管理器
    QTimer *m_discoveryTimer;                       // 发现计时器
    QThreadPool *m_threadPool;                      // 线程池
    
    QList<QNetworkInterface> m_networkInterfaces;   // 网络接口列表
    QList<QNetworkReply*> m_activeReplies;          // 活动网络请求
    
    QHash<int, QStringList> m_supportedProtocols; // 支持的协议 (使用int代替ProtocolType以避免qHash问题)
    QList<NetworkScannerDevice> m_discoveredDevices;       // 发现的设备
    NetworkDiscoveryStatistics m_statistics;               // 统计信息
};

// 发现任务基类
class DiscoveryTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit DiscoveryTask(QObject *parent = nullptr) : QObject(parent) {
        setAutoDelete(true);
    }

Q_SIGNALS:
    void deviceFound(const NetworkScannerDevice &device);
    void finished();
};

// mDNS发现任务
class MdnsDiscoveryTask : public DiscoveryTask
{
    Q_OBJECT
public:
    explicit MdnsDiscoveryTask(const QString &serviceType, QObject *parent = nullptr);
    void run() override;

private:
    QString m_serviceType;
    NetworkScannerDevice parseMdnsResponse(const QByteArray &data, const QHostAddress &sender);
};

// WS-Discovery发现任务
class WsdDiscoveryTask : public DiscoveryTask
{
    Q_OBJECT
public:
    explicit WsdDiscoveryTask(const QString &message, QObject *parent = nullptr);
    void run() override;

private:
    QString m_message;
    NetworkScannerDevice parseWsdResponse(const QByteArray &data, const QHostAddress &sender);
};

// SOAP发现任务
class SoapDiscoveryTask : public DiscoveryTask
{
    Q_OBJECT
public:
    explicit SoapDiscoveryTask(const QNetworkAddressEntry &entry, QObject *parent = nullptr);
    void run() override;

private:
    QNetworkAddressEntry m_networkEntry;
    NetworkScannerDevice parseEsclResponse(const QByteArray &data, const QHostAddress &address);
};

// SNMP发现任务
class SnmpDiscoveryTask : public DiscoveryTask
{
    Q_OBJECT
public:
    explicit SnmpDiscoveryTask(const QNetworkAddressEntry &entry, QObject *parent = nullptr);
    void run() override;

private:
    QNetworkAddressEntry m_networkEntry;
};

// UPnP发现任务
class UpnpDiscoveryTask : public DiscoveryTask
{
    Q_OBJECT
public:
    explicit UpnpDiscoveryTask(const QString &message, QObject *parent = nullptr);
    void run() override;

private:
    QString m_message;
    NetworkScannerDevice parseUpnpResponse(const QByteArray &data, const QHostAddress &sender);
};

// 端口扫描任务
class PortScanTask : public DiscoveryTask
{
    Q_OBJECT
public:
    explicit PortScanTask(const QNetworkAddressEntry &entry, 
                         const QList<quint16> &ports, 
                         QObject *parent = nullptr);
    void run() override;

private:
    QNetworkAddressEntry m_networkEntry;
    QList<quint16> m_ports;
    bool isKnownScannerPort(quint16 port);
};

DSCANNER_END_NAMESPACE

Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::NetworkScannerDevice)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::NetworkDiscoveryStatistics)

#endif // NETWORK_COMPLETE_DISCOVERY_H 