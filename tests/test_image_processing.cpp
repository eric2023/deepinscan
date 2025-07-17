//
// SPDX-FileCopyrightText: 2025 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QImage>
#include <QDir>
#include <QPainter>

// 测试图像处理相关头文件
#include "include/Scanner/DScannerGlobal.h"
#include "include/Scanner/DScannerTypes.h"
#include "include/Scanner/DScannerImageProcessor.h"

using namespace Dtk::Scanner;

class ImageProcessingTest : public QObject
{
    Q_OBJECT

public:
    ImageProcessingTest(QObject *parent = nullptr)
        : QObject(parent)
        , processor(new DScannerImageProcessor(this))
    {
        // 连接处理完成信号
        connect(processor, &DScannerImageProcessor::processingCompleted,
                this, &ImageProcessingTest::onProcessingCompleted);
        connect(processor, &DScannerImageProcessor::processingFailed,
                this, &ImageProcessingTest::onProcessingFailed);
    }

public slots:
    void startTest()
    {
        qDebug() << "=== DeepinScan 图像处理管道测试开始 ===";
        
        // 1. 创建测试图像
        createTestImage();
        
        // 2. 测试基础图像处理功能
        testBasicProcessing();
        
        // 3. 测试高级处理功能
        testAdvancedProcessing();
        
        // 4. 测试批量处理
        testBatchProcessing();
        
        // 5. 测试性能监控
        testPerformanceMonitoring();
        
        // 6. 设置超时
        QTimer::singleShot(8000, this, &ImageProcessingTest::finishTest);
    }

private slots:
    void onProcessingCompleted(const QImage &result, const QString &processingId)
    {
        qDebug() << "✅ 图像处理完成:" << processingId;
        qDebug() << "结果图像尺寸:" << result.size();
        qDebug() << "结果图像格式:" << result.format();
        
        processedImages[processingId] = result;
        completedProcessing++;
        
        checkTestCompletion();
    }
    
    void onProcessingFailed(const QString &error, const QString &processingId)
    {
        qDebug() << "❌ 图像处理失败:" << processingId << "错误:" << error;
        failedProcessing++;
        
        checkTestCompletion();
    }
    
    void checkTestCompletion()
    {
        if (completedProcessing + failedProcessing >= expectedProcessing) {
            finishTest();
        }
    }
    
    void finishTest()
    {
        qDebug() << "\n=== 图像处理管道测试结果 ===";
        qDebug() << "测试图像创建:" << (testImageCreated ? "✅" : "❌");
        qDebug() << "基础处理功能:" << (basicProcessingTested ? "✅" : "❌");
        qDebug() << "高级处理功能:" << (advancedProcessingTested ? "✅" : "❌");
        qDebug() << "批量处理功能:" << (batchProcessingTested ? "✅" : "❌");
        qDebug() << "性能监控功能:" << (performanceMonitoringTested ? "✅" : "❌");
        
        qDebug() << "\n处理统计:";
        qDebug() << "完成的处理任务:" << completedProcessing;
        qDebug() << "失败的处理任务:" << failedProcessing;
        qDebug() << "总处理任务:" << expectedProcessing;
        qDebug() << "成功率:" << QString::number(completedProcessing * 100.0 / expectedProcessing, 'f', 1) << "%";
        
        // 显示性能统计
        auto stats = processor->getStatistics();
        qDebug() << "\n性能统计:";
        qDebug() << "总处理时间:" << stats.value("totalProcessingTime", 0).toInt() << "ms";
        qDebug() << "平均处理时间:" << stats.value("averageProcessingTime", 0).toInt() << "ms";
        qDebug() << "内存使用峰值:" << stats.value("peakMemoryUsage", 0).toInt() << "MB";
        
        QCoreApplication::quit();
    }

private:
    void createTestImage()
    {
        qDebug() << "\n--- 创建测试图像 ---";
        
        // 创建一个简单的测试图像
        testImage = QImage(300, 200, QImage::Format_RGB888);
        testImage.fill(Qt::white);
        
        // 添加一些图案
        QPainter painter(&testImage);
        painter.setPen(Qt::black);
        painter.setBrush(Qt::blue);
        painter.drawRect(50, 50, 100, 50);
        painter.setBrush(Qt::red);
        painter.drawEllipse(200, 75, 50, 50);
        painter.drawText(100, 150, "DeepinScan Test");
        
        if (!testImage.isNull()) {
            qDebug() << "✅ 测试图像创建成功";
            qDebug() << "图像尺寸:" << testImage.size();
            qDebug() << "图像格式:" << testImage.format();
            testImageCreated = true;
        } else {
            qDebug() << "❌ 测试图像创建失败";
        }
    }
    
    void testBasicProcessing()
    {
        qDebug() << "\n--- 测试基础图像处理功能 ---";
        
        if (!testImageCreated) {
            qDebug() << "⚠️ 跳过基础处理测试 (测试图像未创建)";
            return;
        }
        
        // 测试亮度调整
        ImageProcessingOptions brightnessOptions;
        brightnessOptions.brightness = 20.0;
        processor->processImageAsync(testImage, brightnessOptions, "brightness_test");
        expectedProcessing++;
        
        // 测试对比度调整
        ImageProcessingOptions contrastOptions;
        contrastOptions.contrast = 15.0;
        processor->processImageAsync(testImage, contrastOptions, "contrast_test");
        expectedProcessing++;
        
        // 测试伽马校正
        ImageProcessingOptions gammaOptions;
        gammaOptions.gamma = 1.2;
        processor->processImageAsync(testImage, gammaOptions, "gamma_test");
        expectedProcessing++;
        
        qDebug() << "✅ 启动了" << 3 << "个基础处理任务";
        basicProcessingTested = true;
    }
    
    void testAdvancedProcessing()
    {
        qDebug() << "\n--- 测试高级图像处理功能 ---";
        
        if (!testImageCreated) {
            qDebug() << "⚠️ 跳过高级处理测试 (测试图像未创建)";
            return;
        }
        
        // 测试降噪
        ImageProcessingOptions denoiseOptions;
        denoiseOptions.denoise = true;
        processor->processImageAsync(testImage, denoiseOptions, "denoise_test");
        expectedProcessing++;
        
        // 测试锐化
        ImageProcessingOptions sharpenOptions;
        sharpenOptions.sharpen = true;
        processor->processImageAsync(testImage, sharpenOptions, "sharpen_test");
        expectedProcessing++;
        
        // 测试自动旋转
        ImageProcessingOptions rotateOptions;
        rotateOptions.autoRotate = true;
        processor->processImageAsync(testImage, rotateOptions, "rotate_test");
        expectedProcessing++;
        
        // 测试自动裁剪
        ImageProcessingOptions cropOptions;
        cropOptions.autoCrop = true;
        processor->processImageAsync(testImage, cropOptions, "crop_test");
        expectedProcessing++;
        
        qDebug() << "✅ 启动了" << 4 << "个高级处理任务";
        advancedProcessingTested = true;
    }
    
    void testBatchProcessing()
    {
        qDebug() << "\n--- 测试批量处理功能 ---";
        
        if (!testImageCreated) {
            qDebug() << "⚠️ 跳过批量处理测试 (测试图像未创建)";
            return;
        }
        
        // 创建批量图像列表
        QList<QImage> batchImages;
        for (int i = 0; i < 3; ++i) {
            QImage img = testImage.copy();
            batchImages.append(img);
        }
        
        // 设置批量处理选项
        ImageProcessingOptions batchOptions;
        batchOptions.brightness = 10.0;
        batchOptions.denoise = true;
        
        // 启动批量处理
        if (processor->processBatchAsync(batchImages, batchOptions, "batch_test")) {
            qDebug() << "✅ 批量处理启动成功";
            qDebug() << "批量图像数量:" << batchImages.size();
            expectedProcessing += batchImages.size();
            batchProcessingTested = true;
        } else {
            qDebug() << "❌ 批量处理启动失败";
        }
    }
    
    void testPerformanceMonitoring()
    {
        qDebug() << "\n--- 测试性能监控功能 ---";
        
        // 启用性能监控
        if (processor->setMonitoringEnabled(true)) {
            qDebug() << "✅ 性能监控启用成功";
            
            // 获取当前统计信息
            auto stats = processor->getStatistics();
            qDebug() << "当前统计信息:";
            for (auto it = stats.begin(); it != stats.end(); ++it) {
                qDebug() << "  " << it.key() << ":" << it.value().toString();
            }
            
            performanceMonitoringTested = true;
        } else {
            qDebug() << "❌ 性能监控启用失败";
        }
    }

private:
    DScannerImageProcessor *processor;
    QImage testImage;
    QMap<QString, QImage> processedImages;
    
    // 测试状态标记
    bool testImageCreated = false;
    bool basicProcessingTested = false;
    bool advancedProcessingTested = false;
    bool batchProcessingTested = false;
    bool performanceMonitoringTested = false;
    
    // 处理统计
    int completedProcessing = 0;
    int failedProcessing = 0;
    int expectedProcessing = 0;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "DeepinScan 图像处理管道测试" << std::endl;
    std::cout << "============================" << std::endl;
    
    ImageProcessingTest test;
    
    // 启动测试
    QTimer::singleShot(100, &test, &ImageProcessingTest::startTest);
    
    return app.exec();
}

#include "test_image_processing.moc" 