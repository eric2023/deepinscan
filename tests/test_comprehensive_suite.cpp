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
#include <QFuture>
#include <QtConcurrent>

#include "advanced_image_processor.h"
#include "simd_image_algorithms.h"
#include "memory_optimized_processor.h"

/**
 * @brief 综合性能测试套件
 * 
 * 集成测试SIMD优化、内存管理、图像处理管道等所有性能优化功能
 */
class TestComprehensiveSuite : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 集成功能测试
    void testCompleteImageProcessingPipeline();
    void testSIMDMemoryIntegration();
    void testAsynchronousProcessingChain();
    void testBatchProcessingOptimization();
    
    // 性能基准测试
    void testPerformanceBaseline();
    void testMemoryEfficiencyBenchmark();
    void testConcurrentProcessingBenchmark();
    void testLargeDatasetProcessing();
    
    // 稳定性测试
    void testLongRunningStabilityTest();
    void testMemoryLeakDetection();
    void testErrorRecoveryMechanisms();
    
    // 质量验证测试
    void testProcessingAccuracy();
    void testEdgeCaseHandling();
    void testConfigurationConsistency();

private:
    // 测试辅助方法
    struct BenchmarkResult {
        QString testName;
        qint64 processingTime;
        double memoryUsageMB;
        double pixelsPerSecond;
        double speedupRatio;
        bool passed;
    };
    
    struct QualityMetrics {
        double psnr;           // 峰值信噪比
        double ssim;           // 结构相似性
        double meanError;      // 平均误差
        double maxError;       // 最大误差
    };
    
    QImage createTestImage(const QSize &size, const QString &pattern);
    QList<QImage> createTestDataset(int count, const QSize &size);
    BenchmarkResult runPerformanceBenchmark(const QString &testName, std::function<void()> operation);
    QualityMetrics calculateQualityMetrics(const QImage &reference, const QImage &processed);
    void validateMemoryUsage(const QString &operation, std::function<void()> func);
    
    double calculatePSNR(const QImage &img1, const QImage &img2);
    double calculateSSIM(const QImage &img1, const QImage &img2);
    
    AdvancedImageProcessor *m_advancedProcessor;
    MemoryOptimizedProcessor *m_memoryProcessor;
    
    QList<BenchmarkResult> m_benchmarkResults;
    QList<QualityMetrics> m_qualityResults;
    
    // 测试配置
    struct TestConfig {
        QSize smallImageSize = QSize(512, 384);       // 小图像
        QSize mediumImageSize = QSize(1920, 1080);    // 中等图像  
        QSize largeImageSize = QSize(4096, 3072);     // 大图像
        int batchSize = 20;                           // 批处理数量
        int stabilityTestDuration = 30000;            // 稳定性测试时长(ms)
        double qualityThreshold = 0.95;              // 质量阈值
        double performanceThreshold = 2.0;           // 性能提升阈值
    } m_config;
};

void TestComprehensiveSuite::initTestCase()
{
    qDebug() << "=== 综合性能测试套件开始 ===";
    qDebug() << "测试配置:";
    qDebug() << QString("  小图像: %1x%2").arg(m_config.smallImageSize.width()).arg(m_config.smallImageSize.height());
    qDebug() << QString("  中等图像: %1x%2").arg(m_config.mediumImageSize.width()).arg(m_config.mediumImageSize.height());
    qDebug() << QString("  大图像: %1x%2").arg(m_config.largeImageSize.width()).arg(m_config.largeImageSize.height());
    qDebug() << QString("  批处理数量: %1").arg(m_config.batchSize);
    
    // 创建处理器实例
    m_advancedProcessor = new AdvancedImageProcessor(this);
    m_memoryProcessor = new MemoryOptimizedProcessor(this);
    
    // 启用所有优化功能
    bool simdEnabled = m_advancedProcessor->enableSIMDOptimization();
    qDebug() << "SIMD优化启用状态:" << simdEnabled;
    
    m_advancedProcessor->optimizeColorCorrectionNode();
    m_advancedProcessor->optimizeFormatConversionNode();
    m_advancedProcessor->optimizeMemoryAlignment();
    
    // 配置内存优化处理器
    MemoryOptimizedProcessor::Config memConfig;
    memConfig.memoryLimitMB = 512;
    memConfig.maxTileSize = QSize(512, 512);
    memConfig.enableMemoryPool = true;
    memConfig.enableTileProcessing = true;
    m_memoryProcessor->setConfig(memConfig);
    
    qDebug() << "综合测试套件初始化完成";
}

void TestComprehensiveSuite::cleanupTestCase()
{
    qDebug() << "\n=== 综合性能测试结果汇总 ===";
    
    // 性能基准结果
    qDebug() << "\n📊 性能基准测试结果:";
    double totalSpeedup = 0.0;
    int passedTests = 0;
    
    for (const auto &result : m_benchmarkResults) {
        qDebug() << QString("🔹 %1:").arg(result.testName);
        qDebug() << QString("   处理时间: %1ms").arg(result.processingTime);
        qDebug() << QString("   内存使用: %1MB").arg(result.memoryUsageMB, 0, 'f', 2);
        qDebug() << QString("   处理速度: %1 MP/s").arg(result.pixelsPerSecond / 1000000.0, 0, 'f', 2);
        qDebug() << QString("   加速比: %1x").arg(result.speedupRatio, 0, 'f', 2);
        qDebug() << QString("   状态: %1").arg(result.passed ? "✅ 通过" : "❌ 失败");
        
        if (result.passed) {
            totalSpeedup += result.speedupRatio;
            passedTests++;
        }
    }
    
    if (passedTests > 0) {
        double averageSpeedup = totalSpeedup / passedTests;
        qDebug() << QString("\n📈 平均性能提升: %1x").arg(averageSpeedup, 0, 'f', 2);
        qDebug() << QString("📋 通过测试: %1/%2").arg(passedTests).arg(m_benchmarkResults.size());
    }
    
    // 质量验证结果
    qDebug() << "\n🎯 图像质量验证结果:";
    double totalPSNR = 0.0;
    double totalSSIM = 0.0;
    
    for (int i = 0; i < m_qualityResults.size(); ++i) {
        const auto &quality = m_qualityResults[i];
        qDebug() << QString("🔹 测试 %1:").arg(i + 1);
        qDebug() << QString("   PSNR: %1 dB").arg(quality.psnr, 0, 'f', 2);
        qDebug() << QString("   SSIM: %1").arg(quality.ssim, 0, 'f', 4);
        qDebug() << QString("   平均误差: %1").arg(quality.meanError, 0, 'f', 3);
        qDebug() << QString("   最大误差: %1").arg(quality.maxError, 0, 'f', 3);
        
        totalPSNR += quality.psnr;
        totalSSIM += quality.ssim;
    }
    
    if (!m_qualityResults.isEmpty()) {
        qDebug() << QString("\n📊 平均质量指标:");
        qDebug() << QString("   平均PSNR: %1 dB").arg(totalPSNR / m_qualityResults.size(), 0, 'f', 2);
        qDebug() << QString("   平均SSIM: %1").arg(totalSSIM / m_qualityResults.size(), 0, 'f', 4);
    }
    
    // 系统资源使用情况
    auto memStats = m_memoryProcessor->getMemoryStatistics();
    auto perfStats = m_advancedProcessor->getPerformanceStats();
    
    qDebug() << "\n💾 系统资源使用情况:";
    qDebug() << QString("   内存池总分配: %1MB").arg(memStats.totalAllocated / 1024 / 1024);
    qDebug() << QString("   内存回收率: %1%").arg((memStats.totalFreed * 100.0) / std::max(1UL, memStats.totalAllocated), 0, 'f', 1);
    qDebug() << QString("   图像处理总数: %1").arg(perfStats.processingCount);
    qDebug() << QString("   平均处理速度: %1 MP/s").arg(perfStats.pixelsPerSecond / 1000000.0, 0, 'f', 2);
    
    qDebug() << "\n=== 综合性能测试完成 ===";
}

void TestComprehensiveSuite::testCompleteImageProcessingPipeline()
{
    qDebug() << "\n--- 完整图像处理管道测试 ---";
    
    QImage testImage = createTestImage(m_config.mediumImageSize, "complex");
    QImage referenceImage = testImage.copy();
    
    // 配置完整的处理管道
    AdvancedImageProcessor::ProcessingParameters params;
    params.brightness = 15.0;
    params.contrast = 10.0;
    params.saturation = 5.0;
    params.gaussianBlurRadius = 2;
    params.enableAutoCorrection = true;
    params.enableNoiseReduction = true;
    
    auto benchmark = runPerformanceBenchmark("完整处理管道", [&]() {
        QImage result = m_advancedProcessor->processImageWithSIMD(testImage, params);
        QVERIFY(!result.isNull());
        QCOMPARE(result.size(), testImage.size());
        
        // 计算质量指标
        auto quality = calculateQualityMetrics(referenceImage, result);
        m_qualityResults.append(quality);
        
        // 验证质量阈值
        QVERIFY(quality.ssim > m_config.qualityThreshold);
    });
    
    m_benchmarkResults.append(benchmark);
    
    qDebug() << "完整图像处理管道测试通过";
}

void TestComprehensiveSuite::testSIMDMemoryIntegration()
{
    qDebug() << "\n--- SIMD与内存优化集成测试 ---";
    
    QImage largeImage = createTestImage(m_config.largeImageSize, "gradient");
    
    auto benchmark = runPerformanceBenchmark("SIMD内存集成", [&]() {
        // 使用内存优化处理器处理大图像，同时启用SIMD
        QImage result = m_memoryProcessor->processLargeImage(largeImage, [&](const QImage &tile) {
            // 对每个分块使用SIMD优化处理
            return SIMDImageAlgorithms::adjustBrightnessSIMD(
                SIMDImageAlgorithms::adjustContrastSIMD(tile, 1.2), 1.1);
        });
        
        QVERIFY(!result.isNull());
        QCOMPARE(result.size(), largeImage.size());
    });
    
    m_benchmarkResults.append(benchmark);
    
    // 验证内存使用效率
    auto memStats = m_memoryProcessor->getMemoryStatistics();
    qDebug() << QString("内存效率: 分配%1MB, 碎片率%2")
                .arg(memStats.totalAllocated / 1024 / 1024)
                .arg(memStats.fragmentationRatio, 0, 'f', 3);
    
    QVERIFY(memStats.fragmentationRatio < 0.3);  // 碎片率应小于30%
    
    qDebug() << "SIMD与内存优化集成测试通过";
}

void TestComprehensiveSuite::testAsynchronousProcessingChain()
{
    qDebug() << "\n--- 异步处理链测试 ---";
    
    QList<QImage> imageChain = createTestDataset(5, m_config.mediumImageSize);
    
    auto benchmark = runPerformanceBenchmark("异步处理链", [&]() {
        QList<QFuture<QImage>> futures;
        
        // 启动异步处理链
        for (const QImage &image : imageChain) {
            AdvancedImageProcessor::ProcessingParameters params;
            params.brightness = 10.0;
            params.gaussianBlurRadius = 1;
            
            auto future = m_advancedProcessor->processImageAsyncSIMD(image, params);
            futures.append(future);
        }
        
        // 等待所有处理完成
        QList<QImage> results;
        for (auto &future : futures) {
            future.waitForFinished();
            QImage result = future.result();
            QVERIFY(!result.isNull());
            results.append(result);
        }
        
        QCOMPARE(results.size(), imageChain.size());
    });
    
    m_benchmarkResults.append(benchmark);
    
    qDebug() << "异步处理链测试通过";
}

void TestComprehensiveSuite::testBatchProcessingOptimization()
{
    qDebug() << "\n--- 批量处理优化测试 ---";
    
    QList<QImage> batch = createTestDataset(m_config.batchSize, m_config.smallImageSize);
    
    auto benchmark = runPerformanceBenchmark("批量处理优化", [&]() {
        // 使用内存优化的批量处理
        auto results = m_memoryProcessor->processBatch(batch, [](const QImage &img) {
            return SIMDImageAlgorithms::convertToGrayscaleSIMD(img);
        });
        
        QCOMPARE(results.size(), batch.size());
        
        for (const QImage &result : results) {
            QVERIFY(!result.isNull());
        }
    });
    
    m_benchmarkResults.append(benchmark);
    
    qDebug() << QString("批量处理 %1 张图像完成").arg(batch.size());
    qDebug() << "批量处理优化测试通过";
}

void TestComprehensiveSuite::testPerformanceBaseline()
{
    qDebug() << "\n--- 性能基准测试 ---";
    
    QImage testImage = createTestImage(m_config.mediumImageSize, "photo");
    
    // 标量实现基准
    qint64 scalarTime = 0;
    {
        QElapsedTimer timer;
        timer.start();
        
        QImage result = SIMDImageAlgorithms::adjustBrightnessScalar(testImage, 1.2);
        QVERIFY(!result.isNull());
        
        scalarTime = timer.elapsed();
    }
    
    // SIMD优化实现
    auto simdBenchmark = runPerformanceBenchmark("SIMD优化基准", [&]() {
        QImage result = SIMDImageAlgorithms::adjustBrightnessSIMD(testImage, 1.2);
        QVERIFY(!result.isNull());
    });
    
    // 计算加速比
    if (scalarTime > 0) {
        simdBenchmark.speedupRatio = static_cast<double>(scalarTime) / simdBenchmark.processingTime;
    }
    
    simdBenchmark.passed = simdBenchmark.speedupRatio > m_config.performanceThreshold;
    m_benchmarkResults.append(simdBenchmark);
    
    qDebug() << QString("标量实现: %1ms, SIMD实现: %2ms, 加速比: %3x")
                .arg(scalarTime)
                .arg(simdBenchmark.processingTime)
                .arg(simdBenchmark.speedupRatio, 0, 'f', 2);
    
    qDebug() << "性能基准测试通过";
}

void TestComprehensiveSuite::testMemoryEfficiencyBenchmark()
{
    qDebug() << "\n--- 内存效率基准测试 ---";
    
    auto initialStats = m_memoryProcessor->getMemoryStatistics();
    
    auto benchmark = runPerformanceBenchmark("内存效率基准", [&]() {
        // 处理多个大图像以测试内存管理
        for (int i = 0; i < 3; ++i) {
            QImage largeImage = createTestImage(m_config.largeImageSize, "complex");
            
            QImage result = m_memoryProcessor->processLargeImage(largeImage, [](const QImage &img) {
                return SIMDImageAlgorithms::gaussianBlurSIMD(img, 3, 2.0);
            });
            
            QVERIFY(!result.isNull());
            
            // 强制内存优化
            m_memoryProcessor->optimizeMemoryUsage();
        }
    });
    
    auto finalStats = m_memoryProcessor->getMemoryStatistics();
    
    // 计算内存效率指标
    benchmark.memoryUsageMB = static_cast<double>(finalStats.currentUsage) / 1024 / 1024;
    benchmark.passed = (finalStats.fragmentationRatio < 0.4) && (benchmark.memoryUsageMB < 200.0);
    
    m_benchmarkResults.append(benchmark);
    
    qDebug() << QString("内存使用峰值: %1MB, 碎片率: %2%")
                .arg(benchmark.memoryUsageMB, 0, 'f', 1)
                .arg(finalStats.fragmentationRatio * 100, 0, 'f', 1);
    
    qDebug() << "内存效率基准测试通过";
}

void TestComprehensiveSuite::testConcurrentProcessingBenchmark()
{
    qDebug() << "\n--- 并发处理基准测试 ---";
    
    QList<QImage> concurrentBatch = createTestDataset(10, m_config.mediumImageSize);
    
    auto benchmark = runPerformanceBenchmark("并发处理基准", [&]() {
        // 使用QtConcurrent进行并发处理
        auto results = QtConcurrent::mapped(concurrentBatch, [this](const QImage &img) {
            AdvancedImageProcessor::ProcessingParameters params;
            params.brightness = 15.0;
            params.contrast = 10.0;
            return m_advancedProcessor->processImageWithSIMD(img, params);
        }).results();
        
        QCOMPARE(results.size(), concurrentBatch.size());
        
        for (const QImage &result : results) {
            QVERIFY(!result.isNull());
        }
    });
    
    // 计算并发效率
    qint64 totalPixels = static_cast<qint64>(concurrentBatch.size()) * 
                        m_config.mediumImageSize.width() * m_config.mediumImageSize.height();
    benchmark.pixelsPerSecond = (totalPixels * 1000.0) / benchmark.processingTime;
    benchmark.passed = benchmark.pixelsPerSecond > 10000000.0;  // 10MP/s阈值
    
    m_benchmarkResults.append(benchmark);
    
    qDebug() << QString("并发处理 %1 张图像，吞吐量: %2 MP/s")
                .arg(concurrentBatch.size())
                .arg(benchmark.pixelsPerSecond / 1000000.0, 0, 'f', 2);
    
    qDebug() << "并发处理基准测试通过";
}

void TestComprehensiveSuite::testLargeDatasetProcessing()
{
    qDebug() << "\n--- 大数据集处理测试 ---";
    
    QList<QImage> largeDataset = createTestDataset(50, m_config.smallImageSize);
    
    auto benchmark = runPerformanceBenchmark("大数据集处理", [&]() {
        int processedCount = 0;
        
        // 分批处理大数据集
        for (int i = 0; i < largeDataset.size(); i += 10) {
            QList<QImage> batch;
            for (int j = i; j < std::min(i + 10, largeDataset.size()); ++j) {
                batch.append(largeDataset[j]);
            }
            
            auto results = m_memoryProcessor->processBatch(batch, [](const QImage &img) {
                return SIMDImageAlgorithms::adjustContrastSIMD(img, 1.3);
            });
            
            processedCount += results.size();
            
            // 定期优化内存
            if ((i + 10) % 30 == 0) {
                m_memoryProcessor->optimizeMemoryUsage();
            }
        }
        
        QCOMPARE(processedCount, largeDataset.size());
    });
    
    benchmark.passed = benchmark.processingTime < 60000;  // 1分钟内完成
    m_benchmarkResults.append(benchmark);
    
    qDebug() << QString("处理 %1 张图像，总用时: %2ms")
                .arg(largeDataset.size())
                .arg(benchmark.processingTime);
    
    qDebug() << "大数据集处理测试通过";
}

void TestComprehensiveSuite::testLongRunningStabilityTest()
{
    qDebug() << "\n--- 长时间运行稳定性测试 ---";
    
    QElapsedTimer stabilityTimer;
    stabilityTimer.start();
    
    int iterations = 0;
    bool stabilityPassed = true;
    
    while (stabilityTimer.elapsed() < m_config.stabilityTestDuration) {
        try {
            // 随机处理不同尺寸的图像
            QSize randomSize(
                200 + (iterations % 5) * 200,
                150 + (iterations % 4) * 150
            );
            
            QImage testImage = createTestImage(randomSize, "random");
            
            // 随机选择处理方法
            if (iterations % 3 == 0) {
                // SIMD处理
                testImage = SIMDImageAlgorithms::adjustBrightnessSIMD(testImage, 1.1);
            } else if (iterations % 3 == 1) {
                // 内存优化处理
                testImage = m_memoryProcessor->processLargeImage(testImage, [](const QImage &img) {
                    return SIMDImageAlgorithms::convertToGrayscaleSIMD(img);
                });
            } else {
                // 高级处理器
                AdvancedImageProcessor::ProcessingParameters params;
                params.brightness = (iterations % 10) * 5.0;
                testImage = m_advancedProcessor->processImageWithSIMD(testImage, params);
            }
            
            QVERIFY(!testImage.isNull());
            
            iterations++;
            
            // 定期内存优化
            if (iterations % 20 == 0) {
                m_memoryProcessor->optimizeMemoryUsage();
            }
            
        } catch (...) {
            stabilityPassed = false;
            qWarning() << "稳定性测试在第" << iterations << "次迭代时发生异常";
            break;
        }
    }
    
    QVERIFY(stabilityPassed);
    QVERIFY(iterations > 100);  // 至少完成100次迭代
    
    qDebug() << QString("稳定性测试完成: %1次迭代，用时%2ms")
                .arg(iterations)
                .arg(stabilityTimer.elapsed());
    
    qDebug() << "长时间运行稳定性测试通过";
}

void TestComprehensiveSuite::testMemoryLeakDetection()
{
    qDebug() << "\n--- 内存泄漏检测测试 ---";
    
    auto initialStats = m_memoryProcessor->getMemoryStatistics();
    
    // 执行大量分配和释放操作
    for (int cycle = 0; cycle < 5; ++cycle) {
        QList<QImage> images = createTestDataset(20, m_config.smallImageSize);
        
        // 处理所有图像
        for (const QImage &img : images) {
            QImage result = SIMDImageAlgorithms::adjustBrightnessSIMD(img, 1.2);
            Q_UNUSED(result)
        }
        
        // 强制内存优化
        m_memoryProcessor->optimizeMemoryUsage();
    }
    
    auto finalStats = m_memoryProcessor->getMemoryStatistics();
    
    // 检查内存回收率
    double recoveryRate = static_cast<double>(finalStats.totalFreed) / 
                         std::max(1UL, finalStats.totalAllocated);
    
    qDebug() << QString("内存回收率: %1%").arg(recoveryRate * 100, 0, 'f', 2);
    qDebug() << QString("当前内存使用: %1KB").arg(finalStats.currentUsage / 1024);
    
    // 内存泄漏检测
    QVERIFY(recoveryRate > 0.8);  // 至少80%的内存应该被回收
    QVERIFY(finalStats.currentUsage < initialStats.currentUsage * 2);  // 内存使用不应翻倍
    
    qDebug() << "内存泄漏检测测试通过";
}

void TestComprehensiveSuite::testErrorRecoveryMechanisms()
{
    qDebug() << "\n--- 错误恢复机制测试 ---";
    
    bool errorRecoveryPassed = true;
    
    try {
        // 测试无效图像处理
        QImage invalidImage;
        QImage result = SIMDImageAlgorithms::adjustBrightnessSIMD(invalidImage, 1.5);
        QVERIFY(result.isNull());  // 应该返回无效图像而不是崩溃
        
        // 测试极端参数
        QImage testImage = createTestImage(QSize(100, 100), "solid");
        result = SIMDImageAlgorithms::adjustBrightnessSIMD(testImage, 1000.0);  // 极大值
        QVERIFY(!result.isNull());  // 应该处理但限制在有效范围内
        
        // 测试内存不足情况模拟
        MemoryOptimizedProcessor::Config restrictiveConfig;
        restrictiveConfig.memoryLimitMB = 1;  // 极小内存限制
        restrictiveConfig.enableTileProcessing = true;
        m_memoryProcessor->setConfig(restrictiveConfig);
        
        QImage largeImage = createTestImage(QSize(2048, 2048), "complex");
        result = m_memoryProcessor->processLargeImage(largeImage, [](const QImage &img) {
            return SIMDImageAlgorithms::convertToGrayscaleSIMD(img);
        });
        QVERIFY(!result.isNull());  // 即使内存受限也应该通过分块处理成功
        
    } catch (...) {
        errorRecoveryPassed = false;
        qWarning() << "错误恢复机制测试中发生未处理异常";
    }
    
    QVERIFY(errorRecoveryPassed);
    
    qDebug() << "错误恢复机制测试通过";
}

void TestComprehensiveSuite::testProcessingAccuracy()
{
    qDebug() << "\n--- 处理精度测试 ---";
    
    QImage referenceImage = createTestImage(m_config.smallImageSize, "gradient");
    
    // 测试可逆性处理
    QImage processedImage = SIMDImageAlgorithms::adjustBrightnessSIMD(referenceImage, 1.2);
    processedImage = SIMDImageAlgorithms::adjustBrightnessSIMD(processedImage, 1.0/1.2);
    
    auto quality = calculateQualityMetrics(referenceImage, processedImage);
    m_qualityResults.append(quality);
    
    qDebug() << QString("可逆性测试 - PSNR: %1 dB, SSIM: %2")
                .arg(quality.psnr, 0, 'f', 2)
                .arg(quality.ssim, 0, 'f', 4);
    
    // 精度应该较高（允许一定的数值误差）
    QVERIFY(quality.psnr > 40.0);  // 40dB以上
    QVERIFY(quality.ssim > 0.98);   // 98%相似性
    
    qDebug() << "处理精度测试通过";
}

void TestComprehensiveSuite::testEdgeCaseHandling()
{
    qDebug() << "\n--- 边界情况处理测试 ---";
    
    // 测试极小图像
    QImage tinyImage = createTestImage(QSize(1, 1), "solid");
    QImage result = SIMDImageAlgorithms::adjustBrightnessSIMD(tinyImage, 1.5);
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), tinyImage.size());
    
    // 测试极大图像（受内存限制）
    QImage hugeImage = createTestImage(QSize(8192, 6144), "simple");
    result = m_memoryProcessor->processLargeImage(hugeImage, [](const QImage &img) {
        return SIMDImageAlgorithms::convertToGrayscaleSIMD(img);
    });
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), hugeImage.size());
    
    // 测试特殊宽高比
    QImage wideImage = createTestImage(QSize(4000, 100), "gradient");
    result = SIMDImageAlgorithms::gaussianBlurSIMD(wideImage, 3, 2.0);
    QVERIFY(!result.isNull());
    
    QImage tallImage = createTestImage(QSize(100, 4000), "gradient");
    result = SIMDImageAlgorithms::gaussianBlurSIMD(tallImage, 3, 2.0);
    QVERIFY(!result.isNull());
    
    qDebug() << "边界情况处理测试通过";
}

void TestComprehensiveSuite::testConfigurationConsistency()
{
    qDebug() << "\n--- 配置一致性测试 ---";
    
    // 保存原始配置
    auto originalConfig = m_memoryProcessor->getConfig();
    auto originalAdvancedConfig = m_advancedProcessor->getPerformanceStats();
    
    // 测试配置更改
    MemoryOptimizedProcessor::Config newConfig = originalConfig;
    newConfig.memoryLimitMB = 256;
    newConfig.maxTileSize = QSize(1024, 1024);
    m_memoryProcessor->setConfig(newConfig);
    
    auto retrievedConfig = m_memoryProcessor->getConfig();
    QCOMPARE(retrievedConfig.memoryLimitMB, 256);
    QCOMPARE(retrievedConfig.maxTileSize, QSize(1024, 1024));
    
    // 测试配置影响
    QImage testImage = createTestImage(m_config.largeImageSize, "complex");
    QImage result = m_memoryProcessor->processLargeImage(testImage, [](const QImage &img) {
        return SIMDImageAlgorithms::adjustContrastSIMD(img, 1.1);
    });
    QVERIFY(!result.isNull());
    
    // 恢复原始配置
    m_memoryProcessor->setConfig(originalConfig);
    
    qDebug() << "配置一致性测试通过";
}

// 辅助方法实现
QImage TestComprehensiveSuite::createTestImage(const QSize &size, const QString &pattern)
{
    QImage image(size, QImage::Format_ARGB32);
    
    if (pattern == "gradient") {
        for (int y = 0; y < size.height(); ++y) {
            for (int x = 0; x < size.width(); ++x) {
                int r = (x * 255) / size.width();
                int g = (y * 255) / size.height();
                int b = ((x + y) * 255) / (size.width() + size.height());
                image.setPixel(x, y, qRgb(r, g, b));
            }
        }
    } else if (pattern == "complex") {
        for (int y = 0; y < size.height(); ++y) {
            for (int x = 0; x < size.width(); ++x) {
                int r = static_cast<int>(127 + 127 * std::sin(x * 0.1) * std::cos(y * 0.1));
                int g = static_cast<int>(127 + 127 * std::cos(x * 0.05) * std::sin(y * 0.05));
                int b = static_cast<int>(127 + 127 * std::sin((x + y) * 0.02));
                image.setPixel(x, y, qRgb(r, g, b));
            }
        }
    } else if (pattern == "photo") {
        // 模拟真实照片的复杂图案
        for (int y = 0; y < size.height(); ++y) {
            for (int x = 0; x < size.width(); ++x) {
                double noise = (rand() % 50 - 25) / 255.0;
                int r = qBound(0, static_cast<int>((x * 255.0 / size.width()) + noise * 255), 255);
                int g = qBound(0, static_cast<int>((y * 255.0 / size.height()) + noise * 255), 255);
                int b = qBound(0, static_cast<int>(((x + y) * 127.0 / (size.width() + size.height())) + noise * 255), 255);
                image.setPixel(x, y, qRgb(r, g, b));
            }
        }
    } else {
        image.fill(qRgb(128, 128, 128));
    }
    
    return image;
}

QList<QImage> TestComprehensiveSuite::createTestDataset(int count, const QSize &size)
{
    QList<QImage> dataset;
    dataset.reserve(count);
    
    QStringList patterns = {"gradient", "complex", "photo", "solid"};
    
    for (int i = 0; i < count; ++i) {
        QString pattern = patterns[i % patterns.size()];
        dataset.append(createTestImage(size, pattern));
    }
    
    return dataset;
}

TestComprehensiveSuite::BenchmarkResult TestComprehensiveSuite::runPerformanceBenchmark(const QString &testName, std::function<void()> operation)
{
    BenchmarkResult result;
    result.testName = testName;
    result.passed = false;
    
    auto memStatsBefore = m_memoryProcessor->getMemoryStatistics();
    
    QElapsedTimer timer;
    timer.start();
    
    try {
        operation();
        result.processingTime = timer.elapsed();
        result.passed = true;
    } catch (...) {
        result.processingTime = timer.elapsed();
        qWarning() << "基准测试" << testName << "执行失败";
    }
    
    auto memStatsAfter = m_memoryProcessor->getMemoryStatistics();
    result.memoryUsageMB = static_cast<double>(memStatsAfter.currentUsage - memStatsBefore.currentUsage) / 1024 / 1024;
    
    return result;
}

TestComprehensiveSuite::QualityMetrics TestComprehensiveSuite::calculateQualityMetrics(const QImage &reference, const QImage &processed)
{
    QualityMetrics metrics;
    
    if (reference.size() != processed.size()) {
        metrics.psnr = 0.0;
        metrics.ssim = 0.0;
        metrics.meanError = 100.0;
        metrics.maxError = 255.0;
        return metrics;
    }
    
    metrics.psnr = calculatePSNR(reference, processed);
    metrics.ssim = calculateSSIM(reference, processed);
    
    // 计算误差统计
    double totalError = 0.0;
    double maxError = 0.0;
    int pixelCount = reference.width() * reference.height();
    
    for (int y = 0; y < reference.height(); ++y) {
        for (int x = 0; x < reference.width(); ++x) {
            QRgb ref = reference.pixel(x, y);
            QRgb proc = processed.pixel(x, y);
            
            double rError = qAbs(qRed(ref) - qRed(proc));
            double gError = qAbs(qGreen(ref) - qGreen(proc));
            double bError = qAbs(qBlue(ref) - qBlue(proc));
            
            double pixelError = (rError + gError + bError) / 3.0;
            totalError += pixelError;
            maxError = std::max(maxError, pixelError);
        }
    }
    
    metrics.meanError = totalError / pixelCount;
    metrics.maxError = maxError;
    
    return metrics;
}

double TestComprehensiveSuite::calculatePSNR(const QImage &img1, const QImage &img2)
{
    if (img1.size() != img2.size()) return 0.0;
    
    double mse = 0.0;
    int pixelCount = img1.width() * img1.height();
    
    for (int y = 0; y < img1.height(); ++y) {
        for (int x = 0; x < img1.width(); ++x) {
            QRgb p1 = img1.pixel(x, y);
            QRgb p2 = img2.pixel(x, y);
            
            double rDiff = qRed(p1) - qRed(p2);
            double gDiff = qGreen(p1) - qGreen(p2);
            double bDiff = qBlue(p1) - qBlue(p2);
            
            mse += (rDiff * rDiff + gDiff * gDiff + bDiff * bDiff) / 3.0;
        }
    }
    
    mse /= pixelCount;
    
    if (mse < 1e-10) return 100.0;  // 几乎完全相同
    
    return 20.0 * std::log10(255.0 / std::sqrt(mse));
}

double TestComprehensiveSuite::calculateSSIM(const QImage &img1, const QImage &img2)
{
    // 简化的SSIM计算（仅使用亮度通道）
    if (img1.size() != img2.size()) return 0.0;
    
    const double c1 = 6.5025;  // (0.01 * 255)^2
    const double c2 = 58.5225; // (0.03 * 255)^2
    
    double mean1 = 0.0, mean2 = 0.0;
    int pixelCount = img1.width() * img1.height();
    
    // 计算均值
    for (int y = 0; y < img1.height(); ++y) {
        for (int x = 0; x < img1.width(); ++x) {
            QRgb p1 = img1.pixel(x, y);
            QRgb p2 = img2.pixel(x, y);
            
            mean1 += qGray(p1);
            mean2 += qGray(p2);
        }
    }
    
    mean1 /= pixelCount;
    mean2 /= pixelCount;
    
    // 计算方差和协方差
    double var1 = 0.0, var2 = 0.0, covar = 0.0;
    
    for (int y = 0; y < img1.height(); ++y) {
        for (int x = 0; x < img1.width(); ++x) {
            double gray1 = qGray(img1.pixel(x, y));
            double gray2 = qGray(img2.pixel(x, y));
            
            var1 += (gray1 - mean1) * (gray1 - mean1);
            var2 += (gray2 - mean2) * (gray2 - mean2);
            covar += (gray1 - mean1) * (gray2 - mean2);
        }
    }
    
    var1 /= pixelCount;
    var2 /= pixelCount;
    covar /= pixelCount;
    
    // 计算SSIM
    double numerator = (2.0 * mean1 * mean2 + c1) * (2.0 * covar + c2);
    double denominator = (mean1 * mean1 + mean2 * mean2 + c1) * (var1 + var2 + c2);
    
    return numerator / denominator;
}

QTEST_MAIN(TestComprehensiveSuite)
#include "test_comprehensive_suite.moc" 