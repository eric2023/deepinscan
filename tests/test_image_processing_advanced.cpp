// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QSignalSpy>
#include <QImage>
#include <QColor>
#include <QPainter>

#include "Scanner/DScannerImageProcessor.h"

DSCANNER_BEGIN_NAMESPACE

class TestImageProcessingAdvanced : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testAutoLevel();
    void testColorCorrection();
    void testCropDetection();
    void testDeskew();
    void testAdvancedProcessingChain();
    void testPresetManagement();
    void testPerformanceMetrics();
    void cleanupTestCase();

private:
    DScannerImageProcessor *m_processor;
    QImage createTestDocumentImage();
    QImage createTestPhotoImage();
};

void TestImageProcessingAdvanced::initTestCase()
{
    m_processor = new DScannerImageProcessor(this);
    QVERIFY(m_processor != nullptr);
}

QImage TestImageProcessingAdvanced::createTestDocumentImage()
{
    // 创建一个模拟文档图像（带白边和文字）
    QImage image(300, 400, QImage::Format_RGB32);
    image.fill(Qt::white);
    
    QPainter painter(&image);
    painter.fillRect(20, 20, 260, 360, Qt::white);
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 12));
    painter.drawText(40, 60, "Test Document");
    painter.drawText(40, 100, "This is a sample text");
    painter.drawRect(40, 120, 200, 200);
    
    return image;
}

QImage TestImageProcessingAdvanced::createTestPhotoImage()
{
    // 创建一个模拟照片图像（带色彩偏差）
    QImage image(200, 150, QImage::Format_RGB32);
    
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            // 创建带轻微色彩偏差的渐变
            int r = qBound(0, 100 + x / 3, 255);
            int g = qBound(0, 80 + y / 3, 255);
            int b = qBound(0, 120 + (x + y) / 4, 255);
            image.setPixelColor(x, y, QColor(r, g, b));
        }
    }
    
    return image;
}

void TestImageProcessingAdvanced::testAutoLevel()
{
    QImage testImage = createTestPhotoImage();
    
    // 测试自动色阶
    QImage result = m_processor->autoLevel(testImage);
    
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), testImage.size());
    
    // 验证色阶拉伸效果（应该有更大的动态范围）
    QColor originalCenter = testImage.pixelColor(100, 75);
    QColor processedCenter = result.pixelColor(100, 75);
    
    // 处理后的图像应该有变化
    QVERIFY(originalCenter != processedCenter);
    
    qDebug() << "Auto Level Test: Original center color:" << originalCenter
             << "Processed center color:" << processedCenter;
}

void TestImageProcessingAdvanced::testColorCorrection()
{
    QImage testImage = createTestPhotoImage();
    
    // 测试色彩校正（假设白点为灰色，需要增强对比度）
    QColor whitePoint(200, 200, 200);
    QImage result = m_processor->colorCorrection(testImage, whitePoint);
    
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), testImage.size());
    
    // 检查校正效果
    QColor originalPixel = testImage.pixelColor(50, 50);
    QColor correctedPixel = result.pixelColor(50, 50);
    
    // 校正后的颜色值应该更高（更亮）
    QVERIFY(correctedPixel.red() >= originalPixel.red());
    QVERIFY(correctedPixel.green() >= originalPixel.green());
    QVERIFY(correctedPixel.blue() >= originalPixel.blue());
    
    qDebug() << "Color Correction Test: Original:" << originalPixel
             << "Corrected:" << correctedPixel;
}

void TestImageProcessingAdvanced::testCropDetection()
{
    QImage testImage = createTestDocumentImage();
    
    // 测试自动裁剪检测
    QRect cropArea = m_processor->detectCropArea(testImage);
    
    QVERIFY(cropArea.isValid());
    QVERIFY(cropArea.width() > 0);
    QVERIFY(cropArea.height() > 0);
    
    // 裁剪区域应该小于原图（去除了白边）
    QVERIFY(cropArea.width() < testImage.width());
    QVERIFY(cropArea.height() < testImage.height());
    
    // 裁剪区域应该包含内容区域
    QVERIFY(cropArea.left() <= 40);
    QVERIFY(cropArea.top() <= 60);
    QVERIFY(cropArea.right() >= 240);
    QVERIFY(cropArea.bottom() >= 320);
    
    qDebug() << "Crop Detection Test: Detected area:" << cropArea
             << "Original size:" << testImage.size();
}

void TestImageProcessingAdvanced::testDeskew()
{
    QImage testImage = createTestDocumentImage();
    
    // 测试倾斜校正（真实实现）
    QImage result = m_processor->deskew(testImage);
    
    QVERIFY(!result.isNull());
    // 注意：倾斜校正可能会改变图像尺寸，因为要旋转图像
    QVERIFY(result.width() > 0);
    QVERIFY(result.height() > 0);
    
    // 对于没有明显倾斜的测试图像，算法应该检测到角度很小，返回原图或接近原图
    // 允许一定的尺寸变化，因为旋转可能导致边界调整
    QVERIFY(qAbs(result.width() - testImage.width()) < 100);
    QVERIFY(qAbs(result.height() - testImage.height()) < 100);
    
    qDebug() << "Deskew Test: Real implementation verified, size changed from" 
             << testImage.size() << "to" << result.size();
}

void TestImageProcessingAdvanced::testAdvancedProcessingChain()
{
    QImage testImage = createTestDocumentImage();
    
    // 测试复杂的处理链
    QList<ImageProcessingParameters> params;
    
    // 添加多个高级算法
    params.append(ImageProcessingParameters(ImageProcessingAlgorithm::AutoLevel));
    params.append(ImageProcessingParameters(ImageProcessingAlgorithm::ContrastEnhance, 
        QVariantMap{{"contrast", 20}}));
    params.append(ImageProcessingParameters(ImageProcessingAlgorithm::CropDetection));
    params.append(ImageProcessingParameters(ImageProcessingAlgorithm::Deskew));
    
    QImage result = m_processor->processImage(testImage, params);
    
    QVERIFY(!result.isNull());
    // 由于包含裁剪，结果图像可能比原图小
    QVERIFY(result.width() <= testImage.width());
    QVERIFY(result.height() <= testImage.height());
    
    qDebug() << "Advanced Processing Chain Test: Original size:" << testImage.size()
             << "Processed size:" << result.size();
}

void TestImageProcessingAdvanced::testPresetManagement()
{
    // 测试预设管理功能
    QList<ImageProcessingParameters> documentPreset;
    documentPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::AutoLevel));
    documentPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::ContrastEnhance, 
        QVariantMap{{"contrast", 30}}));
    documentPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::CropDetection));
    
    // 添加预设
    m_processor->addPreset("CustomDocument", documentPreset);
    
    // 验证预设添加成功
    QStringList presetNames = m_processor->getPresetNames();
    QVERIFY(presetNames.contains("CustomDocument"));
    
    // 获取预设并验证
    QList<ImageProcessingParameters> retrieved = m_processor->getPreset("CustomDocument");
    QCOMPARE(retrieved.size(), documentPreset.size());
    QCOMPARE(retrieved[0].algorithm, ImageProcessingAlgorithm::AutoLevel);
    QCOMPARE(retrieved[1].algorithm, ImageProcessingAlgorithm::ContrastEnhance);
    
    // 移除预设
    m_processor->removePreset("CustomDocument");
    presetNames = m_processor->getPresetNames();
    QVERIFY(!presetNames.contains("CustomDocument"));
    
    qDebug() << "Preset Management Test: Real implementation verified";
}

void TestImageProcessingAdvanced::testPerformanceMetrics()
{
    QImage testImage = createTestPhotoImage();
    
    // 记录处理前的统计
    qint64 initialProcessed = m_processor->totalProcessedImages();
    qint64 initialTime = m_processor->totalProcessingTime();
    
    // 执行一些处理
    QList<ImageProcessingParameters> params;
    params.append(ImageProcessingParameters(ImageProcessingAlgorithm::AutoLevel));
    params.append(ImageProcessingParameters(ImageProcessingAlgorithm::BrightnessAdjust, 
        QVariantMap{{"brightness", 10}}));
    
    QImage result = m_processor->processImage(testImage, params);
    QVERIFY(!result.isNull());
    
    // 验证统计信息更新
    qint64 finalProcessed = m_processor->totalProcessedImages();
    qint64 finalTime = m_processor->totalProcessingTime();
    
    QVERIFY(finalProcessed > initialProcessed);
    QVERIFY(finalTime >= initialTime); // 可能为0如果处理很快
    
    // 测试平均处理时间
    double avgTime = m_processor->averageProcessingTime();
    QVERIFY(avgTime >= 0.0);
    
    qDebug() << "Performance Metrics Test:"
             << "Processed images:" << finalProcessed
             << "Total time:" << finalTime << "ms"
             << "Average time:" << avgTime << "ms";
}

void TestImageProcessingAdvanced::cleanupTestCase()
{
    delete m_processor;
    m_processor = nullptr;
}

DSCANNER_END_NAMESPACE

QTEST_MAIN(Dtk::Scanner::TestImageProcessingAdvanced)
#include "test_image_processing_advanced.moc" 