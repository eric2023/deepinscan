// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QSignalSpy>
#include <QImage>
#include <QColor>

#include "Scanner/DScannerImageProcessor.h"

DSCANNER_BEGIN_NAMESPACE

class TestImageProcessingSimple : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBasicImageProcessing();
    void testBrightnessAdjustment();
    void testContrastAdjustment();
    void testGammaCorrection();
    void testFormatConversion();
    void testBatchProcessing();
    void cleanupTestCase();

private:
    DScannerImageProcessor *m_processor;
};

void TestImageProcessingSimple::initTestCase()
{
    m_processor = new DScannerImageProcessor(this);
    QVERIFY(m_processor != nullptr);
}

void TestImageProcessingSimple::testBasicImageProcessing()
{
    // 创建测试图像
    QImage testImage(100, 100, QImage::Format_RGB32);
    testImage.fill(Qt::red);
    
    // 测试基本处理
    QList<ImageProcessingParameters> params;
    QImage result = m_processor->processImage(testImage, params);
    
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), testImage.size());
}

void TestImageProcessingSimple::testBrightnessAdjustment()
{
    // 创建测试图像
    QImage testImage(50, 50, QImage::Format_RGB32);
    testImage.fill(QColor(100, 100, 100));
    
    // 测试亮度调整
    QImage result = m_processor->adjustBrightness(testImage, 50);
    
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), testImage.size());
    
    // 检查亮度是否增加
    QColor centerColor = result.pixelColor(25, 25);
    QVERIFY(centerColor.red() > 100);
    QVERIFY(centerColor.green() > 100);
    QVERIFY(centerColor.blue() > 100);
}

void TestImageProcessingSimple::testContrastAdjustment()
{
    // 创建测试图像
    QImage testImage(50, 50, QImage::Format_RGB32);
    testImage.fill(QColor(100, 100, 100));
    
    // 测试对比度调整
    QImage result = m_processor->adjustContrast(testImage, 20);
    
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), testImage.size());
}

void TestImageProcessingSimple::testGammaCorrection()
{
    // 创建测试图像
    QImage testImage(50, 50, QImage::Format_RGB32);
    testImage.fill(QColor(100, 100, 100));
    
    // 测试伽马校正
    QImage result = m_processor->adjustGamma(testImage, 1.5);
    
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), testImage.size());
}

void TestImageProcessingSimple::testFormatConversion()
{
    // 创建测试图像
    QImage testImage(50, 50, QImage::Format_RGB32);
    testImage.fill(Qt::blue);
    
    // 测试格式转换
    QByteArray pngData = m_processor->convertToFormat(testImage, ImageFormat::PNG);
    QByteArray jpegData = m_processor->convertToFormat(testImage, ImageFormat::JPEG);
    
    QVERIFY(!pngData.isEmpty());
    QVERIFY(!jpegData.isEmpty());
    QVERIFY(pngData.size() > 0);
    QVERIFY(jpegData.size() > 0);
}

void TestImageProcessingSimple::testBatchProcessing()
{
    // 创建测试图像列表
    QList<QImage> images;
    for (int i = 0; i < 3; ++i) {
        QImage img(30, 30, QImage::Format_RGB32);
        img.fill(QColor(i * 50, i * 50, i * 50));
        images.append(img);
    }
    
    // 测试批量处理
    QList<ImageProcessingParameters> params;
    QList<ImageProcessingResult> results = m_processor->processBatch(images, params);
    
    QCOMPARE(results.size(), images.size());
    for (const auto &result : results) {
        QVERIFY(result.success);
        QVERIFY(!result.processedImage.isNull());
    }
}

void TestImageProcessingSimple::cleanupTestCase()
{
    delete m_processor;
    m_processor = nullptr;
}

DSCANNER_END_NAMESPACE

QTEST_MAIN(Dtk::Scanner::TestImageProcessingSimple)
#include "test_image_processing_simple.moc" 