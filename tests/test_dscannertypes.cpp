// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QDebug>
#include <QVariant>
#include <QSize>
#include <QRect>

#include "Scanner/DScannerTypes.h"

DSCANNER_USE_NAMESPACE

/**
 * @brief DScannerTypes单元测试类
 * 
 * 测试扫描仪类型定义和转换功能，确保类型系统的正确性
 */
class TestDScannerTypes : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 枚举类型测试
    void testScanFormat();
    void testColorMode();
    void testScannerType();
    void testScannerStatus();
    void testDeviceCapability();
    
    // 结构体测试
    void testScanParameters();
    void testScannerCapabilities();
    void testScannerDevice();
    void testScanResult();
    
    // 转换功能测试
    void testFormatConversion();
    void testColorModeConversion();
    void testParameterSerialization();
    void testCapabilitiesSerialization();
    
    // 边界值测试
    void testInvalidValues();
    void testBoundaryValues();
    
    // 性能测试
    void testPerformance();
};

void TestDScannerTypes::initTestCase()
{
    qDebug() << "TestDScannerTypes: 开始测试DScannerTypes模块";
}

void TestDScannerTypes::cleanupTestCase()
{
    qDebug() << "TestDScannerTypes: DScannerTypes模块测试完成";
}

void TestDScannerTypes::testScanFormat()
{
    qDebug() << "TestDScannerTypes::testScanFormat: 测试扫描格式枚举";
    
    // 测试所有格式枚举值
    QVERIFY(static_cast<int>(ScanFormat::Auto) == 0);
    QVERIFY(static_cast<int>(ScanFormat::JPEG) == 1);
    QVERIFY(static_cast<int>(ScanFormat::PNG) == 2);
    QVERIFY(static_cast<int>(ScanFormat::TIFF) == 3);
    QVERIFY(static_cast<int>(ScanFormat::PDF) == 4);
    QVERIFY(static_cast<int>(ScanFormat::BMP) == 5);
    
    // 测试格式到字符串转换
    QCOMPARE(scanFormatToString(ScanFormat::JPEG), QString("JPEG"));
    QCOMPARE(scanFormatToString(ScanFormat::PNG), QString("PNG"));
    QCOMPARE(scanFormatToString(ScanFormat::TIFF), QString("TIFF"));
    QCOMPARE(scanFormatToString(ScanFormat::PDF), QString("PDF"));
    QCOMPARE(scanFormatToString(ScanFormat::BMP), QString("BMP"));
    
    // 测试字符串到格式转换
    QVERIFY(stringToScanFormat("JPEG") == ScanFormat::JPEG);
    QVERIFY(stringToScanFormat("PNG") == ScanFormat::PNG);
    QVERIFY(stringToScanFormat("TIFF") == ScanFormat::TIFF);
    QVERIFY(stringToScanFormat("PDF") == ScanFormat::PDF);
    QVERIFY(stringToScanFormat("BMP") == ScanFormat::BMP);
    
    // 测试大小写不敏感
    QVERIFY(stringToScanFormat("jpeg") == ScanFormat::JPEG);
    QVERIFY(stringToScanFormat("Png") == ScanFormat::PNG);
    
    // 测试无效字符串
    QVERIFY(stringToScanFormat("INVALID") == ScanFormat::Auto);
    QVERIFY(stringToScanFormat("") == ScanFormat::Auto);
}

void TestDScannerTypes::testColorMode()
{
    qDebug() << "TestDScannerTypes::testColorMode: 测试颜色模式枚举";
    
    // 测试所有颜色模式枚举值
    QVERIFY(static_cast<int>(ColorMode::Auto) == 0);
    QVERIFY(static_cast<int>(ColorMode::Monochrome) == 1);
    QVERIFY(static_cast<int>(ColorMode::Grayscale) == 2);
    QVERIFY(static_cast<int>(ColorMode::Color) == 3);
    
    // 测试颜色模式到字符串转换
    QCOMPARE(colorModeToString(ColorMode::Auto), QString("Auto"));
    QCOMPARE(colorModeToString(ColorMode::Monochrome), QString("Monochrome"));
    QCOMPARE(colorModeToString(ColorMode::Grayscale), QString("Grayscale"));
    QCOMPARE(colorModeToString(ColorMode::Color), QString("Color"));
    
    // 测试字符串到颜色模式转换
    QVERIFY(stringToColorMode("Auto") == ColorMode::Auto);
    QVERIFY(stringToColorMode("Monochrome") == ColorMode::Monochrome);
    QVERIFY(stringToColorMode("Grayscale") == ColorMode::Grayscale);
    QVERIFY(stringToColorMode("Color") == ColorMode::Color);
    
    // 测试颜色深度获取
    QCOMPARE(getColorDepth(ColorMode::Monochrome), 1);
    QCOMPARE(getColorDepth(ColorMode::Grayscale), 8);
    QCOMPARE(getColorDepth(ColorMode::Color), 24);
}

void TestDScannerTypes::testScannerType()
{
    qDebug() << "TestDScannerTypes::testScannerType: 测试扫描仪类型枚举";
    
    // 测试扫描仪类型
    QVERIFY(static_cast<int>(ScannerType::Unknown) == 0);
    QVERIFY(static_cast<int>(ScannerType::Flatbed) == 1);
    QVERIFY(static_cast<int>(ScannerType::DocumentFeeder) == 2);
    QVERIFY(static_cast<int>(ScannerType::Handheld) == 3);
    QVERIFY(static_cast<int>(ScannerType::FilmScanner) == 4);
    
    // 测试类型描述
    QCOMPARE(scannerTypeToString(ScannerType::Flatbed), QString("Flatbed"));
    QCOMPARE(scannerTypeToString(ScannerType::DocumentFeeder), QString("Document Feeder"));
    QCOMPARE(scannerTypeToString(ScannerType::Handheld), QString("Handheld"));
    QCOMPARE(scannerTypeToString(ScannerType::FilmScanner), QString("Film Scanner"));
}

void TestDScannerTypes::testScannerStatus()
{
    qDebug() << "TestDScannerTypes::testScannerStatus: 测试扫描仪状态枚举";
    
    // 测试状态枚举
    QVERIFY(static_cast<int>(ScannerStatus::Unknown) == 0);
    QVERIFY(static_cast<int>(ScannerStatus::Ready) == 1);
    QVERIFY(static_cast<int>(ScannerStatus::Busy) == 2);
    QVERIFY(static_cast<int>(ScannerStatus::Error) == 3);
    QVERIFY(static_cast<int>(ScannerStatus::Offline) == 4);
    
    // 测试状态检查
    QVERIFY(isStatusReady(ScannerStatus::Ready));
    QVERIFY(!isStatusReady(ScannerStatus::Busy));
    QVERIFY(!isStatusReady(ScannerStatus::Error));
    QVERIFY(!isStatusReady(ScannerStatus::Offline));
    
    QVERIFY(isStatusError(ScannerStatus::Error));
    QVERIFY(!isStatusError(ScannerStatus::Ready));
    QVERIFY(!isStatusError(ScannerStatus::Busy));
}

void TestDScannerTypes::testDeviceCapability()
{
    qDebug() << "TestDScannerTypes::testDeviceCapability: 测试设备能力枚举";
    
    // 测试设备能力标志
    DeviceCapabilities caps = DeviceCapability::None;
    QVERIFY(caps == DeviceCapability::None);
    
    caps |= DeviceCapability::Scan;
    QVERIFY(caps & DeviceCapability::Scan);
    
    caps |= DeviceCapability::Preview;
    QVERIFY(caps & DeviceCapability::Preview);
    QVERIFY(caps & DeviceCapability::Scan);
    
    caps |= DeviceCapability::Calibration;
    QVERIFY(caps & DeviceCapability::Calibration);
    
    // 测试能力检查函数
    QVERIFY(hasCapability(caps, DeviceCapability::Scan));
    QVERIFY(hasCapability(caps, DeviceCapability::Preview));
    QVERIFY(hasCapability(caps, DeviceCapability::Calibration));
    QVERIFY(!hasCapability(caps, DeviceCapability::BatchScan));
}

void TestDScannerTypes::testScanParameters()
{
    qDebug() << "TestDScannerTypes::testScanParameters: 测试扫描参数结构";
    
    // 测试默认构造
    ScanParameters params;
    QVERIFY(params.resolution > 0);
    QVERIFY(params.format == ScanFormat::Auto);
    QVERIFY(params.colorMode == ColorMode::Auto);
    QVERIFY(params.quality >= 0 && params.quality <= 100);
    
    // 测试参数设置
    params.resolution = 300;
    params.format = ScanFormat::JPEG;
    params.colorMode = ColorMode::Color;
    params.quality = 90;
    params.scanArea = QRect(0, 0, 210, 297); // A4尺寸
    params.brightness = 50;
    params.contrast = 50;
    params.gamma = 1.0;
    
    QCOMPARE(params.resolution, 300);
    QVERIFY(params.format == ScanFormat::JPEG);
    QVERIFY(params.colorMode == ColorMode::Color);
    QCOMPARE(params.quality, 90);
    QCOMPARE(params.scanArea.width(), 210);
    QCOMPARE(params.scanArea.height(), 297);
    
    // 测试参数验证
    QVERIFY(isValidScanParameters(params));
    
    // 测试无效参数
    ScanParameters invalidParams = params;
    invalidParams.resolution = 0;
    QVERIFY(!isValidScanParameters(invalidParams));
    
    invalidParams = params;
    invalidParams.quality = 150;
    QVERIFY(!isValidScanParameters(invalidParams));
}

void TestDScannerTypes::testScannerCapabilities()
{
    qDebug() << "TestDScannerTypes::testScannerCapabilities: 测试扫描仪能力结构";
    
    // 创建能力结构
    ScannerCapabilities caps;
    caps.supportedFormats = {ScanFormat::JPEG, ScanFormat::PNG, ScanFormat::PDF};
    caps.supportedColorModes = {ColorMode::Color, ColorMode::Grayscale, ColorMode::Monochrome};
    caps.supportedResolutions = {75, 150, 300, 600, 1200};
    caps.maxScanArea = QSize(216, 356); // Letter size
    caps.minScanArea = QSize(10, 10);
    caps.deviceCapabilities = DeviceCapability::Scan | DeviceCapability::Preview;
    caps.hasADF = false;
    caps.hasDuplex = false;
    caps.maxBitDepth = 24;
    
    // 测试能力检查
    QVERIFY(caps.supportedFormats.contains(ScanFormat::JPEG));
    QVERIFY(caps.supportedFormats.contains(ScanFormat::PNG));
    QVERIFY(caps.supportedFormats.contains(ScanFormat::PDF));
    QVERIFY(!caps.supportedFormats.contains(ScanFormat::BMP));
    
    QVERIFY(caps.supportedColorModes.contains(ColorMode::Color));
    QVERIFY(caps.supportedResolutions.contains(300));
    QVERIFY(!caps.supportedResolutions.contains(2400));
    
    QVERIFY(hasCapability(caps.deviceCapabilities, DeviceCapability::Scan));
    QVERIFY(hasCapability(caps.deviceCapabilities, DeviceCapability::Preview));
    QVERIFY(!hasCapability(caps.deviceCapabilities, DeviceCapability::Calibration));
    
    // 测试能力验证
    QVERIFY(isValidCapabilities(caps));
}

void TestDScannerTypes::testScannerDevice()
{
    qDebug() << "TestDScannerTypes::testScannerDevice: 测试扫描仪设备信息结构";
    
    // 创建设备信息
    ScannerDeviceInfo device;
    device.deviceId = "test_scanner_001";
    device.deviceName = "Test Scanner Model X";
    device.vendor = "Test Vendor";
    device.model = "Model X";
    device.type = ScannerType::Flatbed;
    device.status = ScannerStatus::Ready;
    device.connectionType = "USB";
    device.driverName = "test_driver";
    device.isNetworkDevice = false;
    device.ipAddress = "";
    device.port = 0;
    
    // 测试设备信息
    QCOMPARE(device.deviceId, QString("test_scanner_001"));
    QCOMPARE(device.deviceName, QString("Test Scanner Model X"));
    QCOMPARE(device.vendor, QString("Test Vendor"));
    QCOMPARE(device.model, QString("Model X"));
    QVERIFY(device.type == ScannerType::Flatbed);
    QVERIFY(device.status == ScannerStatus::Ready);
    QVERIFY(!device.isNetworkDevice);
    
    // 测试设备信息验证
    QVERIFY(isValidDeviceInfo(device));
    
    // 测试无效设备信息
    ScannerDeviceInfo invalidDevice = device;
    invalidDevice.deviceId = "";
    QVERIFY(!isValidDeviceInfo(invalidDevice));
}

void TestDScannerTypes::testScanResult()
{
    qDebug() << "TestDScannerTypes::testScanResult: 测试扫描结果结构";
    
    // 创建扫描结果
    ScanResult result;
    result.success = true;
    result.errorCode = 0;
    result.errorMessage = "";
    result.scanTime = QDateTime::currentDateTime();
    result.imageData = QByteArray("test_image_data");
    result.actualParameters.resolution = 300;
    result.actualParameters.format = ScanFormat::JPEG;
    result.actualParameters.colorMode = ColorMode::Color;
    
    // 测试结果检查
    QVERIFY(result.success);
    QCOMPARE(result.errorCode, 0);
    QVERIFY(result.errorMessage.isEmpty());
    QVERIFY(!result.imageData.isEmpty());
    QVERIFY(result.scanTime.isValid());
    
    // 测试失败结果
    ScanResult failedResult;
    failedResult.success = false;
    failedResult.errorCode = 1001;
    failedResult.errorMessage = "Scan failed: Device not ready";
    
    QVERIFY(!failedResult.success);
    QCOMPARE(failedResult.errorCode, 1001);
    QVERIFY(!failedResult.errorMessage.isEmpty());
}

void TestDScannerTypes::testFormatConversion()
{
    qDebug() << "TestDScannerTypes::testFormatConversion: 测试格式转换功能";
    
    // 测试MIME类型转换
    QCOMPARE(scanFormatToMimeType(ScanFormat::JPEG), QString("image/jpeg"));
    QCOMPARE(scanFormatToMimeType(ScanFormat::PNG), QString("image/png"));
    QCOMPARE(scanFormatToMimeType(ScanFormat::TIFF), QString("image/tiff"));
    QCOMPARE(scanFormatToMimeType(ScanFormat::PDF), QString("application/pdf"));
    QCOMPARE(scanFormatToMimeType(ScanFormat::BMP), QString("image/bmp"));
    
    // 测试文件扩展名转换
    QCOMPARE(scanFormatToExtension(ScanFormat::JPEG), QString("jpg"));
    QCOMPARE(scanFormatToExtension(ScanFormat::PNG), QString("png"));
    QCOMPARE(scanFormatToExtension(ScanFormat::TIFF), QString("tiff"));
    QCOMPARE(scanFormatToExtension(ScanFormat::PDF), QString("pdf"));
    QCOMPARE(scanFormatToExtension(ScanFormat::BMP), QString("bmp"));
    
    // 测试从扩展名获取格式
    QVERIFY(extensionToScanFormat("jpg") == ScanFormat::JPEG);
    QVERIFY(extensionToScanFormat("jpeg") == ScanFormat::JPEG);
    QVERIFY(extensionToScanFormat("png") == ScanFormat::PNG);
    QVERIFY(extensionToScanFormat("tiff") == ScanFormat::TIFF);
    QVERIFY(extensionToScanFormat("tif") == ScanFormat::TIFF);
    QVERIFY(extensionToScanFormat("pdf") == ScanFormat::PDF);
    QVERIFY(extensionToScanFormat("bmp") == ScanFormat::BMP);
}

void TestDScannerTypes::testColorModeConversion()
{
    qDebug() << "TestDScannerTypes::testColorModeConversion: 测试颜色模式转换";
    
    // 测试颜色通道数获取
    QCOMPARE(getColorChannels(ColorMode::Monochrome), 1);
    QCOMPARE(getColorChannels(ColorMode::Grayscale), 1);
    QCOMPARE(getColorChannels(ColorMode::Color), 3);
    
    // 测试字节数计算
    QSize imageSize(100, 100);
    QCOMPARE(calculateImageBytes(imageSize, ColorMode::Monochrome), 100 * 100 / 8); // 1 bit per pixel
    QCOMPARE(calculateImageBytes(imageSize, ColorMode::Grayscale), 100 * 100 * 1); // 8 bits per pixel
    QCOMPARE(calculateImageBytes(imageSize, ColorMode::Color), 100 * 100 * 3); // 24 bits per pixel
}

void TestDScannerTypes::testParameterSerialization()
{
    qDebug() << "TestDScannerTypes::testParameterSerialization: 测试参数序列化";
    
    // 创建参数对象
    ScanParameters original;
    original.resolution = 600;
    original.format = ScanFormat::PNG;
    original.colorMode = ColorMode::Color;
    original.quality = 95;
    original.scanArea = QRect(10, 10, 200, 280);
    original.brightness = 75;
    original.contrast = 60;
    original.gamma = 1.2;
    original.autoCrop = true;
    original.autoDeskew = true;
    
    // 序列化到QVariant
    QVariant serialized = parametersToVariant(original);
    QVERIFY(serialized.isValid());
    
    // 反序列化
    ScanParameters deserialized = variantToParameters(serialized);
    
    // 验证数据一致性
    QCOMPARE(deserialized.resolution, original.resolution);
    QVERIFY(deserialized.format == original.format);
    QVERIFY(deserialized.colorMode == original.colorMode);
    QCOMPARE(deserialized.quality, original.quality);
    QCOMPARE(deserialized.scanArea, original.scanArea);
    QCOMPARE(deserialized.brightness, original.brightness);
    QCOMPARE(deserialized.contrast, original.contrast);
    QCOMPARE(deserialized.gamma, original.gamma);
    QCOMPARE(deserialized.autoCrop, original.autoCrop);
    QCOMPARE(deserialized.autoDeskew, original.autoDeskew);
}

void TestDScannerTypes::testCapabilitiesSerialization()
{
    qDebug() << "TestDScannerTypes::testCapabilitiesSerialization: 测试能力序列化";
    
    // 创建能力对象
    ScannerCapabilities original;
    original.supportedFormats = {ScanFormat::JPEG, ScanFormat::PNG, ScanFormat::TIFF};
    original.supportedColorModes = {ColorMode::Color, ColorMode::Grayscale};
    original.supportedResolutions = {150, 300, 600, 1200};
    original.maxScanArea = QSize(216, 356);
    original.minScanArea = QSize(10, 10);
    original.deviceCapabilities = DeviceCapability::Scan | DeviceCapability::Preview | DeviceCapability::Calibration;
    original.hasADF = true;
    original.hasDuplex = false;
    original.maxBitDepth = 24;
    
    // 序列化
    QVariant serialized = capabilitiesToVariant(original);
    QVERIFY(serialized.isValid());
    
    // 反序列化
    ScannerCapabilities deserialized = variantToCapabilities(serialized);
    
    // 验证数据一致性
    QCOMPARE(deserialized.supportedFormats, original.supportedFormats);
    QCOMPARE(deserialized.supportedColorModes, original.supportedColorModes);
    QCOMPARE(deserialized.supportedResolutions, original.supportedResolutions);
    QCOMPARE(deserialized.maxScanArea, original.maxScanArea);
    QCOMPARE(deserialized.minScanArea, original.minScanArea);
    QCOMPARE(deserialized.deviceCapabilities, original.deviceCapabilities);
    QCOMPARE(deserialized.hasADF, original.hasADF);
    QCOMPARE(deserialized.hasDuplex, original.hasDuplex);
    QCOMPARE(deserialized.maxBitDepth, original.maxBitDepth);
}

void TestDScannerTypes::testInvalidValues()
{
    qDebug() << "TestDScannerTypes::testInvalidValues: 测试无效值处理";
    
    // 测试无效格式转换
    QCOMPARE(scanFormatToString(static_cast<ScanFormat>(999)), QString("Unknown"));
    QCOMPARE(scanFormatToMimeType(static_cast<ScanFormat>(999)), QString("application/octet-stream"));
    
    // 测试无效颜色模式
    QCOMPARE(colorModeToString(static_cast<ColorMode>(999)), QString("Unknown"));
    QCOMPARE(getColorDepth(static_cast<ColorMode>(999)), 0);
    
    // 测试无效参数
    ScanParameters invalidParams;
    invalidParams.resolution = -100;
    invalidParams.quality = 200;
    invalidParams.brightness = -50;
    invalidParams.contrast = 150;
    invalidParams.gamma = -1.0;
    
    QVERIFY(!isValidScanParameters(invalidParams));
}

void TestDScannerTypes::testBoundaryValues()
{
    qDebug() << "TestDScannerTypes::testBoundaryValues: 测试边界值";
    
    // 测试分辨率边界
    QVERIFY(isValidResolution(75));
    QVERIFY(isValidResolution(4800));
    QVERIFY(!isValidResolution(0));
    QVERIFY(!isValidResolution(10000));
    
    // 测试质量边界
    QVERIFY(isValidQuality(0));
    QVERIFY(isValidQuality(100));
    QVERIFY(!isValidQuality(-1));
    QVERIFY(!isValidQuality(101));
    
    // 测试亮度对比度边界
    QVERIFY(isValidBrightness(0));
    QVERIFY(isValidBrightness(100));
    QVERIFY(!isValidBrightness(-1));
    QVERIFY(!isValidBrightness(101));
    
    QVERIFY(isValidContrast(0));
    QVERIFY(isValidContrast(100));
    QVERIFY(!isValidContrast(-1));
    QVERIFY(!isValidContrast(101));
    
    // 测试伽马值边界
    QVERIFY(isValidGamma(0.1));
    QVERIFY(isValidGamma(3.0));
    QVERIFY(!isValidGamma(0.0));
    QVERIFY(!isValidGamma(5.0));
}

void TestDScannerTypes::testPerformance()
{
    qDebug() << "TestDScannerTypes::testPerformance: 测试性能";
    
    const int iterations = 10000;
    
    // 测试格式转换性能
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < iterations; ++i) {
        ScanFormat format = static_cast<ScanFormat>(i % 6);
        QString str = scanFormatToString(format);
        ScanFormat converted = stringToScanFormat(str);
        Q_UNUSED(converted);
    }
    
    qint64 formatConversionTime = timer.elapsed();
    qDebug() << "格式转换性能:" << formatConversionTime << "ms for" << iterations << "iterations";
    
    // 测试参数序列化性能
    ScanParameters params;
    params.resolution = 300;
    params.format = ScanFormat::JPEG;
    params.colorMode = ColorMode::Color;
    
    timer.restart();
    
    for (int i = 0; i < iterations; ++i) {
        QVariant serialized = parametersToVariant(params);
        ScanParameters deserialized = variantToParameters(serialized);
        Q_UNUSED(deserialized);
    }
    
    qint64 serializationTime = timer.elapsed();
    qDebug() << "参数序列化性能:" << serializationTime << "ms for" << iterations << "iterations";
    
    // 性能要求：操作应该在合理时间内完成
    QVERIFY(formatConversionTime < 1000); // 1秒内完成10000次转换
    QVERIFY(serializationTime < 2000); // 2秒内完成10000次序列化
}

QTEST_MAIN(TestDScannerTypes)
#include "test_dscannertypes.moc" 