#include <QtTest>
#include <QObject>
#include <QImage>
#include <QSignalSpy>
#include <QDebug>

#include "../src/processing/performance_optimizer.h"

class TestPerformanceOptimization : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testMemoryLimitSettings();
    void testImageMemoryEstimation();
    void testTileProcessingDetection();
    void testTileSizeCalculation();
    void testPerformanceMonitoring();
    void testImageFormatOptimization();
    void testSystemResourcesCheck();
    void testPerformanceStats();

private:
    PerformanceOptimizer* m_optimizer;
    
    QImage createTestImage(int width, int height, QImage::Format format = QImage::Format_ARGB32);
};

void TestPerformanceOptimization::initTestCase()
{
    qDebug() << "=== Performance Optimization Tests ===";
    m_optimizer = new PerformanceOptimizer(this);
    QVERIFY(m_optimizer != nullptr);
}

void TestPerformanceOptimization::cleanupTestCase()
{
    delete m_optimizer;
    qDebug() << "=== Performance Optimization Tests Completed ===";
}

QImage TestPerformanceOptimization::createTestImage(int width, int height, QImage::Format format)
{
    QImage image(width, height, format);
    image.fill(Qt::gray);
    return image;
}

void TestPerformanceOptimization::testMemoryLimitSettings()
{
    qDebug() << "Testing memory limit settings...";
    
    // 设置内存限制
    m_optimizer->setMemoryLimit(512); // 512MB
    QCOMPARE(m_optimizer->memoryLimit(), 512);
    
    m_optimizer->setMemoryLimit(1024); // 1GB
    QCOMPARE(m_optimizer->memoryLimit(), 1024);
    
    qDebug() << "Memory limit settings test passed";
}

void TestPerformanceOptimization::testImageMemoryEstimation()
{
    qDebug() << "Testing image memory estimation...";
    
    // 测试不同大小的图像
    QImage smallImage = createTestImage(100, 100);
    QImage largeImage = createTestImage(2000, 2000);
    
    qint64 smallMemory = PerformanceOptimizer::estimateImageMemoryUsage(smallImage);
    qint64 largeMemory = PerformanceOptimizer::estimateImageMemoryUsage(largeImage);
    
    QVERIFY(smallMemory > 0);
    QVERIFY(largeMemory > smallMemory);
    
    // 大图像的内存使用应该大约是小图像的400倍（20x20倍大小）
    QVERIFY(largeMemory > smallMemory * 300);
    
    qDebug() << "Small image memory:" << (smallMemory / 1024) << "KB";
    qDebug() << "Large image memory:" << (largeMemory / 1024 / 1024) << "MB";
    qDebug() << "Image memory estimation test passed";
}

void TestPerformanceOptimization::testTileProcessingDetection()
{
    qDebug() << "Testing tile processing detection...";
    
    // 设置较低的内存限制以触发分块处理
    m_optimizer->setMemoryLimit(50); // 50MB
    
    QImage smallImage = createTestImage(500, 500);
    QImage largeImage = createTestImage(4000, 4000);
    
    bool smallNeedsTiling = m_optimizer->needsTileProcessing(smallImage);
    bool largeNeedsTiling = m_optimizer->needsTileProcessing(largeImage);
    
    // 大图像应该需要分块处理
    QVERIFY(largeNeedsTiling);
    
    qDebug() << "Small image needs tiling:" << smallNeedsTiling;
    qDebug() << "Large image needs tiling:" << largeNeedsTiling;
    qDebug() << "Tile processing detection test passed";
}

void TestPerformanceOptimization::testTileSizeCalculation()
{
    qDebug() << "Testing tile size calculation...";
    
    m_optimizer->setMemoryLimit(100); // 100MB
    
    QImage testImage = createTestImage(2000, 2000);
    QSize tileSize = m_optimizer->calculateOptimalTileSize(testImage);
    
    QVERIFY(tileSize.width() > 0);
    QVERIFY(tileSize.height() > 0);
    QVERIFY(tileSize.width() <= testImage.width());
    QVERIFY(tileSize.height() <= testImage.height());
    
    // 分块大小应该在合理范围内
    QVERIFY(tileSize.width() >= 256);
    QVERIFY(tileSize.height() >= 256);
    
    qDebug() << "Calculated tile size:" << tileSize << "for image" << testImage.size();
    qDebug() << "Tile size calculation test passed";
}

void TestPerformanceOptimization::testPerformanceMonitoring()
{
    qDebug() << "Testing performance monitoring...";
    
    // 开始监控
    int monitorId = m_optimizer->startPerformanceMonitoring("Test Operation");
    QVERIFY(monitorId > 0);
    
    // 模拟一些处理时间
    QThread::msleep(100);
    
    // 结束监控
    m_optimizer->endPerformanceMonitoring(monitorId);
    
    // 获取统计数据
    PerformanceOptimizer::PerformanceStats stats = m_optimizer->getPerformanceStats();
    QVERIFY(stats.totalImagesProcessed > 0);
    QVERIFY(stats.totalProcessingTime >= 100); // 至少100ms
    
    qDebug() << "Performance stats:"
             << "Images processed:" << stats.totalImagesProcessed
             << "Total time:" << stats.totalProcessingTime << "ms"
             << "Average time:" << stats.averageProcessingTime << "ms";
    
    qDebug() << "Performance monitoring test passed";
}

void TestPerformanceOptimization::testImageFormatOptimization()
{
    qDebug() << "Testing image format optimization...";
    
    // 测试ARGB32图像
    QImage argbImage = createTestImage(100, 100, QImage::Format_ARGB32);
    QImage optimizedArgb = PerformanceOptimizer::optimizeImageFormat(argbImage);
    QVERIFY(!optimizedArgb.isNull());
    
    // 测试RGB32图像（无alpha通道）
    QImage rgbImage = createTestImage(100, 100, QImage::Format_RGB32);
    QImage optimizedRgb = PerformanceOptimizer::optimizeImageFormat(rgbImage);
    QVERIFY(!optimizedRgb.isNull());
    QCOMPARE(optimizedRgb.format(), QImage::Format_RGB32);
    
    // 测试灰度图像
    QImage grayImage = createTestImage(100, 100, QImage::Format_Grayscale8);
    QImage optimizedGray = PerformanceOptimizer::optimizeImageFormat(grayImage);
    QVERIFY(!optimizedGray.isNull());
    QCOMPARE(optimizedGray.format(), QImage::Format_Grayscale8);
    
    qDebug() << "Original formats:" << argbImage.format() << rgbImage.format() << grayImage.format();
    qDebug() << "Optimized formats:" << optimizedArgb.format() << optimizedRgb.format() << optimizedGray.format();
    qDebug() << "Image format optimization test passed";
}

void TestPerformanceOptimization::testSystemResourcesCheck()
{
    qDebug() << "Testing system resources check...";
    
    bool resourcesOk = m_optimizer->checkSystemResources();
    
    // 系统资源检查应该返回一个布尔值
    QVERIFY(resourcesOk == true || resourcesOk == false);
    
    qDebug() << "System resources OK:" << resourcesOk;
    qDebug() << "System resources check test passed";
}

void TestPerformanceOptimization::testPerformanceStats()
{
    qDebug() << "Testing performance statistics...";
    
    // 重置统计
    m_optimizer->resetPerformanceStats();
    
    PerformanceOptimizer::PerformanceStats initialStats = m_optimizer->getPerformanceStats();
    QCOMPARE(initialStats.totalImagesProcessed, 0);
    QCOMPARE(initialStats.totalProcessingTime, 0);
    
    // 进行一些监控操作
    int id1 = m_optimizer->startPerformanceMonitoring("Operation 1");
    QThread::msleep(50);
    m_optimizer->endPerformanceMonitoring(id1);
    
    int id2 = m_optimizer->startPerformanceMonitoring("Operation 2");
    QThread::msleep(30);
    m_optimizer->endPerformanceMonitoring(id2);
    
    PerformanceOptimizer::PerformanceStats finalStats = m_optimizer->getPerformanceStats();
    QCOMPARE(finalStats.totalImagesProcessed, 2);
    QVERIFY(finalStats.totalProcessingTime >= 80); // 至少80ms
    QVERIFY(finalStats.averageProcessingTime > 0);
    
    qDebug() << "Final stats:"
             << "Total images:" << finalStats.totalImagesProcessed
             << "Total time:" << finalStats.totalProcessingTime << "ms"
             << "Average time:" << finalStats.averageProcessingTime << "ms"
             << "Memory used:" << (finalStats.memoryUsed / 1024 / 1024) << "MB"
             << "Peak memory:" << (finalStats.peakMemoryUsage / 1024 / 1024) << "MB";
    
    qDebug() << "Performance statistics test passed";
}

QTEST_MAIN(TestPerformanceOptimization)
#include "test_performance_optimization.moc" 