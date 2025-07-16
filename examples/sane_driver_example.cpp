// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QDebug>
#include <QTimer>

#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerSANE.h"
#include "Scanner/DScannerDevice.h"

using namespace Dtk::Scanner;

class SANEExample : public QObject
{
    Q_OBJECT

public:
    SANEExample(QObject *parent = nullptr) : QObject(parent)
    {
        // 创建扫描仪管理器
        manager = new DScannerManager(this);
        
        // 创建SANE适配器
        sane = new DScannerSANE(this);
        
        // 连接信号
        connect(sane, &DScannerSANE::deviceDiscovered, 
                this, &SANEExample::onDeviceDiscovered);
        connect(sane, &DScannerSANE::errorOccurred,
                this, &SANEExample::onErrorOccurred);
    }

public slots:
    void run()
    {
        qDebug() << "=== DeepinScan SANE驱动示例 ===";
        
        // 初始化SANE子系统
        int versionCode = 0;
        if (sane->init(&versionCode)) {
            qDebug() << "SANE初始化成功，版本:" << QString("0x%1").arg(versionCode, 0, 16);
            
            // 发现设备
            discoverDevices();
        } else {
            qDebug() << "SANE初始化失败";
            QApplication::quit();
        }
    }

private slots:
    void discoverDevices()
    {
        qDebug() << "\n--- 发现SANE设备 ---";
        
        QList<SANEDevice> devices = sane->getDevices(false);
        
        if (devices.isEmpty()) {
            qDebug() << "未发现SANE设备";
            
            // 演示基本功能
            demonstrateBasicFeatures();
        } else {
            qDebug() << "发现" << devices.size() << "个SANE设备:";
            
            for (const auto &device : devices) {
                qDebug() << "  设备:" << device.name;
                qDebug() << "  厂商:" << device.vendor;
                qDebug() << "  型号:" << device.model;
                qDebug() << "  类型:" << device.type;
                qDebug() << "";
                
                // 尝试打开第一个设备
                if (devices.indexOf(device) == 0) {
                    testDevice(device);
                }
            }
        }
    }
    
    void testDevice(const SANEDevice &device)
    {
        qDebug() << "--- 测试设备:" << device.name << "---";
        
        void *handle = sane->openDevice(device.name);
        if (handle) {
            qDebug() << "设备打开成功";
            
            // 获取设备参数
            SANEParameters params = sane->getParameters(handle);
            qDebug() << "设备参数:";
            qDebug() << "  格式:" << params.format;
            qDebug() << "  每行字节数:" << params.bytesPerLine;
            qDebug() << "  每行像素数:" << params.pixelsPerLine;
            qDebug() << "  行数:" << params.lines;
            qDebug() << "  位深度:" << params.depth;
            
            // 关闭设备
            sane->closeDevice(handle);
            qDebug() << "设备已关闭";
        } else {
            qDebug() << "设备打开失败";
        }
    }
    
    void demonstrateBasicFeatures()
    {
        qDebug() << "\n--- 演示基本功能 ---";
        
        // 测试状态转换
        qDebug() << "状态码测试:";
        qDebug() << "  Good:" << DScannerSANE::statusToString(SANEStatus::Good);
        qDebug() << "  DeviceBusy:" << DScannerSANE::statusToString(SANEStatus::DeviceBusy);
        qDebug() << "  IOError:" << DScannerSANE::statusToString(SANEStatus::IOError);
        
        // 测试设备信息转换
        DeviceInfo deviceInfo;
        deviceInfo.deviceId = "test:scanner:001";
        deviceInfo.manufacturer = "Test Manufacturer";
        deviceInfo.model = "Test Scanner Model";
        
        SANEDevice saneDevice = DScannerSANE::deviceInfoToSANE(deviceInfo);
        qDebug() << "\n设备信息转换测试:";
        qDebug() << "  原始设备ID:" << deviceInfo.deviceId;
        qDebug() << "  SANE设备名:" << saneDevice.name;
        qDebug() << "  SANE厂商:" << saneDevice.vendor;
        qDebug() << "  SANE型号:" << saneDevice.model;
        
        // 反向转换
        DeviceInfo convertedBack = DScannerSANE::saneToDeviceInfo(saneDevice);
        qDebug() << "  转换回的设备ID:" << convertedBack.deviceId;
        qDebug() << "  转换回的名称:" << convertedBack.name;
        
        // 完成演示
        QTimer::singleShot(1000, this, &SANEExample::finish);
    }
    
    void finish()
    {
        qDebug() << "\n--- 清理资源 ---";
        
        if (sane->isInitialized()) {
            sane->exit();
            qDebug() << "SANE子系统已关闭";
        }
        
        qDebug() << "\n=== 示例完成 ===";
        QApplication::quit();
    }
    
    void onDeviceDiscovered(const SANEDevice &device)
    {
        qDebug() << "发现新设备:" << device.name;
    }
    
    void onErrorOccurred(SANEStatus status, const QString &message)
    {
        qDebug() << "SANE错误:" << DScannerSANE::statusToString(status) << "-" << message;
    }

private:
    DScannerManager *manager;
    DScannerSANE *sane;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("DeepinScan SANE Example");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DeepinScan Team");
    
    qDebug() << "启动DeepinScan SANE驱动示例...";
    qDebug() << "Qt版本:" << qVersion();
    qDebug() << "DeepinScan版本:" << DScannerManager::version();
    qDebug() << "";
    
    // 创建示例实例
    SANEExample example;
    
    // 延迟启动以确保事件循环已经开始
    QTimer::singleShot(100, &example, &SANEExample::run);
    
    return app.exec();
}

#include "sane_driver_example.moc" 