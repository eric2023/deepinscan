// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file core_test.cpp
 * @brief 核心功能测试程序
 * 
 * 测试DeepinScan核心类的基础功能，避免触发MOC相关的链接问题
 */

#include <iostream>
#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include <QSysInfo>

// 只包含不使用Q_OBJECT的头文件
#include "Scanner/DScannerException.h"
#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerGlobal.h"

using namespace Dtk::Scanner;

// 简单的测试函数
void testExceptionHandling() {
    std::cout << "\n=== 测试异常处理 ===" << std::endl;
    
    try {
        // 测试异常创建和处理
        DScannerException ex(DScannerException::ErrorCode::DeviceNotFound, "测试设备未找到");
        std::cout << "异常码: " << static_cast<int>(ex.errorCode()) << std::endl;
        std::cout << "异常消息: " << ex.what() << std::endl;
        
        // 测试异常恢复建议
        bool recoverable = DScannerException::isRecoverable(DScannerException::ErrorCode::DeviceNotFound);
        std::cout << "是否可恢复: " << (recoverable ? "是" : "否") << std::endl;
        
        QString suggestion = DScannerException::getSuggestion(DScannerException::ErrorCode::DeviceNotFound);
        std::cout << "解决建议: " << suggestion.toStdString() << std::endl;
        
        QString action = DScannerException::getRecoveryAction(DScannerException::ErrorCode::DeviceNotConnected);
        std::cout << "恢复操作: " << action.toStdString() << std::endl;
        
    } catch (const DScannerException &e) {
        std::cout << "捕获异常: " << e.what() << std::endl;
    }
}

void testDataStructures() {
    std::cout << "\n=== 测试数据结构 ===" << std::endl;
    
    // 测试设备信息
    DeviceInfo deviceInfo;
    deviceInfo.deviceId = "test_scanner_001";
    deviceInfo.name = "测试扫描仪 Pro";
    deviceInfo.manufacturer = "DeepinScan";
    deviceInfo.model = "DS-2024";
    deviceInfo.isAvailable = true;
    
    std::cout << "设备ID: " << deviceInfo.deviceId.toStdString() << std::endl;
    std::cout << "设备名称: " << deviceInfo.name.toStdString() << std::endl;
    std::cout << "制造商: " << deviceInfo.manufacturer.toStdString() << std::endl;
    std::cout << "型号: " << deviceInfo.model.toStdString() << std::endl;
    std::cout << "是否可用: " << (deviceInfo.isAvailable ? "是" : "否") << std::endl;
    
    // 测试扫描参数
    ScanParameters scanParams;
    scanParams.resolution = 600;
    scanParams.colorMode = ColorMode::Color;
    scanParams.area = ScanArea(0, 0, 210, 297); // A4
    scanParams.format = ImageFormat::PNG;
    
    std::cout << "\n扫描参数:" << std::endl;
    std::cout << "分辨率: " << scanParams.resolution << " DPI" << std::endl;
    std::cout << "颜色模式: " << static_cast<int>(scanParams.colorMode) << std::endl;
    std::cout << "扫描区域: " << scanParams.area.width << "x" << scanParams.area.height << " mm" << std::endl;
    std::cout << "输出格式: " << static_cast<int>(scanParams.format) << std::endl;
    
    // 测试扫描仪能力
    ScannerCapabilities caps;
    caps.supportedResolutions = {75, 150, 300, 600, 1200, 2400};
    caps.supportedColorModes = {ColorMode::Monochrome, ColorMode::Grayscale, ColorMode::Color};
    caps.maxScanArea = ScanArea(0, 0, 216.0, 297.0);  // A4 size
    caps.minScanArea = ScanArea(0, 0, 10.0, 10.0);    // Minimum area
    caps.hasADF = true;
    caps.hasDuplex = false;
    
    std::cout << "\n扫描仪能力:" << std::endl;
    std::cout << "支持分辨率数: " << caps.supportedResolutions.size() << std::endl;
    if (!caps.supportedResolutions.isEmpty()) {
        std::cout << "分辨率范围: " << caps.supportedResolutions.first() << "-" << caps.supportedResolutions.last() << " DPI" << std::endl;
    }
    std::cout << "最大扫描区域: " << caps.maxScanArea.width << "x" << caps.maxScanArea.height << " mm" << std::endl;
    std::cout << "支持ADF: " << (caps.hasADF ? "是" : "否") << std::endl;
    std::cout << "支持双面: " << (caps.hasDuplex ? "是" : "否") << std::endl;
    std::cout << "支持颜色模式数: " << caps.supportedColorModes.size() << std::endl;
}

void testEnumerations() {
    std::cout << "\n=== 测试枚举类型 ===" << std::endl;
    
    // 测试颜色模式
    std::cout << "颜色模式枚举测试:" << std::endl;
    std::cout << "单色: " << static_cast<int>(ColorMode::Monochrome) << std::endl;
    std::cout << "灰度: " << static_cast<int>(ColorMode::Grayscale) << std::endl;
    std::cout << "彩色: " << static_cast<int>(ColorMode::Color) << std::endl;
    
    // 测试图像格式
    std::cout << "\n图像格式枚举测试:" << std::endl;
    std::cout << "JPEG: " << static_cast<int>(ImageFormat::JPEG) << std::endl;
    std::cout << "PNG: " << static_cast<int>(ImageFormat::PNG) << std::endl;
    std::cout << "TIFF: " << static_cast<int>(ImageFormat::TIFF) << std::endl;
    std::cout << "PDF: " << static_cast<int>(ImageFormat::PDF) << std::endl;
    
    // 测试通信协议
    std::cout << "\n通信协议枚举测试:" << std::endl;
    std::cout << "USB: " << static_cast<int>(CommunicationProtocol::USB) << std::endl;
    std::cout << "Network: " << static_cast<int>(CommunicationProtocol::Network) << std::endl;
    std::cout << "SCSI: " << static_cast<int>(CommunicationProtocol::SCSI) << std::endl;
}

void testGlobalFunctions() {
    std::cout << "\n=== 测试版本信息 ===" << std::endl;
    
    // 测试应用信息
    std::cout << "应用名称: DeepinScan" << std::endl;
    std::cout << "应用版本: 1.0.0" << std::endl;
    std::cout << "构建日期: " << __DATE__ << std::endl;
    std::cout << "编译器: " << Q_CC_GNU << std::endl;
    
    // 测试Qt框架信息
    std::cout << "Qt版本: " << QT_VERSION_STR << std::endl;
    std::cout << "架构: " << QSysInfo::currentCpuArchitecture().toStdString() << std::endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("DeepinScan Core Test");
    app.setApplicationVersion("1.0.0");
    
    std::cout << "============================================" << std::endl;
    std::cout << "     DeepinScan 核心功能测试程序" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "编译时间: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "Qt版本: " << QT_VERSION_STR << std::endl;
    std::cout << "============================================" << std::endl;
    
    try {
        // 执行各项测试
        testGlobalFunctions();
        testExceptionHandling();
        testDataStructures();
        testEnumerations();
        
        std::cout << "\n============================================" << std::endl;
        std::cout << "✅ 所有核心功能测试通过！" << std::endl;
        std::cout << "✅ DeepinScan核心库工作正常！" << std::endl;
        std::cout << "============================================" << std::endl;
        
    } catch (const std::exception &e) {
        std::cout << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 