/*
 * Copyright (C) 2024 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtTest>
#include <QImage>
#include <QElapsedTimer>
#include <QDebug>

#include "simd_image_algorithms.h"
#include "advanced_image_processor.h"

/**
 * @brief SIMD性能测试类
 * 
 * 测试SIMD优化前后图像处理算法的性能差异
 */
class TestSIMDPerformance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 性能基准测试
    void testBrightnessAdjustmentPerformance();
    void testContrastAdjustmentPerformance();
    void testGrayscaleConversionPerformance();
    void testGaussianBlurPerformance();
    void testSaturationAdjustmentPerformance();
    
    // 批量处理性能测试
    void testBatchProcessingPerformance();
    
    // 内存使用测试
    void testMemoryUsageOptimization();
    
    // 多线程性能测试
    void testMultithreadedProcessingPerformance();

private:
    // 测试辅助方法
    QImage createTestImage(int width, int height);
    QList<QImage> createTestImageBatch(int count, int width, int height);
    void compareProcessingTime(const QString &testName, qint64 scalarTime, qint64 simdTime);
    double calculateSpeedup(qint64 scalarTime, qint64 simdTime);
    
    // 测试图像数据
    QImage m_testImage;
    QList<QImage> m_testBatch;
    
    // 处理器实例
    AdvancedImageProcessor *m_processor;
    
    // 性能统计
    struct PerformanceResult {
        QString testName;
        qint64 scalarTime;
        qint64 simdTime;
        double speedup;
        bool successful;
    };
    
    QList<PerformanceResult> m_results;
};

void TestSIMDPerformance::initTestCase()
{
    qDebug() << "=== SIMD性能测试开始 ===";
    
    // 创建测试图像
    m_testImage = createTestImage(1920, 1080);  // Full HD图像
    m_testBatch = createTestImageBatch(10, 800, 600);  // 批量处理测试
    
    // 创建图像处理器
    m_processor = new AdvancedImageProcessor(this);
    
    // 检测SIMD支持
    QString simdInfo = SIMDImageAlgorithms::detectSIMDSupport();
    qDebug() << "SIMD支持情况:" << simdInfo;
    
    QVERIFY(!m_testImage.isNull());
    QVERIFY(!m_testBatch.isEmpty());
    
    // 启用SIMD优化
    bool simdEnabled = m_processor->enableSIMDOptimization();
    qDebug() << "SIMD优化启用状态:" << simdEnabled;
}

void TestSIMDPerformance::cleanupTestCase()
{
    qDebug() << "\n=== 性能测试结果汇总 ===";
    
    double totalSpeedup = 0.0;
    int successfulTests = 0;
    
    for (const auto &result : m_results) {
        qDebug() << QString("测试: %1").arg(result.testName);
        qDebug() << QString("  标量时间: %1ms").arg(result.scalarTime);
        qDebug() << QString("  SIMD时间: %2ms").arg(result.simdTime);
        qDebug() << QString("  加速比: %3x").arg(result.speedup, 0, 'f', 2);
        qDebug() << QString("  状态: %4").arg(result.successful ? "成功" : "失败");
        
        if (result.successful) {
            totalSpeedup += result.speedup;
            successfulTests++;
        }
    }
    
    if (successfulTests > 0) {
        double averageSpeedup = totalSpeedup / successfulTests;
        qDebug() << QString("\n平均加速比: %1x").arg(averageSpeedup, 0, 'f', 2);
        qDebug() << QString("成功测试数: %1/%2").arg(successfulTests).arg(m_results.size());
    }
    
    qDebug() << "=== SIMD性能测试完成 ===";
}

void TestSIMDPerformance::testBrightnessAdjustmentPerformance()
{
    qDebug() << "\n--- 亮度调整性能测试 ---";
    
    const double brightnessFactor = 1.2;
    QElapsedTimer timer;
    
    // 标量实现测试
    timer.start();
    QImage scalarResult = SIMDImageAlgorithms::adjustBrightnessScalar(m_testImage, brightnessFactor);
    qint64 scalarTime = timer.elapsed();
    
    // SIMD实现测试
    timer.start();
    QImage simdResult = SIMDImageAlgorithms::adjustBrightnessSIMD(m_testImage, brightnessFactor);
    qint64 simdTime = timer.elapsed();
    
    // 验证结果正确性
    QVERIFY(!scalarResult.isNull());
    QVERIFY(!simdResult.isNull());
    QCOMPARE(scalarResult.size(), simdResult.size());
    
    // 记录性能结果
    PerformanceResult result;
    result.testName = "亮度调整";
    result.scalarTime = scalarTime;
    result.simdTime = simdTime;
    result.speedup = calculateSpeedup(scalarTime, simdTime);
    result.successful = true;
    m_results.append(result);
    
    compareProcessingTime("亮度调整", scalarTime, simdTime);
}

void TestSIMDPerformance::testContrastAdjustmentPerformance()
{
    qDebug() << "\n--- 对比度调整性能测试 ---";
    
    const double contrastFactor = 1.3;
    QElapsedTimer timer;
    
    // 标量实现测试
    timer.start();
    QImage scalarResult = SIMDImageAlgorithms::adjustContrastScalar(m_testImage, contrastFactor);
    qint64 scalarTime = timer.elapsed();
    
    // SIMD实现测试
    timer.start();
    QImage simdResult = SIMDImageAlgorithms::adjustContrastSIMD(m_testImage, contrastFactor);
    qint64 simdTime = timer.elapsed();
    
    // 验证结果
    QVERIFY(!scalarResult.isNull());
    QVERIFY(!simdResult.isNull());
    
    // 记录性能结果
    PerformanceResult result;
    result.testName = "对比度调整";
    result.scalarTime = scalarTime;
    result.simdTime = simdTime;
    result.speedup = calculateSpeedup(scalarTime, simdTime);
    result.successful = true;
    m_results.append(result);
    
    compareProcessingTime("对比度调整", scalarTime, simdTime);
}

void TestSIMDPerformance::testGrayscaleConversionPerformance()
{
    qDebug() << "\n--- 灰度转换性能测试 ---";
    
    QElapsedTimer timer;
    
    // 标量实现测试
    timer.start();
    QImage scalarResult = SIMDImageAlgorithms::convertToGrayscaleScalar(m_testImage);
    qint64 scalarTime = timer.elapsed();
    
    // SIMD实现测试
    timer.start();
    QImage simdResult = SIMDImageAlgorithms::convertToGrayscaleSIMD(m_testImage);
    qint64 simdTime = timer.elapsed();
    
    // 验证结果
    QVERIFY(!scalarResult.isNull());
    QVERIFY(!simdResult.isNull());
    
    // 记录性能结果
    PerformanceResult result;
    result.testName = "灰度转换";
    result.scalarTime = scalarTime;
    result.simdTime = simdTime;
    result.speedup = calculateSpeedup(scalarTime, simdTime);
    result.successful = true;
    m_results.append(result);
    
    compareProcessingTime("灰度转换", scalarTime, simdTime);
}

void TestSIMDPerformance::testGaussianBlurPerformance()
{
    qDebug() << "\n--- 高斯模糊性能测试 ---";
    
    const int blurRadius = 5;
    const double sigma = 2.0;
    QElapsedTimer timer;
    
    // 标量实现测试
    timer.start();
    QImage scalarResult = SIMDImageAlgorithms::gaussianBlurScalar(m_testImage, blurRadius, sigma);
    qint64 scalarTime = timer.elapsed();
    
    // SIMD实现测试
    timer.start();
    QImage simdResult = SIMDImageAlgorithms::gaussianBlurSIMD(m_testImage, blurRadius, sigma);
    qint64 simdTime = timer.elapsed();
    
    // 验证结果
    QVERIFY(!scalarResult.isNull());
    QVERIFY(!simdResult.isNull());
    
    // 记录性能结果
    PerformanceResult result;
    result.testName = "高斯模糊";
    result.scalarTime = scalarTime;
    result.simdTime = simdTime;
    result.speedup = calculateSpeedup(scalarTime, simdTime);
    result.successful = true;
    m_results.append(result);
    
    compareProcessingTime("高斯模糊", scalarTime, simdTime);
}

void TestSIMDPerformance::testSaturationAdjustmentPerformance()
{
    qDebug() << "\n--- 饱和度调整性能测试 ---";
    
    const double saturationFactor = 1.2;
    QElapsedTimer timer;
    
    // SIMD实现测试（饱和度调整暂无标量对比版本）
    timer.start();
    QImage simdResult = SIMDImageAlgorithms::adjustSaturationSIMD(m_testImage, saturationFactor);
    qint64 simdTime = timer.elapsed();
    
    // 验证结果
    QVERIFY(!simdResult.isNull());
    
    qDebug() << QString("饱和度调整SIMD时间: %1ms").arg(simdTime);
    
    // 记录基准时间
    PerformanceResult result;
    result.testName = "饱和度调整";
    result.scalarTime = 0;  // 暂无标量版本
    result.simdTime = simdTime;
    result.speedup = 0.0;
    result.successful = true;
    m_results.append(result);
}

void TestSIMDPerformance::testBatchProcessingPerformance()
{
    qDebug() << "\n--- 批量处理性能测试 ---";
    
    AdvancedImageProcessor::ProcessingParameters params;
    params.brightness = 10.0;
    params.contrast = 10.0;
    params.convertToGrayscale = false;
    
    QElapsedTimer timer;
    
    // 标准批量处理
    timer.start();
    auto standardFuture = m_processor->processBatch(m_testBatch, params);
    standardFuture.waitForFinished();
    QList<QImage> standardResults = standardFuture.result();
    qint64 standardTime = timer.elapsed();
    
    // SIMD批量处理
    timer.start();
    auto simdFuture = m_processor->processBatchSIMD(m_testBatch, params);
    simdFuture.waitForFinished();
    QList<QImage> simdResults = simdFuture.result();
    qint64 simdTime = timer.elapsed();
    
    // 验证结果
    QCOMPARE(standardResults.size(), simdResults.size());
    QCOMPARE(standardResults.size(), m_testBatch.size());
    
    // 记录性能结果
    PerformanceResult result;
    result.testName = "批量处理";
    result.scalarTime = standardTime;
    result.simdTime = simdTime;
    result.speedup = calculateSpeedup(standardTime, simdTime);
    result.successful = true;
    m_results.append(result);
    
    compareProcessingTime("批量处理", standardTime, simdTime);
}

void TestSIMDPerformance::testMemoryUsageOptimization()
{
    qDebug() << "\n--- 内存使用优化测试 ---";
    
    // 启用内存优化
    bool memoryOptimized = m_processor->optimizeMemoryAlignment();
    qDebug() << "内存对齐优化状态:" << memoryOptimized;
    
    // 测试大图像处理的内存效率
    QImage largeImage = createTestImage(4096, 4096);  // 4K图像
    
    AdvancedImageProcessor::ProcessingParameters params;
    params.brightness = 20.0;
    params.contrast = 20.0;
    
    QElapsedTimer timer;
    timer.start();
    
    QImage result = m_processor->processImageWithSIMD(largeImage, params);
    qint64 processingTime = timer.elapsed();
    
    QVERIFY(!result.isNull());
    qDebug() << QString("大图像处理时间: %1ms").arg(processingTime);
    qDebug() << QString("图像尺寸: %1x%2").arg(largeImage.width()).arg(largeImage.height());
    
    // 计算处理速度（像素/秒）
    qint64 totalPixels = static_cast<qint64>(largeImage.width()) * largeImage.height();
    double pixelsPerSecond = (totalPixels * 1000.0) / processingTime;
    qDebug() << QString("处理速度: %1 兆像素/秒").arg(pixelsPerSecond / 1000000.0, 0, 'f', 2);
}

void TestSIMDPerformance::testMultithreadedProcessingPerformance()
{
    qDebug() << "\n--- 多线程处理性能测试 ---";
    
    AdvancedImageProcessor::ProcessingParameters params;
    params.brightness = 15.0;
    params.gaussianBlurRadius = 3;
    
    QElapsedTimer timer;
    
    // 同步处理
    timer.start();
    QImage syncResult = m_processor->processImageWithSIMD(m_testImage, params);
    qint64 syncTime = timer.elapsed();
    
    // 异步处理
    timer.start();
    auto asyncFuture = m_processor->processImageAsyncSIMD(m_testImage, params);
    asyncFuture.waitForFinished();
    QImage asyncResult = asyncFuture.result();
    qint64 asyncTime = timer.elapsed();
    
    // 验证结果
    QVERIFY(!syncResult.isNull());
    QVERIFY(!asyncResult.isNull());
    QCOMPARE(syncResult.size(), asyncResult.size());
    
    qDebug() << QString("同步处理时间: %1ms").arg(syncTime);
    qDebug() << QString("异步处理时间: %2ms").arg(asyncTime);
    
    // 异步处理可能略慢，因为有线程开销
    QVERIFY(asyncTime <= syncTime * 1.5);  // 允许50%的开销
}

// 辅助方法实现
QImage TestSIMDPerformance::createTestImage(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    
    // 创建有意义的测试图案
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = (x * 255) / width;
            int g = (y * 255) / height;
            int b = ((x + y) * 255) / (width + height);
            image.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return image;
}

QList<QImage> TestSIMDPerformance::createTestImageBatch(int count, int width, int height)
{
    QList<QImage> batch;
    batch.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        QImage image = createTestImage(width, height);
        batch.append(image);
    }
    
    return batch;
}

void TestSIMDPerformance::compareProcessingTime(const QString &testName, qint64 scalarTime, qint64 simdTime)
{
    double speedup = calculateSpeedup(scalarTime, simdTime);
    
    qDebug() << QString("%1性能对比:").arg(testName);
    qDebug() << QString("  标量实现: %1ms").arg(scalarTime);
    qDebug() << QString("  SIMD实现: %2ms").arg(simdTime);
    qDebug() << QString("  性能提升: %3x").arg(speedup, 0, 'f', 2);
    
    if (speedup > 1.0) {
        qDebug() << QString("  ✓ SIMD优化有效，性能提升 %1%").arg((speedup - 1.0) * 100, 0, 'f', 1);
    } else {
        qDebug() << QString("  ⚠ SIMD优化未达到预期效果");
    }
}

double TestSIMDPerformance::calculateSpeedup(qint64 scalarTime, qint64 simdTime)
{
    if (simdTime == 0) return 0.0;
    return static_cast<double>(scalarTime) / static_cast<double>(simdTime);
}

QTEST_MAIN(TestSIMDPerformance)
#include "test_simd_performance.moc" 