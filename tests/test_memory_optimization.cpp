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
#include <QSignalSpy>

#include "memory_optimized_processor.h"

/**
 * @brief 内存优化测试类
 * 
 * 测试内存池管理和大图像分块处理的功能和性能
 */
class TestMemoryOptimization : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 内存池测试
    void testMemoryPoolBasicAllocation();
    void testMemoryPoolAlignment();
    void testMemoryPoolFragmentation();
    void testMemoryPoolExpansion();
    void testMemoryPoolStatistics();
    
    // 分块处理测试
    void testTileCalculation();
    void testTileExtraction();
    void testTileMerging();
    void testTileOverlapHandling();
    
    // 大图像处理测试
    void testLargeImageProcessing();
    void testMemoryLimitHandling();
    void testBatchProcessingMemoryEfficiency();
    
    // 性能测试
    void testProcessingPerformance();
    void testMemoryUsageEfficiency();

private:
    // 测试辅助方法
    QImage createTestImage(int width, int height, const QString &pattern = "gradient");
    QList<QImage> createTestBatch(int count, const QSize &size);
    void verifyImageIntegrity(const QImage &original, const QImage &processed);
    void monitorMemoryUsage(std::function<void()> operation);
    
    // 简单的图像处理函数用于测试
    QImage simpleImageProcessor(const QImage &image);
    QImage complexImageProcessor(const QImage &image);
    
    MemoryOptimizedProcessor *m_processor;
    QList<QPair<QString, qint64>> m_performanceResults;
};

void TestMemoryOptimization::initTestCase()
{
    qDebug() << "=== 内存优化测试开始 ===";
    
    m_processor = new MemoryOptimizedProcessor(this);
    
    // 配置测试环境
    MemoryOptimizedProcessor::Config config;
    config.memoryLimitMB = 128;  // 128MB限制用于测试
    config.maxTileSize = QSize(256, 256);  // 较小的分块
    config.tileOverlap = 8;
    config.enableMemoryPool = true;
    config.enableTileProcessing = true;
    config.poolInitialSizeMB = 32;  // 32MB初始池
    
    m_processor->setConfig(config);
    
    qDebug() << "内存优化测试配置完成";
}

void TestMemoryOptimization::cleanupTestCase()
{
    qDebug() << "\n=== 内存优化测试结果汇总 ===";
    
    for (const auto &result : m_performanceResults) {
        qDebug() << QString("%1: %2ms").arg(result.first).arg(result.second);
    }
    
    auto stats = m_processor->getMemoryStatistics();
    qDebug() << "\n最终内存统计:";
    qDebug() << QString("总分配: %1MB").arg(stats.totalAllocated / 1024 / 1024);
    qDebug() << QString("总释放: %1MB").arg(stats.totalFreed / 1024 / 1024);
    qDebug() << QString("分配次数: %1").arg(stats.allocationCount);
    qDebug() << QString("碎片化率: %1").arg(stats.fragmentationRatio, 0, 'f', 3);
    
    qDebug() << "=== 内存优化测试完成 ===";
}

void TestMemoryOptimization::testMemoryPoolBasicAllocation()
{
    qDebug() << "\n--- 内存池基础分配测试 ---";
    
    MemoryPool pool(1024 * 1024);  // 1MB池
    
    // 测试基本分配
    void* ptr1 = pool.allocate(1024, 16);
    QVERIFY(ptr1 != nullptr);
    
    void* ptr2 = pool.allocate(2048, 32);
    QVERIFY(ptr2 != nullptr);
    QVERIFY(ptr1 != ptr2);
    
    // 检查统计信息
    auto stats = pool.getStatistics();
    QVERIFY(stats.allocationCount >= 2);
    QVERIFY(stats.currentUsage >= 3072);
    
    // 测试释放
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
    
    stats = pool.getStatistics();
    QVERIFY(stats.deallocationCount >= 2);
    QVERIFY(stats.totalFreed >= 3072);
    
    qDebug() << "内存池基础分配测试通过";
}

void TestMemoryOptimization::testMemoryPoolAlignment()
{
    qDebug() << "\n--- 内存池对齐测试 ---";
    
    MemoryPool pool(1024 * 1024);
    
    // 测试不同的对齐要求
    void* ptr16 = pool.allocate(100, 16);
    void* ptr32 = pool.allocate(100, 32);
    void* ptr64 = pool.allocate(100, 64);
    
    QVERIFY(ptr16 != nullptr);
    QVERIFY(ptr32 != nullptr);
    QVERIFY(ptr64 != nullptr);
    
    // 验证对齐
    QVERIFY(reinterpret_cast<uintptr_t>(ptr16) % 16 == 0);
    QVERIFY(reinterpret_cast<uintptr_t>(ptr32) % 32 == 0);
    QVERIFY(reinterpret_cast<uintptr_t>(ptr64) % 64 == 0);
    
    pool.deallocate(ptr16);
    pool.deallocate(ptr32);
    pool.deallocate(ptr64);
    
    qDebug() << "内存池对齐测试通过";
}

void TestMemoryOptimization::testMemoryPoolFragmentation()
{
    qDebug() << "\n--- 内存池碎片化测试 ---";
    
    MemoryPool pool(1024 * 1024);
    
    // 分配多个小块
    QList<void*> ptrs;
    for (int i = 0; i < 100; ++i) {
        void* ptr = pool.allocate(1024, 16);
        if (ptr) {
            ptrs.append(ptr);
        }
    }
    
    // 释放一半（造成碎片化）
    for (int i = 0; i < ptrs.size(); i += 2) {
        pool.deallocate(ptrs[i]);
    }
    
    auto statsBefore = pool.getStatistics();
    qDebug() << "压缩前碎片化率:" << statsBefore.fragmentationRatio;
    
    // 执行压缩
    pool.compact();
    
    auto statsAfter = pool.getStatistics();
    qDebug() << "压缩后碎片化率:" << statsAfter.fragmentationRatio;
    
    // 清理剩余指针
    for (int i = 1; i < ptrs.size(); i += 2) {
        pool.deallocate(ptrs[i]);
    }
    
    qDebug() << "内存池碎片化测试通过";
}

void TestMemoryOptimization::testMemoryPoolExpansion()
{
    qDebug() << "\n--- 内存池扩展测试 ---";
    
    MemoryPool pool(64 * 1024);  // 64KB小池
    
    auto initialStats = pool.getStatistics();
    size_t initialPoolSize = initialStats.poolSize;
    
    // 分配超过初始池大小的内存
    void* largePtr = pool.allocate(128 * 1024, 32);  // 128KB
    QVERIFY(largePtr != nullptr);
    
    auto expandedStats = pool.getStatistics();
    QVERIFY(expandedStats.poolSize > initialPoolSize);
    
    qDebug() << QString("内存池从%1KB扩展到%2KB")
                .arg(initialPoolSize / 1024)
                .arg(expandedStats.poolSize / 1024);
    
    pool.deallocate(largePtr);
    
    qDebug() << "内存池扩展测试通过";
}

void TestMemoryOptimization::testMemoryPoolStatistics()
{
    qDebug() << "\n--- 内存池统计测试 ---";
    
    MemoryPool pool(1024 * 1024);
    
    // 执行一系列分配和释放操作
    QList<void*> ptrs;
    for (int i = 0; i < 50; ++i) {
        size_t size = (i + 1) * 100;
        void* ptr = pool.allocate(size, 16);
        if (ptr) {
            ptrs.append(ptr);
        }
    }
    
    auto allocStats = pool.getStatistics();
    qDebug() << "分配后统计:";
    qDebug() << QString("  总分配: %1KB").arg(allocStats.totalAllocated / 1024);
    qDebug() << QString("  当前使用: %1KB").arg(allocStats.currentUsage / 1024);
    qDebug() << QString("  分配次数: %1").arg(allocStats.allocationCount);
    
    // 释放所有
    for (void* ptr : ptrs) {
        pool.deallocate(ptr);
    }
    
    auto finalStats = pool.getStatistics();
    qDebug() << "释放后统计:";
    qDebug() << QString("  总释放: %1KB").arg(finalStats.totalFreed / 1024);
    qDebug() << QString("  当前使用: %1KB").arg(finalStats.currentUsage / 1024);
    qDebug() << QString("  释放次数: %1").arg(finalStats.deallocationCount);
    
    QVERIFY(finalStats.totalAllocated > 0);
    QVERIFY(finalStats.allocationCount == ptrs.size());
    QVERIFY(finalStats.deallocationCount == ptrs.size());
    
    qDebug() << "内存池统计测试通过";
}

void TestMemoryOptimization::testTileCalculation()
{
    qDebug() << "\n--- 分块计算测试 ---";
    
    TileProcessor processor(QSize(256, 256), 16);
    
    // 测试不同尺寸图像的分块计算
    QSize imageSize(1000, 800);
    auto tiles = processor.calculateTiles(imageSize);
    
    QVERIFY(!tiles.isEmpty());
    
    // 验证分块覆盖整个图像
    QRect combinedRegion;
    for (const auto &tile : tiles) {
        combinedRegion = combinedRegion.united(tile.region);
        QVERIFY(tile.region.isValid());
        QVERIFY(tile.tileIndex >= 0);
        QCOMPARE(tile.originalSize, imageSize);
    }
    
    QVERIFY(combinedRegion.contains(QRect(0, 0, imageSize.width(), imageSize.height())));
    
    qDebug() << QString("图像%1x%2分解为%3个分块")
                .arg(imageSize.width()).arg(imageSize.height()).arg(tiles.size());
    
    qDebug() << "分块计算测试通过";
}

void TestMemoryOptimization::testTileExtraction()
{
    qDebug() << "\n--- 分块提取测试 ---";
    
    QImage testImage = createTestImage(800, 600, "checkerboard");
    TileProcessor processor(QSize(200, 200), 8);
    
    auto tiles = processor.calculateTiles(testImage.size());
    
    for (int i = 0; i < std::min(3, tiles.size()); ++i) {
        const auto &tileInfo = tiles[i];
        QImage extractedTile = processor.extractTile(testImage, tileInfo);
        
        QVERIFY(!extractedTile.isNull());
        QVERIFY(extractedTile.size().width() <= tileInfo.region.width());
        QVERIFY(extractedTile.size().height() <= tileInfo.region.height());
        
        qDebug() << QString("分块%1: 区域%2 -> 图像%3x%4")
                    .arg(i)
                    .arg(QString("(%1,%2,%3x%4)")
                         .arg(tileInfo.region.x())
                         .arg(tileInfo.region.y())
                         .arg(tileInfo.region.width())
                         .arg(tileInfo.region.height()))
                    .arg(extractedTile.width())
                    .arg(extractedTile.height());
    }
    
    qDebug() << "分块提取测试通过";
}

void TestMemoryOptimization::testTileMerging()
{
    qDebug() << "\n--- 分块合并测试 ---";
    
    QImage originalImage = createTestImage(400, 300, "gradient");
    TileProcessor processor(QSize(150, 150), 10);
    
    // 提取分块
    auto tileInfos = processor.calculateTiles(originalImage.size());
    QList<QImage> tiles;
    
    for (const auto &tileInfo : tileInfos) {
        QImage tile = processor.extractTile(originalImage, tileInfo);
        tiles.append(tile);
    }
    
    // 合并分块
    QImage mergedImage = processor.mergeTiles(tiles, tileInfos, originalImage.size());
    
    QVERIFY(!mergedImage.isNull());
    QCOMPARE(mergedImage.size(), originalImage.size());
    
    // 验证合并质量（简单像素比较）
    int differentPixels = 0;
    int totalPixels = originalImage.width() * originalImage.height();
    
    for (int y = 0; y < originalImage.height(); ++y) {
        for (int x = 0; x < originalImage.width(); ++x) {
            QRgb orig = originalImage.pixel(x, y);
            QRgb merged = mergedImage.pixel(x, y);
            
            // 允许小的颜色差异（由于重叠融合）
            int rDiff = qAbs(qRed(orig) - qRed(merged));
            int gDiff = qAbs(qGreen(orig) - qGreen(merged));
            int bDiff = qAbs(qBlue(orig) - qBlue(merged));
            
            if (rDiff > 10 || gDiff > 10 || bDiff > 10) {
                differentPixels++;
            }
        }
    }
    
    double differenceRatio = static_cast<double>(differentPixels) / totalPixels;
    qDebug() << QString("像素差异率: %1%").arg(differenceRatio * 100, 0, 'f', 2);
    
    // 允许5%的像素差异（重叠区域的融合效果）
    QVERIFY(differenceRatio < 0.05);
    
    qDebug() << "分块合并测试通过";
}

void TestMemoryOptimization::testTileOverlapHandling()
{
    qDebug() << "\n--- 分块重叠处理测试 ---";
    
    QImage testImage = createTestImage(300, 300, "solid");
    
    // 测试不同重叠设置
    for (int overlap : {0, 5, 10, 20}) {
        TileProcessor processor(QSize(100, 100), overlap);
        
        auto tileInfos = processor.calculateTiles(testImage.size());
        QList<QImage> tiles;
        
        for (const auto &tileInfo : tileInfos) {
            tiles.append(processor.extractTile(testImage, tileInfo));
        }
        
        QImage merged = processor.mergeTiles(tiles, tileInfos, testImage.size());
        
        QVERIFY(!merged.isNull());
        QCOMPARE(merged.size(), testImage.size());
        
        qDebug() << QString("重叠%1像素: 生成%2个分块").arg(overlap).arg(tiles.size());
    }
    
    qDebug() << "分块重叠处理测试通过";
}

void TestMemoryOptimization::testLargeImageProcessing()
{
    qDebug() << "\n--- 大图像处理测试 ---";
    
    // 创建大图像（会触发分块处理）
    QImage largeImage = createTestImage(2048, 1536);  // 3MP图像
    
    QElapsedTimer timer;
    timer.start();
    
    // 使用简单处理器处理
    QImage result = m_processor->processLargeImage(largeImage, 
        [this](const QImage &img) { return simpleImageProcessor(img); });
    
    qint64 processingTime = timer.elapsed();
    m_performanceResults.append(qMakePair("大图像处理", processingTime));
    
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), largeImage.size());
    
    qDebug() << QString("大图像处理完成: %1x%2 -> %3x%4, 用时%5ms")
                .arg(largeImage.width()).arg(largeImage.height())
                .arg(result.width()).arg(result.height())
                .arg(processingTime);
    
    qDebug() << "大图像处理测试通过";
}

void TestMemoryOptimization::testMemoryLimitHandling()
{
    qDebug() << "\n--- 内存限制处理测试 ---";
    
    // 设置严格的内存限制
    auto originalConfig = m_processor->getConfig();
    auto testConfig = originalConfig;
    testConfig.memoryLimitMB = 32;  // 32MB严格限制
    m_processor->setConfig(testConfig);
    
    // 监控内存使用信号
    QSignalSpy memorySignal(m_processor, &MemoryOptimizedProcessor::memoryUsageChanged);
    QSignalSpy optimizationSignal(m_processor, &MemoryOptimizedProcessor::memoryOptimizationCompleted);
    
    // 处理多个中等大小图像
    QList<QImage> images;
    for (int i = 0; i < 5; ++i) {
        images.append(createTestImage(800, 600));
    }
    
    auto results = m_processor->processBatch(images, 
        [this](const QImage &img) { return complexImageProcessor(img); });
    
    QCOMPARE(results.size(), images.size());
    
    // 验证内存监控信号
    QVERIFY(memorySignal.count() > 0);
    
    // 恢复原配置
    m_processor->setConfig(originalConfig);
    
    qDebug() << QString("内存监控信号: %1次").arg(memorySignal.count());
    qDebug() << QString("内存优化信号: %1次").arg(optimizationSignal.count());
    
    qDebug() << "内存限制处理测试通过";
}

void TestMemoryOptimization::testBatchProcessingMemoryEfficiency()
{
    qDebug() << "\n--- 批量处理内存效率测试 ---";
    
    auto initialStats = m_processor->getMemoryStatistics();
    
    // 创建批量图像
    QList<QImage> batch = createTestBatch(10, QSize(400, 300));
    
    QElapsedTimer timer;
    timer.start();
    
    auto results = m_processor->processBatch(batch,
        [this](const QImage &img) { return simpleImageProcessor(img); });
    
    qint64 batchTime = timer.elapsed();
    m_performanceResults.append(qMakePair("批量处理", batchTime));
    
    auto finalStats = m_processor->getMemoryStatistics();
    
    QCOMPARE(results.size(), batch.size());
    
    qDebug() << QString("批量处理%1张图像，用时%2ms").arg(batch.size()).arg(batchTime);
    qDebug() << QString("内存使用变化: %1KB -> %2KB")
                .arg(initialStats.currentUsage / 1024)
                .arg(finalStats.currentUsage / 1024);
    qDebug() << QString("平均每张: %1ms").arg(batchTime / batch.size());
    
    qDebug() << "批量处理内存效率测试通过";
}

void TestMemoryOptimization::testProcessingPerformance()
{
    qDebug() << "\n--- 处理性能测试 ---";
    
    QImage testImage = createTestImage(1024, 768);
    
    // 测试不同处理复杂度
    struct {
        QString name;
        std::function<QImage(const QImage&)> processor;
    } processors[] = {
        {"简单处理", [this](const QImage &img) { return simpleImageProcessor(img); }},
        {"复杂处理", [this](const QImage &img) { return complexImageProcessor(img); }}
    };
    
    for (const auto &proc : processors) {
        QElapsedTimer timer;
        timer.start();
        
        QImage result = m_processor->processLargeImage(testImage, proc.processor);
        
        qint64 time = timer.elapsed();
        m_performanceResults.append(qMakePair(proc.name, time));
        
        QVERIFY(!result.isNull());
        
        qDebug() << QString("%1: %2ms").arg(proc.name).arg(time);
    }
    
    qDebug() << "处理性能测试通过";
}

void TestMemoryOptimization::testMemoryUsageEfficiency()
{
    qDebug() << "\n--- 内存使用效率测试 ---";
    
    auto beforeStats = m_processor->getMemoryStatistics();
    
    // 执行内存密集型操作
    monitorMemoryUsage([this]() {
        QImage largeImage = createTestImage(4096, 3072);  // 12MP图像
        QImage result = m_processor->processLargeImage(largeImage,
            [this](const QImage &img) { return complexImageProcessor(img); });
        QVERIFY(!result.isNull());
    });
    
    auto afterStats = m_processor->getMemoryStatistics();
    
    qDebug() << "内存使用效率分析:";
    qDebug() << QString("峰值分配: %1MB").arg(afterStats.totalAllocated / 1024 / 1024);
    qDebug() << QString("总释放: %1MB").arg(afterStats.totalFreed / 1024 / 1024);
    qDebug() << QString("分配效率: %1次/MB").arg(afterStats.allocationCount / std::max(1.0, static_cast<double>(afterStats.totalAllocated) / 1024 / 1024));
    
    // 验证内存得到有效回收
    QVERIFY(afterStats.totalFreed >= afterStats.totalAllocated * 0.8);  // 至少80%回收
    
    qDebug() << "内存使用效率测试通过";
}

// 辅助方法实现
QImage TestMemoryOptimization::createTestImage(int width, int height, const QString &pattern)
{
    QImage image(width, height, QImage::Format_ARGB32);
    
    if (pattern == "gradient") {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int r = (x * 255) / width;
                int g = (y * 255) / height;
                int b = ((x + y) * 255) / (width + height);
                image.setPixel(x, y, qRgb(r, g, b));
            }
        }
    } else if (pattern == "checkerboard") {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                bool black = ((x / 32) + (y / 32)) % 2 == 0;
                QRgb color = black ? qRgb(0, 0, 0) : qRgb(255, 255, 255);
                image.setPixel(x, y, color);
            }
        }
    } else if (pattern == "solid") {
        image.fill(qRgb(128, 128, 128));
    }
    
    return image;
}

QList<QImage> TestMemoryOptimization::createTestBatch(int count, const QSize &size)
{
    QList<QImage> batch;
    batch.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        QImage image = createTestImage(size.width(), size.height());
        batch.append(image);
    }
    
    return batch;
}

QImage TestMemoryOptimization::simpleImageProcessor(const QImage &image)
{
    // 简单的亮度调整
    QImage result = image.copy();
    
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            QRgb pixel = result.pixel(x, y);
            int r = qMin(255, qRed(pixel) + 20);
            int g = qMin(255, qGreen(pixel) + 20);
            int b = qMin(255, qBlue(pixel) + 20);
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QImage TestMemoryOptimization::complexImageProcessor(const QImage &image)
{
    // 复杂的多步处理
    QImage result = image.copy();
    
    // 步骤1：高斯模糊近似
    QImage blurred(result.size(), result.format());
    for (int y = 1; y < result.height() - 1; ++y) {
        for (int x = 1; x < result.width() - 1; ++x) {
            int r = 0, g = 0, b = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    QRgb pixel = result.pixel(x + dx, y + dy);
                    r += qRed(pixel);
                    g += qGreen(pixel);
                    b += qBlue(pixel);
                }
            }
            blurred.setPixel(x, y, qRgb(r / 9, g / 9, b / 9));
        }
    }
    
    // 步骤2：对比度增强
    for (int y = 0; y < blurred.height(); ++y) {
        for (int x = 0; x < blurred.width(); ++x) {
            QRgb pixel = blurred.pixel(x, y);
            int r = qBound(0, (qRed(pixel) - 128) * 2 + 128, 255);
            int g = qBound(0, (qGreen(pixel) - 128) * 2 + 128, 255);
            int b = qBound(0, (qBlue(pixel) - 128) * 2 + 128, 255);
            blurred.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return blurred;
}

void TestMemoryOptimization::monitorMemoryUsage(std::function<void()> operation)
{
    auto beforeStats = m_processor->getMemoryStatistics();
    
    QElapsedTimer timer;
    timer.start();
    
    operation();
    
    qint64 operationTime = timer.elapsed();
    auto afterStats = m_processor->getMemoryStatistics();
    
    qDebug() << QString("操作耗时: %1ms").arg(operationTime);
    qDebug() << QString("内存分配: %1KB").arg((afterStats.totalAllocated - beforeStats.totalAllocated) / 1024);
    qDebug() << QString("内存释放: %1KB").arg((afterStats.totalFreed - beforeStats.totalFreed) / 1024);
}

QTEST_MAIN(TestMemoryOptimization)
#include "test_memory_optimization.moc" 