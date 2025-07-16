// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QLoggingCategory>

#include "Scanner/DScannerNetworkDiscovery.h"

DSCANNER_USE_NAMESPACE

/**
 * \brief 网络设备发现示例程序
 * 
 * 演示如何使用DScannerNetworkDiscovery类进行网络扫描设备发现
 */
class NetworkDiscoveryExample : public QObject
{
    Q_OBJECT
    
public:
    explicit NetworkDiscoveryExample(QObject *parent = nullptr)
        : QObject(parent)
        , discovery(new DScannerNetworkDiscovery(this))
    {
        setupDiscovery();
    }
    
    void start()
    {
        qDebug() << "=== DeepinScan 网络设备发现示例 ===";
        qDebug() << "正在启动网络设备发现...";
        
        // 设置发现间隔为60秒
        discovery->setDiscoveryInterval(60);
        
        // 设置启用的协议
        QList<NetworkProtocol> protocols = {
            NetworkProtocol::AirScan,
            NetworkProtocol::WSD,
            NetworkProtocol::IPP
        };
        discovery->setEnabledProtocols(protocols);
        
        // 启动发现
        discovery->startDiscovery();
        
        // 设置30秒后停止示例
        QTimer::singleShot(30000, this, &NetworkDiscoveryExample::stop);
    }
    
    void stop()
    {
        qDebug() << "停止网络设备发现";
        discovery->stopDiscovery();
        
        // 显示发现的设备
        showDiscoveredDevices();
        
        // 退出程序
        QCoreApplication::quit();
    }
    
private slots:
    void onDeviceDiscovered(const NetworkDeviceInfo &device)
    {
        qDebug() << "发现网络设备:";
        qDebug() << "  设备ID:" << device.deviceId;
        qDebug() << "  设备名称:" << device.name;
        qDebug() << "  制造商:" << device.manufacturer;
        qDebug() << "  型号:" << device.model;
        qDebug() << "  IP地址:" << device.ipAddress.toString();
        qDebug() << "  端口:" << device.port;
        qDebug() << "  协议:" << protocolToString(device.protocol);
        qDebug() << "  服务URL:" << device.serviceUrl.toString();
        qDebug() << "  设备能力:" << device.capabilities;
        qDebug() << "  是否在线:" << (device.isOnline ? "是" : "否");
        qDebug() << "  最后发现时间:" << device.lastSeen.toString();
        qDebug() << "---";
    }
    
    void onDeviceUpdated(const NetworkDeviceInfo &device)
    {
        qDebug() << "设备状态更新:" << device.name << "(" << device.ipAddress.toString() << ")";
    }
    
    void onDeviceOffline(const QString &deviceId)
    {
        qDebug() << "设备离线:" << deviceId;
    }
    
    void onDiscoveryFinished()
    {
        qDebug() << "网络设备发现完成";
    }
    
    void onErrorOccurred(const QString &error)
    {
        qWarning() << "发现错误:" << error;
    }
    
private:
    void setupDiscovery()
    {
        // 连接信号
        connect(discovery, &DScannerNetworkDiscovery::deviceDiscovered,
                this, &NetworkDiscoveryExample::onDeviceDiscovered);
        
        connect(discovery, &DScannerNetworkDiscovery::deviceUpdated,
                this, &NetworkDiscoveryExample::onDeviceUpdated);
        
        connect(discovery, &DScannerNetworkDiscovery::deviceOffline,
                this, &NetworkDiscoveryExample::onDeviceOffline);
        
        connect(discovery, &DScannerNetworkDiscovery::discoveryFinished,
                this, &NetworkDiscoveryExample::onDiscoveryFinished);
        
        connect(discovery, &DScannerNetworkDiscovery::errorOccurred,
                this, &NetworkDiscoveryExample::onErrorOccurred);
    }
    
    void showDiscoveredDevices()
    {
        QList<NetworkDeviceInfo> devices = discovery->discoveredDevices();
        
        qDebug() << "=== 发现的设备总结 ===";
        qDebug() << "总共发现" << devices.size() << "个网络设备:";
        
        for (int i = 0; i < devices.size(); ++i) {
            const NetworkDeviceInfo &device = devices[i];
            qDebug() << QString("  %1. %2 (%3) - %4")
                        .arg(i + 1)
                        .arg(device.name)
                        .arg(device.ipAddress.toString())
                        .arg(protocolToString(device.protocol));
        }
        
        if (devices.isEmpty()) {
            qDebug() << "  (未发现任何设备)";
        }
    }
    
    QString protocolToString(NetworkProtocol protocol)
    {
        switch (protocol) {
        case NetworkProtocol::AirScan:
            return "AirScan/eSCL";
        case NetworkProtocol::WSD:
            return "WSD";
        case NetworkProtocol::SOAP:
            return "SOAP";
        case NetworkProtocol::IPP:
            return "IPP";
        case NetworkProtocol::SNMP:
            return "SNMP";
        default:
            return "未知";
        }
    }
    
    DScannerNetworkDiscovery *discovery;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 启用网络日志记录
    QLoggingCategory::setFilterRules("deepinscan.network.debug=true");
    
    // 创建并启动示例
    NetworkDiscoveryExample example;
    example.start();
    
    return app.exec();
}

#include "network_discovery_example.moc" 