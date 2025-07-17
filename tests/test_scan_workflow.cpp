//
// SPDX-FileCopyrightText: 2025 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QImage>

// 测试扫描工作流程相关头文件
#include "include/Scanner/DScannerGlobal.h"
#include "include/Scanner/DScannerTypes.h"
#include "include/Scanner/DScannerManager.h"
#include "include/Scanner/DScannerDevice.h"
#include "include/Scanner/DScannerDriver.h"

using namespace Dtk::Scanner;

class ScanWorkflowTest : public QObject
{
    Q_OBJECT

public:
    ScanWorkflowTest(QObject *parent = nullptr)
        : QObject(parent)
        , manager(DScannerManager::instance())
        , currentDevice(nullptr)
    {
    }

public slots:
    void startTest()
    {
        qDebug() << "=== DeepinScan 扫描工作流程测试开始 ===";
        
        // 1. 初始化扫描管理器
        testManagerInitialization();
        
        // 2. 设备发现
        testDeviceDiscovery();
        
        // 3. 设备连接
        testDeviceConnection();
        
        // 4. 参数设置
        testParameterConfiguration();
        
        // 5. 扫描测试
        testScanOperation();
        
        // 6. 设置超时
        QTimer::singleShot(10000, this, &ScanWorkflowTest::finishTest);
    }

private slots:
    void onScanCompleted(const QImage &image)
    {
        qDebug() << "✅ 扫描完成!";
        qDebug() << "图像尺寸:" << image.size();
        qDebug() << "图像格式:" << image.format();
        
        scanCompleted = true;
        finishTest();
    }
    
    void onScanError(const QString &error)
    {
        qDebug() << "❌ 扫描错误:" << error;
        scanError = true;
        finishTest();
    }
    
    void finishTest()
    {
        qDebug() << "\n=== 扫描工作流程测试结果 ===";
        qDebug() << "管理器初始化:" << (managerInitialized ? "✅" : "❌");
        qDebug() << "设备发现:" << (devicesFound ? "✅" : "❌");
        qDebug() << "设备连接:" << (deviceConnected ? "✅" : "❌");
        qDebug() << "参数配置:" << (parametersConfigured ? "✅" : "❌");
        qDebug() << "扫描操作:" << (scanCompleted ? "✅" : (scanError ? "❌" : "⏸️"));
        
        if (currentDevice) {
            currentDevice->close();
        }
        
        manager->shutdown();
        QCoreApplication::quit();
    }

private:
    void testManagerInitialization()
    {
        qDebug() << "\n--- 测试管理器初始化 ---";
        
        try {
            if (manager->initialize()) {
                qDebug() << "✅ 扫描管理器初始化成功";
                qDebug() << "管理器版本:" << manager->version();
                managerInitialized = true;
            } else {
                qDebug() << "❌ 扫描管理器初始化失败";
            }
        } catch (const DScannerException &e) {
            qDebug() << "❌ 初始化异常:" << e.errorMessage();
        }
    }
    
    void testDeviceDiscovery()
    {
        qDebug() << "\n--- 测试设备发现 ---";
        
        if (!managerInitialized) {
            qDebug() << "⚠️ 跳过设备发现测试 (管理器未初始化)";
            return;
        }
        
        try {
            auto devices = manager->discoverDevices(false); // 不使用缓存
            qDebug() << "发现" << devices.size() << "个扫描设备";
            
            if (!devices.isEmpty()) {
                devicesFound = true;
                firstDevice = devices.first();
                
                qDebug() << "第一个设备信息:";
                qDebug() << "  设备ID:" << firstDevice.deviceId;
                qDebug() << "  名称:" << firstDevice.name;
                qDebug() << "  制造商:" << firstDevice.manufacturer;
                qDebug() << "  型号:" << firstDevice.model;
                qDebug() << "  可用性:" << firstDevice.isAvailable;
            } else {
                qDebug() << "⚠️ 未发现任何扫描设备";
            }
        } catch (const DScannerException &e) {
            qDebug() << "❌ 设备发现异常:" << e.errorMessage();
        }
    }
    
    void testDeviceConnection()
    {
        qDebug() << "\n--- 测试设备连接 ---";
        
        if (!devicesFound) {
            qDebug() << "⚠️ 跳过设备连接测试 (无可用设备)";
            return;
        }
        
        try {
            currentDevice = manager->openDevice(firstDevice.deviceId);
            
            if (currentDevice) {
                qDebug() << "✅ 设备连接成功";
                qDebug() << "设备状态:" << (currentDevice->isReady() ? "就绪" : "未就绪");
                
                auto status = currentDevice->status();
                qDebug() << "设备详细状态:";
                qDebug() << "  是否就绪:" << status.isReady;
                qDebug() << "  是否扫描中:" << status.isScanning;
                qDebug() << "  是否有错误:" << status.hasError;
                
                deviceConnected = true;
                
                // 连接扫描信号
                connect(currentDevice, &DScannerDevice::scanCompleted,
                        this, &ScanWorkflowTest::onScanCompleted);
                connect(currentDevice, &DScannerDevice::errorOccurred,
                        this, &ScanWorkflowTest::onScanError);
                
            } else {
                qDebug() << "❌ 设备连接失败";
            }
        } catch (const DScannerException &e) {
            qDebug() << "❌ 设备连接异常:" << e.errorMessage();
        }
    }
    
    void testParameterConfiguration()
    {
        qDebug() << "\n--- 测试参数配置 ---";
        
        if (!deviceConnected || !currentDevice) {
            qDebug() << "⚠️ 跳过参数配置测试 (设备未连接)";
            return;
        }
        
        try {
            // 获取设备能力
            auto capabilities = currentDevice->capabilities();
            qDebug() << "设备能力信息:";
            qDebug() << "  支持的分辨率数:" << capabilities.supportedResolutions.size();
            qDebug() << "  支持的颜色模式数:" << capabilities.supportedColorModes.size();
            qDebug() << "  最大扫描区域:" << capabilities.maxScanArea.width << "x" << capabilities.maxScanArea.height;
            
            // 设置扫描参数
            ScanParameters params;
            params.resolution = 150; // 使用较低分辨率进行测试
            params.colorMode = ColorMode::Color;
            params.format = ImageFormat::JPEG;
            params.area = ScanArea(0, 0, 100, 100); // 小区域测试
            
            if (currentDevice->setScanParameters(params)) {
                qDebug() << "✅ 扫描参数设置成功";
                qDebug() << "  分辨率:" << params.resolution;
                qDebug() << "  颜色模式:" << static_cast<int>(params.colorMode);
                qDebug() << "  输出格式:" << static_cast<int>(params.format);
                
                parametersConfigured = true;
            } else {
                qDebug() << "❌ 扫描参数设置失败";
            }
        } catch (const DScannerException &e) {
            qDebug() << "❌ 参数配置异常:" << e.errorMessage();
        }
    }
    
    void testScanOperation()
    {
        qDebug() << "\n--- 测试扫描操作 ---";
        
        if (!parametersConfigured || !currentDevice) {
            qDebug() << "⚠️ 跳过扫描操作测试 (参数未配置)";
            return;
        }
        
        try {
            qDebug() << "开始扫描测试...";
            qDebug() << "注意: 这只是一个功能测试，不需要实际扫描仪设备";
            
            // 启动扫描操作
            if (currentDevice->startScan()) {
                qDebug() << "✅ 扫描启动成功";
                qDebug() << "等待扫描完成或超时...";
            } else {
                qDebug() << "❌ 扫描启动失败";
                scanError = true;
            }
        } catch (const DScannerException &e) {
            qDebug() << "❌ 扫描操作异常:" << e.errorMessage();
            scanError = true;
        }
    }

private:
    DScannerManager *manager;
    DScannerDevice *currentDevice;
    DeviceInfo firstDevice;
    
    // 测试状态标记
    bool managerInitialized = false;
    bool devicesFound = false;
    bool deviceConnected = false;
    bool parametersConfigured = false;
    bool scanCompleted = false;
    bool scanError = false;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "DeepinScan 扫描工作流程测试" << std::endl;
    std::cout << "============================" << std::endl;
    
    ScanWorkflowTest test;
    
    // 启动测试
    QTimer::singleShot(100, &test, &ScanWorkflowTest::startTest);
    
    return app.exec();
}

#include "test_scan_workflow.moc" 