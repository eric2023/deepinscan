// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QDebug>
#include <QTimer>

#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerGenesys.h"
#include "Scanner/DScannerUSB.h"

using namespace Dtk::Scanner;

class GenesysExample : public QObject
{
    Q_OBJECT

public:
    GenesysExample(QObject *parent = nullptr) : QObject(parent)
    {
        // 创建USB通信接口
        usbComm = new DScannerUSB(this);
        
        // 创建Genesys驱动
        genesysDriver = new DScannerGenesysDriver(this);
        
        // 连接信号
        connect(usbComm, &DScannerUSB::deviceConnected,
                this, &GenesysExample::onUSBDeviceConnected);
        connect(usbComm, &DScannerUSB::deviceDisconnected,
                this, &GenesysExample::onUSBDeviceDisconnected);
        connect(usbComm, &DScannerUSB::errorOccurred,
                this, &GenesysExample::onUSBError);
                
        connect(genesysDriver, &DScannerGenesysDriver::chipsetStatusChanged,
                this, &GenesysExample::onChipsetStatusChanged);
        connect(genesysDriver, &DScannerGenesysDriver::calibrationProgress,
                this, &GenesysExample::onCalibrationProgress);
    }

public slots:
    void run()
    {
        qDebug() << "=== DeepinScan Genesys驱动示例 ===";
        
        // 初始化USB子系统
        if (usbComm->initialize()) {
            qDebug() << "USB子系统初始化成功";
            
            // 初始化Genesys驱动
            if (genesysDriver->initialize()) {
                qDebug() << "Genesys驱动初始化成功";
                
                // 发现设备
                discoverDevices();
            } else {
                qDebug() << "Genesys驱动初始化失败:" << genesysDriver->lastError();
                QApplication::quit();
            }
        } else {
            qDebug() << "USB子系统初始化失败:" << usbComm->lastError();
            QApplication::quit();
        }
    }

private slots:
    void discoverDevices()
    {
        qDebug() << "\n--- 发现Genesys设备 ---";
        
        // 发现USB设备
        QList<USBDeviceDescriptor> usbDevices = usbComm->discoverDevices();
        qDebug() << "发现" << usbDevices.size() << "个USB设备";
        
        for (const auto &usbDevice : usbDevices) {
            if (DScannerUSB::isScannerDevice(usbDevice)) {
                qDebug() << "扫描仪设备:";
                qDebug() << "  厂商ID:" << QString("0x%1").arg(usbDevice.vendorId, 4, 16, QLatin1Char('0'));
                qDebug() << "  产品ID:" << QString("0x%1").arg(usbDevice.productId, 4, 16, QLatin1Char('0'));
                qDebug() << "  厂商:" << usbDevice.manufacturer;
                qDebug() << "  产品:" << usbDevice.product;
                qDebug() << "  序列号:" << usbDevice.serialNumber;
                qDebug() << "  设备路径:" << usbDevice.devicePath;
                
                // 检查是否为Genesys设备
                GenesysChipset chipset = DScannerGenesysDriver::detectChipset(usbDevice.vendorId, usbDevice.productId);
                if (chipset != GenesysChipset::Unknown) {
                    qDebug() << "  Genesys芯片组:" << DScannerGenesysDriver::chipsetName(chipset);
                    
                    // 获取设备模型信息
                    GenesysModel model = DScannerGenesysDriver::getDeviceModel(usbDevice.vendorId, usbDevice.productId);
                    if (!model.name.isEmpty()) {
                        qDebug() << "  设备型号:" << model.name;
                        qDebug() << "  最大分辨率:" << model.maxResolution << "DPI";
                        qDebug() << "  传感器类型:" << (model.sensorType == SensorType::CCD ? "CCD" : 
                                                    model.sensorType == SensorType::CIS ? "CIS" : "Unknown");
                        qDebug() << "  是否有灯管:" << (model.hasLamp ? "是" : "否");
                        qDebug() << "  是否有ADF:" << (model.hasADF ? "是" : "否");
                        
                        // 测试设备
                        testGenesysDevice(model);
                        return;
                    }
                }
                qDebug() << "";
            }
        }
        
        // 如果没有找到真实设备，演示基本功能
        demonstrateBasicFeatures();
    }
    
    void testGenesysDevice(const GenesysModel &model)
    {
        qDebug() << "\n--- 测试Genesys设备:" << model.name << "---";
        
        // 发现Genesys设备
        QList<DeviceInfo> devices = genesysDriver->discoverDevices();
        
        if (!devices.isEmpty()) {
            DeviceInfo device = devices.first();
            qDebug() << "发现Genesys设备:" << device.name;
            
            // 尝试打开设备
            if (genesysDriver->openDevice(device.deviceId)) {
                qDebug() << "设备打开成功";
                
                // 获取设备能力
                ScannerCapabilities caps = genesysDriver->getCapabilities();
                qDebug() << "设备能力:";
                qDebug() << "  支持分辨率:" << caps.supportedResolutions;
                qDebug() << "  支持颜色模式数:" << caps.supportedColorModes.size();
                qDebug() << "  最大扫描区域:" << caps.maxScanArea.width << "x" << caps.maxScanArea.height << "mm";
                qDebug() << "  是否支持预览:" << (caps.hasPreview ? "是" : "否");
                qDebug() << "  是否支持校准:" << (caps.hasCalibration ? "是" : "否");
                
                // 获取芯片组信息
                GenesysChipset chipset = genesysDriver->getChipset();
                qDebug() << "当前芯片组:" << DScannerGenesysDriver::chipsetName(chipset);
                
                // 演示寄存器操作
                demonstrateRegisterOperations();
                
                // 关闭设备
                genesysDriver->closeDevice();
                qDebug() << "设备已关闭";
            } else {
                qDebug() << "设备打开失败:" << genesysDriver->lastError();
            }
        } else {
            qDebug() << "未发现Genesys设备";
        }
        
        // 完成测试
        QTimer::singleShot(1000, this, &GenesysExample::finish);
    }
    
    void demonstrateRegisterOperations()
    {
        qDebug() << "\n--- 演示寄存器操作 ---";
        
        // 读取芯片ID寄存器
        quint8 chipId = genesysDriver->readRegister(GenesysRegisters::REG_0x01);
        qDebug() << "芯片ID寄存器 (0x01):" << QString("0x%1").arg(chipId, 2, 16, QLatin1Char('0'));
        
        // 读取芯片版本寄存器
        quint8 chipVersion = genesysDriver->readRegister(GenesysRegisters::REG_0x02);
        qDebug() << "芯片版本寄存器 (0x02):" << QString("0x%1").arg(chipVersion, 2, 16, QLatin1Char('0'));
        
        // 读取状态寄存器
        quint8 status = genesysDriver->readRegister(GenesysRegisters::REG_0x04);
        qDebug() << "状态寄存器 (0x04):" << QString("0x%1").arg(status, 2, 16, QLatin1Char('0'));
        
        // 批量读取寄存器
        QByteArray registers = genesysDriver->readRegisters(0x01, 5);
        qDebug() << "批量读取寄存器 (0x01-0x05):";
        for (int i = 0; i < registers.size(); ++i) {
            quint8 value = static_cast<quint8>(registers[i]);
            qDebug() << "  0x" << QString("%1").arg(i + 1, 2, 16, QLatin1Char('0')) 
                     << ":" << QString("0x%1").arg(value, 2, 16, QLatin1Char('0'));
        }
    }
    
    void demonstrateBasicFeatures()
    {
        qDebug() << "\n--- 演示基本功能 ---";
        
        // 显示支持的芯片组
        QList<GenesysChipset> chipsets = DScannerGenesysDriver::supportedChipsets();
        qDebug() << "支持的Genesys芯片组:";
        for (GenesysChipset chipset : chipsets) {
            qDebug() << "  " << DScannerGenesysDriver::chipsetName(chipset);
        }
        
        // 显示驱动信息
        qDebug() << "\n驱动信息:";
        qDebug() << "  驱动名称:" << genesysDriver->driverName();
        qDebug() << "  驱动版本:" << genesysDriver->driverVersion();
        qDebug() << "  驱动类型:" << (genesysDriver->driverType() == DriverType::Genesys ? "Genesys" : "Other");
        
        // 显示支持的设备
        QStringList supportedDevices = genesysDriver->supportedDevices();
        qDebug() << "  支持设备数:" << supportedDevices.size();
        
        // 完成演示
        QTimer::singleShot(1000, this, &GenesysExample::finish);
    }
    
    void finish()
    {
        qDebug() << "\n--- 清理资源 ---";
        
        if (genesysDriver->isDeviceOpen()) {
            genesysDriver->closeDevice();
        }
        
        genesysDriver->shutdown();
        qDebug() << "Genesys驱动已关闭";
        
        usbComm->shutdown();
        qDebug() << "USB子系统已关闭";
        
        qDebug() << "\n=== 示例完成 ===";
        QApplication::quit();
    }
    
    void onUSBDeviceConnected(const USBDeviceDescriptor &descriptor)
    {
        qDebug() << "USB设备连接:" << descriptor.devicePath;
    }
    
    void onUSBDeviceDisconnected(const QString &devicePath)
    {
        qDebug() << "USB设备断开:" << devicePath;
    }
    
    void onUSBError(int errorCode, const QString &errorMessage)
    {
        qDebug() << "USB错误 (" << errorCode << "):" << errorMessage;
    }
    
    void onChipsetStatusChanged(const QString &status)
    {
        qDebug() << "芯片组状态:" << status;
    }
    
    void onCalibrationProgress(int progress)
    {
        qDebug() << "校准进度:" << progress << "%";
    }

private:
    DScannerUSB *usbComm;
    DScannerGenesysDriver *genesysDriver;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("DeepinScan Genesys Example");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DeepinScan Team");
    
    qDebug() << "启动DeepinScan Genesys驱动示例...";
    qDebug() << "Qt版本:" << qVersion();
    qDebug() << "";
    
    // 创建示例实例
    GenesysExample example;
    
    // 延迟启动以确保事件循环已经开始
    QTimer::singleShot(100, &example, &GenesysExample::run);
    
    return app.exec();
}

#include "genesys_driver_example.moc" 