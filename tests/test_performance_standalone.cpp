#include <QtTest>
#include <QObject>
#include <QImage>
#include <QDebug>

// 直接包含性能优化器的头文件和源文件
#include "../src/processing/performance_optimizer.h"

class TestPerformanceStandalone : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testBasicFunctionality();
    void testMemoryEstimation();
    void testImageOptimization();

private:
    PerformanceOptimizer* m_optimizer;
};

void TestPerformanceStandalone::initTestCase()
{
    qDebug() << "=== Standalone Performance Tests ===";
    m_optimizer = new PerformanceOptimizer(this);
    QVERIFY(m_optimizer != nullptr);
}

void TestPerformanceStandalone::cleanupTestCase()
{
    delete m_optimizer;
    qDebug() << "=== Standalone Performance Tests Completed ===";
}

void TestPerformanceStandalone::testBasicFunctionality()
{
    qDebug() << "Testing basic functionality...";
    
    // 测试内存限制设置
    m_optimizer->setMemoryLimit(512);
    QCOMPARE(m_optimizer->memoryLimit(), 512);
    
    // 测试性能监控
    int monitorId = m_optimizer->startPerformanceMonitoring("Test Operation");
    QVERIFY(monitorId > 0);
    
    QThread::msleep(50);
    m_optimizer->endPerformanceMonitoring(monitorId);
    
    PerformanceOptimizer::PerformanceStats stats = m_optimizer->getPerformanceStats();
    QVERIFY(stats.totalImagesProcessed > 0);
    
    qDebug() << "Basic functionality test passed";
}

void TestPerformanceStandalone::testMemoryEstimation()
{
    qDebug() << "Testing memory estimation...";
    
    QImage testImage(1000, 1000, QImage::Format_ARGB32);
    testImage.fill(Qt::blue);
    
    qint64 memory = PerformanceOptimizer::estimateImageMemoryUsage(testImage);
    QVERIFY(memory > 0);
    
    // 1000x1000 ARGB32 图像应该至少使用 4MB (1000*1000*4)
    QVERIFY(memory > 4 * 1024 * 1024);
    
    qDebug() << "Estimated memory for 1000x1000 image:" << (memory / 1024 / 1024) << "MB";
    qDebug() << "Memory estimation test passed";
}

void TestPerformanceStandalone::testImageOptimization()
{
    qDebug() << "Testing image optimization...";
    
    // 测试格式优化
    QImage argbImage(100, 100, QImage::Format_ARGB32);
    argbImage.fill(Qt::red);
    
    QImage optimized = PerformanceOptimizer::optimizeImageFormat(argbImage);
    QVERIFY(!optimized.isNull());
    QCOMPARE(optimized.size(), argbImage.size());
    
    // 测试分块检测
    m_optimizer->setMemoryLimit(50); // 低内存限制
    QImage largeImage(3000, 3000, QImage::Format_ARGB32);
    bool needsTiling = m_optimizer->needsTileProcessing(largeImage);
    QVERIFY(needsTiling); // 大图像应该需要分块
    
    QSize tileSize = m_optimizer->calculateOptimalTileSize(largeImage);
    QVERIFY(tileSize.width() > 0);
    QVERIFY(tileSize.height() > 0);
    
    qDebug() << "Tile size for large image:" << tileSize;
    qDebug() << "Image optimization test passed";
}

QTEST_MAIN(TestPerformanceStandalone)
#include "test_performance_standalone.moc" 