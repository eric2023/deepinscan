//
// SPDX-FileCopyrightText: 2025 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

// 测试设备发现相关头文件
#include "include/Scanner/DScannerGlobal.h"
#include "include/Scanner/DScannerTypes.h"
#include "include/Scanner/DScannerManager.h"
#include "include/Scanner/DScannerUSB.h"
#include "include/Scanner/DScannerNetworkDiscovery.h"

using namespace Dtk::Scanner;

class DeviceDiscoveryTest : public QObject
{
    Q_OBJECT

public:
    DeviceDiscoveryTest(QObject *parent = nullptr)
        : QObject(parent)
        , usbDiscovery(new DScannerUSB(this))
        , networkDiscovery(new DScannerNetworkDiscovery(this))
    {
        // 连接USB设备发现信号
        connect(usbDiscovery, &DScannerUSB::deviceConnected,
                this, &DeviceDiscoveryTest::onUSBDeviceFound);
        
        // 连接网络设备发现信号
        connect(networkDiscovery, &DScannerNetworkDiscovery::deviceDiscovered,
                this, &DeviceDiscoveryTest::onNetworkDeviceFound);
    }

public slots:
    void startDiscovery()
    {
        qDebug() << "=== DeepinScan 设备发现测试开始 ===";
        
        // 1. 测试USB设备发现
        testUSBDiscovery();
        
        // 2. 测试网络设备发现
        testNetworkDiscovery();
        
        // 3. 设置超时定时器
        QTimer::singleShot(5000, this, &DeviceDiscoveryTest::finishTest);
    }

private slots:
    void onUSBDeviceFound(const USBDeviceDescriptor &descriptor)
    {
        qDebug() << "发现USB设备:";
        qDebug() << "  厂商ID:" << QString::number(descriptor.vendorId, 16);
        qDebug() << "  产品ID:" << QString::number(descriptor.productId, 16);
        qDebug() << "  制造商:" << descriptor.manufacturer;
        qDebug() << "  产品名:" << descriptor.product;
        qDebug() << "  序列号:" << descriptor.serialNumber;
        
        usbDeviceCount++;
    }
    
    void onNetworkDeviceFound(const DeviceInfo &deviceInfo)
    {
        qDebug() << "发现网络设备:";
        qDebug() << "  设备ID:" << deviceInfo.deviceId;
        qDebug() << "  名称:" << deviceInfo.name;
        qDebug() << "  制造商:" << deviceInfo.manufacturer;
        qDebug() << "  型号:" << deviceInfo.model;
        qDebug() << "  连接:" << deviceInfo.connectionString;
        
        networkDeviceCount++;
    }
    
    void finishTest()
    {
        qDebug() << "\n=== 设备发现测试结果 ===";
        qDebug() << "USB设备数量:" << usbDeviceCount;
        qDebug() << "网络设备数量:" << networkDeviceCount;
        qDebug() << "总设备数量:" << (usbDeviceCount + networkDeviceCount);
        
        if (usbDeviceCount > 0 || networkDeviceCount > 0) {
            qDebug() << "✅ 设备发现功能正常";
        } else {
            qDebug() << "⚠️  未发现任何设备 (可能正常，取决于环境)";
        }
        
        QCoreApplication::quit();
    }

private:
    void testUSBDiscovery()
    {
        qDebug() << "\n--- 测试USB设备发现 ---";
        
        // 初始化USB子系统
        if (!usbDiscovery->initialize()) {
            qDebug() << "❌ USB子系统初始化失败";
            return;
        }
        
        qDebug() << "✅ USB子系统初始化成功";
        
        // 发现所有USB设备
        auto usbDevices = usbDiscovery->discoverDevices();
        qDebug() << "发现" << usbDevices.size() << "个USB设备";
        
        // 过滤扫描仪设备
        int scannerCount = 0;
        for (const auto &device : usbDevices) {
            if (DScannerUSB::isScannerDevice(device)) {
                onUSBDeviceFound(device);
                scannerCount++;
            }
        }
        
        qDebug() << "其中" << scannerCount << "个是扫描仪设备";
    }
    
    void testNetworkDiscovery()
    {
        qDebug() << "\n--- 测试网络设备发现 ---";
        
        // 启动网络设备发现
        if (!networkDiscovery->startDiscovery()) {
            qDebug() << "❌ 网络设备发现启动失败";
            return;
        }
        
        qDebug() << "✅ 网络设备发现已启动";
        qDebug() << "正在搜索网络扫描仪设备...";
    }

private:
    DScannerUSB *usbDiscovery;
    DScannerNetworkDiscovery *networkDiscovery;
    int usbDeviceCount = 0;
    int networkDeviceCount = 0;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "DeepinScan 设备发现功能测试" << std::endl;
    std::cout << "============================" << std::endl;
    
    DeviceDiscoveryTest test;
    
    // 启动测试
    QTimer::singleShot(100, &test, &DeviceDiscoveryTest::startDiscovery);
    
    return app.exec();
}

#include "test_device_discovery.moc" 