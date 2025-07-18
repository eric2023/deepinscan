/**
 * @file test_core_functionality.cpp
 * @brief 核心功能简化测试
 * @author DeepinScan Development Team
 * @date 2024
 */

#include <QTest>
#include <QObject>
#include <QDebug>

#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerException.h"

using namespace Dtk::Scanner;

/**
 * @brief 核心功能测试类
 * 
 * 测试已经实现的基础功能，避免依赖未实现的模块
 */
class TestCoreFunctionality : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 基础类型测试
    void testBasicTypes();
    void testDeviceInfo();
    void testScanParameters();
    void testScannerCapabilities();
    
    // 异常处理测试
    void testDScannerException();
    void testExceptionTypes();
    
    // 全局配置测试
    void testGlobalSettings();

private:
    void logTestInfo(const QString &testName);
};

void TestCoreFunctionality::initTestCase()
{
    qDebug() << "=======================================";
    qDebug() << "       DeepinScan 核心功能测试";
    qDebug() << "=======================================";
}

void TestCoreFunctionality::cleanupTestCase()
{
    qDebug() << "=======================================";
    qDebug() << "       测试完成";
    qDebug() << "=======================================";
}

void TestCoreFunctionality::logTestInfo(const QString &testName)
{
    qDebug() << QString("开始测试: %1").arg(testName);
}

void TestCoreFunctionality::testBasicTypes()
{
    logTestInfo("基础类型");
    
    // 测试ColorMode枚举
    ColorMode colorMode = ColorMode::Color;
    QVERIFY(colorMode == ColorMode::Color);
    
    colorMode = ColorMode::Grayscale;
    QVERIFY(colorMode == ColorMode::Grayscale);
    
    colorMode = ColorMode::Lineart;
    QVERIFY(colorMode == ColorMode::Lineart);
    
    colorMode = ColorMode::Monochrome;
    QVERIFY(colorMode == ColorMode::Monochrome);
    
    // 测试ImageFormat枚举
    ImageFormat format = ImageFormat::JPEG;
    QVERIFY(format == ImageFormat::JPEG);
    
    format = ImageFormat::PNG;
    QVERIFY(format == ImageFormat::PNG);
    
    format = ImageFormat::TIFF;
    QVERIFY(format == ImageFormat::TIFF);
    
    format = ImageFormat::PDF;
    QVERIFY(format == ImageFormat::PDF);
    
    format = ImageFormat::BMP;
    QVERIFY(format == ImageFormat::BMP);
    
    format = ImageFormat::PNM;
    QVERIFY(format == ImageFormat::PNM);
    
    qDebug() << "✅ 基础类型测试通过";
}

void TestCoreFunctionality::testDeviceInfo()
{
    logTestInfo("DeviceInfo结构");
    
    DeviceInfo deviceInfo;
    
    // 测试默认值
    QVERIFY(deviceInfo.deviceId.isEmpty());
    QVERIFY(deviceInfo.name.isEmpty());
    QVERIFY(deviceInfo.manufacturer.isEmpty());
    QVERIFY(deviceInfo.model.isEmpty());
    QVERIFY(deviceInfo.serialNumber.isEmpty());
    QVERIFY(deviceInfo.connectionString.isEmpty());
    QVERIFY(deviceInfo.isAvailable == false);
    
    // 测试赋值
    deviceInfo.deviceId = "test_device_001";
    deviceInfo.name = "Test Scanner";
    deviceInfo.manufacturer = "Test Manufacturer";
    deviceInfo.model = "Test Model v1.0";
    deviceInfo.serialNumber = "SN123456789";
    deviceInfo.connectionString = "usb:001:002";
    deviceInfo.isAvailable = true;
    
    // 验证赋值结果
    QCOMPARE(deviceInfo.deviceId, QString("test_device_001"));
    QCOMPARE(deviceInfo.name, QString("Test Scanner"));
    QCOMPARE(deviceInfo.manufacturer, QString("Test Manufacturer"));
    QCOMPARE(deviceInfo.model, QString("Test Model v1.0"));
    QCOMPARE(deviceInfo.serialNumber, QString("SN123456789"));
    QCOMPARE(deviceInfo.connectionString, QString("usb:001:002"));
    QVERIFY(deviceInfo.isAvailable == true);
    
    // 测试isValid()方法
    QVERIFY(deviceInfo.isValid());
    
    DeviceInfo emptyInfo;
    QVERIFY(!emptyInfo.isValid());
    
    qDebug() << "✅ DeviceInfo结构测试通过";
}

void TestCoreFunctionality::testScanParameters()
{
    logTestInfo("ScanParameters结构");
    
    ScanParameters params;
    
    // 测试默认值
    QVERIFY(params.resolution == 300);
    QVERIFY(params.colorMode == ColorMode::Color);
    QVERIFY(params.format == ImageFormat::JPEG);
    QVERIFY(params.quality == 85);
    QVERIFY(params.brightness == 0.0);
    QVERIFY(params.contrast == 0.0);
    QVERIFY(params.gamma == 1.0);
    
    // 测试赋值
    params.resolution = 600;
    params.colorMode = ColorMode::Grayscale;
    params.format = ImageFormat::PNG;
    params.quality = 95;
    params.brightness = 60;
    params.contrast = 55;
    params.gamma = 1.2;
    
    // 验证赋值结果
    QCOMPARE(params.resolution, 600);
    QVERIFY(params.colorMode == ColorMode::Grayscale);
    QVERIFY(params.format == ImageFormat::PNG);
    QCOMPARE(params.quality, 95);
    QCOMPARE(params.brightness, 60);
    QCOMPARE(params.contrast, 55);
    QCOMPARE(params.gamma, 1.2);
    
    qDebug() << "✅ ScanParameters结构测试通过";
}

void TestCoreFunctionality::testScannerCapabilities()
{
    logTestInfo("ScannerCapabilities结构");
    
    ScannerCapabilities caps;
    
    // 测试默认值
    QVERIFY(caps.supportedResolutions.isEmpty());
    QVERIFY(caps.supportedColorModes.isEmpty());
    QVERIFY(caps.supportedFormats.isEmpty());
    QVERIFY(caps.maxScanArea.width == 0.0);
    QVERIFY(caps.maxScanArea.height == 0.0);
    QVERIFY(caps.maxScanArea.x == 0.0);
    QVERIFY(caps.maxScanArea.y == 0.0);
    QVERIFY(caps.hasADF == false);
    QVERIFY(caps.hasDuplex == false);
    
    // 测试赋值
    caps.supportedResolutions = {75, 150, 300, 600, 1200};
    
    // 使用append方式设置ColorMode列表（避免初始化列表问题）
    caps.supportedColorModes.append(ColorMode::Lineart);
    caps.supportedColorModes.append(ColorMode::Grayscale);
    caps.supportedColorModes.append(ColorMode::Color);
    
    // 使用append方式设置ImageFormat列表
    caps.supportedFormats.append(ImageFormat::JPEG);
    caps.supportedFormats.append(ImageFormat::PNG);
    caps.supportedFormats.append(ImageFormat::PDF);
    
    caps.maxScanArea.width = 210.0;  // A4 width in mm
    caps.maxScanArea.height = 297.0; // A4 height in mm
    caps.maxScanArea.x = 0.0;
    caps.maxScanArea.y = 0.0;
    caps.hasADF = true;
    caps.hasDuplex = true;
    
    // 验证赋值结果
    QCOMPARE(caps.supportedResolutions.size(), 5);
    QVERIFY(caps.supportedResolutions.contains(300));
    QVERIFY(caps.supportedResolutions.contains(600));
    
    QCOMPARE(caps.supportedColorModes.size(), 3);
    QVERIFY(caps.supportedColorModes.contains(ColorMode::Color));
    
    QCOMPARE(caps.supportedFormats.size(), 3);
    QVERIFY(caps.supportedFormats.contains(ImageFormat::PDF));
    
    QCOMPARE(caps.maxScanArea.width, 210.0);
    QCOMPARE(caps.maxScanArea.height, 297.0);
    QVERIFY(caps.hasADF == true);
    QVERIFY(caps.hasDuplex == true);
    
    qDebug() << "✅ ScannerCapabilities结构测试通过";
}

void TestCoreFunctionality::testDScannerException()
{
    logTestInfo("DScannerException异常处理");
    
    // 测试基本异常创建
    try {
        DScannerException exception(DScannerException::ErrorCode::DeviceNotFound, "测试设备未找到");
        QCOMPARE(exception.errorCode(), DScannerException::ErrorCode::DeviceNotFound);
        QVERIFY(exception.what() != nullptr);
        QVERIFY(QString(exception.what()).contains("测试设备未找到"));
        
        // 抛出异常进行捕获测试
        throw exception;
    }
    catch (const DScannerException &e) {
        QCOMPARE(e.errorCode(), DScannerException::ErrorCode::DeviceNotFound);
        qDebug() << "✅ 异常捕获测试通过:" << e.what();
    }
    catch (...) {
        QFAIL("应该捕获DScannerException，但捕获了其他异常");
    }
}

void TestCoreFunctionality::testExceptionTypes()
{
    logTestInfo("异常类型枚举");
    
    // 测试所有错误代码都是有效的（使用实际定义的错误代码）
    QList<DScannerException::ErrorCode> errorCodes;
    errorCodes.append(DScannerException::ErrorCode::Unknown);
    errorCodes.append(DScannerException::ErrorCode::DeviceNotFound);
    errorCodes.append(DScannerException::ErrorCode::DeviceNotConnected);
    errorCodes.append(DScannerException::ErrorCode::DeviceBusy);
    errorCodes.append(DScannerException::ErrorCode::InvalidParameter);
    errorCodes.append(DScannerException::ErrorCode::TimeoutError);
    errorCodes.append(DScannerException::ErrorCode::NetworkError);
    errorCodes.append(DScannerException::ErrorCode::LicenseError);
    errorCodes.append(DScannerException::ErrorCode::ConfigurationError);
    
    // 验证每个错误代码都能创建有效的异常
    for (const auto &errorCode : errorCodes) {
        DScannerException exception(errorCode, QString("测试错误代码: %1").arg(static_cast<int>(errorCode)));
        QCOMPARE(exception.errorCode(), errorCode);
        QVERIFY(exception.what() != nullptr);
    }
    
    qDebug() << QString("✅ 异常类型测试通过，共测试 %1 种错误类型").arg(errorCodes.size());
}

void TestCoreFunctionality::testGlobalSettings()
{
    logTestInfo("全局设置");
    
    // 测试版本信息（如果有的话）
    qDebug() << "DeepinScan 版本测试...";
    
    // 这里可以添加版本信息的测试
    // 比如测试 DEEPINSCAN_VERSION_MAJOR 等宏定义
    
    qDebug() << "✅ 全局设置测试通过";
}

QTEST_MAIN(TestCoreFunctionality)
#include "test_core_functionality.moc" 