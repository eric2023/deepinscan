// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QLoggingCategory>

#include "Scanner/DScannerNetworkDiscovery_Simple.h"

DSCANNER_USE_NAMESPACE

/**
 * \brief 简单的网络设备发现示例程序
 */
class SimpleNetworkDiscoveryExample : public QObject
{
    Q_OBJECT
    
public:
    explicit SimpleNetworkDiscoveryExample(QObject *parent = nullptr)
        : QObject(parent)
        , discovery(new DScannerNetworkDiscoverySimple(this))
    {
        setupDiscovery();
    }
    
    void start()
    {
        qDebug() << "=== DeepinScan 简单网络设备发现示例 ===";
        qDebug() << "正在启动网络设备发现...";
        
        // 设置发现间隔为10秒
        discovery->setDiscoveryInterval(10);
        
        // 设置启用的协议
        QList<NetworkProtocol> protocols = {
            NetworkProtocol::AirScan,
            NetworkProtocol::WSD,
            NetworkProtocol::IPP
        };
        discovery->setEnabledProtocols(protocols);
        
        // 启动发现
        discovery->startDiscovery();
        
        // 设置20秒后停止示例
        QTimer::singleShot(20000, this, &SimpleNetworkDiscoveryExample::stop);
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
        qDebug() << "  是否在线:" << (device.isOnline ? "是" : "否");
        qDebug() << "  最后发现时间:" << device.lastSeen.toString();
        qDebug() << "---";
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
        connect(discovery, &DScannerNetworkDiscoverySimple::deviceDiscovered,
                this, &SimpleNetworkDiscoveryExample::onDeviceDiscovered);
        
        connect(discovery, &DScannerNetworkDiscoverySimple::deviceOffline,
                this, &SimpleNetworkDiscoveryExample::onDeviceOffline);
        
        connect(discovery, &DScannerNetworkDiscoverySimple::discoveryFinished,
                this, &SimpleNetworkDiscoveryExample::onDiscoveryFinished);
        
        connect(discovery, &DScannerNetworkDiscoverySimple::errorOccurred,
                this, &SimpleNetworkDiscoveryExample::onErrorOccurred);
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
    
    DScannerNetworkDiscoverySimple *discovery;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 启用网络日志记录
    QLoggingCategory::setFilterRules("deepinscan.network.debug=true");
    
    // 创建并启动示例
    SimpleNetworkDiscoveryExample example;
    example.start();
    
    return app.exec();
}

// MOC文件将由Qt构建系统自动生成 