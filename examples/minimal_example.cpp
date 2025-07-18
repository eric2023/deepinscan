// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file minimal_example.cpp
 * @brief 最小化示例程序
 * 
 * 演示DeepinScan核心库的基础功能，不触发复杂的信号链接
 */

#include <iostream>
#include <QCoreApplication>
#include <QString>
#include <QDebug>

#include "Scanner/DScannerException.h"
#include "Scanner/DScannerTypes.h"

using namespace Dtk::Scanner;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "=== DeepinScan 最小化示例 ===" << std::endl;
    std::cout << "核心库版本: 1.0.0" << std::endl;
    std::cout << "编译时间: " << __DATE__ << " " << __TIME__ << std::endl;
    
    // 测试异常处理
    try {
        std::cout << "\n--- 测试异常处理 ---" << std::endl;
        
        DScannerException testException(DScannerException::ErrorCode::DeviceNotFound, "测试异常");
        std::cout << "异常码: " << static_cast<int>(testException.errorCode()) << std::endl;
        std::cout << "异常消息: " << testException.what() << std::endl;
        
        // 测试默认错误消息
        QString defaultMsg = DScannerException::defaultErrorMessage(DScannerException::ErrorCode::DriverNotFound);
        std::cout << "默认错误消息: " << defaultMsg.toStdString() << std::endl;
        
    } catch (const DScannerException &e) {
        std::cout << "捕获异常: " << e.what() << std::endl;
    }
    
    // 测试设备信息结构
    std::cout << "\n--- 测试设备信息结构 ---" << std::endl;
    DeviceInfo deviceInfo;
    deviceInfo.name = "测试扫描仪";
    deviceInfo.manufacturer = "测试厂商";
    deviceInfo.model = "TestModel";
    deviceInfo.deviceId = "test001";
    
    std::cout << "设备名称: " << deviceInfo.name.toStdString() << std::endl;
    std::cout << "制造商: " << deviceInfo.manufacturer.toStdString() << std::endl;
    std::cout << "型号: " << deviceInfo.model.toStdString() << std::endl;
    std::cout << "设备ID: " << deviceInfo.deviceId.toStdString() << std::endl;
    
    // 测试扫描参数
    std::cout << "\n--- 测试扫描参数 ---" << std::endl;
    ScanParameters scanParams;
    scanParams.resolution = 300;
    scanParams.colorMode = ColorMode::Color;
    scanParams.area = ScanArea(0, 0, 210, 297); // A4 size in mm
    
    std::cout << "分辨率: " << scanParams.resolution << " DPI" << std::endl;
    std::cout << "颜色模式: " << static_cast<int>(scanParams.colorMode) << std::endl;
    std::cout << "扫描区域: " << scanParams.area.width << "x" << scanParams.area.height << " mm" << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    std::cout << "核心库基础功能验证成功！" << std::endl;
    
    return 0;
} 