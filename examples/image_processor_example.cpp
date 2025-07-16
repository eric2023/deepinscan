// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QImage>
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QThread>

#include "Scanner/DScannerImageProcessor.h"

using namespace DSCANNER_NAMESPACE;

class ImageProcessorExample : public QObject
{
    Q_OBJECT

public:
    ImageProcessorExample(QObject *parent = nullptr)
        : QObject(parent)
        , m_processor(new DScannerImageProcessor(this))
    {
        // 连接信号
        connect(m_processor, &DScannerImageProcessor::imageProcessed,
                this, &ImageProcessorExample::onImageProcessed);
        connect(m_processor, &DScannerImageProcessor::processingStarted,
                this, &ImageProcessorExample::onProcessingStarted);
        connect(m_processor, &DScannerImageProcessor::processingFinished,
                this, &ImageProcessorExample::onProcessingFinished);
        connect(m_processor, &DScannerImageProcessor::errorOccurred,
                this, &ImageProcessorExample::onErrorOccurred);
    }

    void runExample()
    {
        qDebug() << "=== DeepinScan 图像处理器示例 ===";
        
        // 创建测试图像
        QImage testImage = createTestImage();
        if (testImage.isNull()) {
            qWarning() << "Failed to create test image";
            QCoreApplication::quit();
            return;
        }
        
        qDebug() << "Created test image:" << testImage.size();
        
        // 示例1：基本图像处理
        demonstrateBasicProcessing(testImage);
        
        // 示例2：异步处理
        demonstrateAsyncProcessing(testImage);
        
        // 示例3：批量处理
        demonstrateBatchProcessing(testImage);
        
        // 示例4：预设配置
        demonstratePresets(testImage);
        
        // 示例5：扫描数据处理
        demonstrateScanDataProcessing();
        
        // 示例6：格式转换
        demonstrateFormatConversion(testImage);
        
        // 延迟退出以等待异步任务完成
        QTimer::singleShot(3000, []() {
            QCoreApplication::quit();
        });
    }

private slots:
    void onImageProcessed(const ImageProcessingResult &result)
    {
        qDebug() << "Image processed:" << result.success 
                 << "Time:" << result.processingTime << "ms";
        if (!result.success) {
            qDebug() << "Error:" << result.errorMessage;
        }
    }
    
    void onProcessingStarted()
    {
        qDebug() << "Processing started";
    }
    
    void onProcessingFinished()
    {
        qDebug() << "Processing finished";
    }
    
    void onErrorOccurred(const QString &error)
    {
        qWarning() << "Processing error:" << error;
    }

private:
    QImage createTestImage()
    {
        // 创建一个简单的测试图像
        QImage image(800, 600, QImage::Format_RGB32);
        image.fill(Qt::white);
        
        // 添加一些内容
        QPainter painter(&image);
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 24));
        painter.drawText(100, 100, "DeepinScan 图像处理测试");
        
        painter.setPen(Qt::blue);
        painter.drawRect(50, 150, 200, 100);
        
        painter.setPen(Qt::red);
        painter.drawEllipse(300, 150, 150, 100);
        
        painter.setPen(Qt::green);
        painter.drawLine(50, 300, 750, 300);
        
        return image;
    }
    
    void demonstrateBasicProcessing(const QImage &image)
    {
        qDebug() << "\n--- 基本图像处理示例 ---";
        
        // 创建处理参数
        QList<ImageProcessingParameters> params;
        
        // 添加亮度调整
        ImageProcessingParameters brightnessParam(ImageProcessingAlgorithm::BrightnessAdjust);
        brightnessParam.parameters["brightness"] = 20;
        brightnessParam.priority = 1;
        params.append(brightnessParam);
        
        // 添加对比度增强
        ImageProcessingParameters contrastParam(ImageProcessingAlgorithm::ContrastEnhance);
        contrastParam.parameters["contrast"] = 15;
        contrastParam.priority = 2;
        params.append(contrastParam);
        
        // 添加锐化
        ImageProcessingParameters sharpenParam(ImageProcessingAlgorithm::Sharpen);
        sharpenParam.parameters["strength"] = 30;
        sharpenParam.priority = 3;
        params.append(sharpenParam);
        
        // 同步处理
        QImage result = m_processor->processImage(image, params);
        qDebug() << "Synchronous processing result:" << !result.isNull();
        
        // 保存结果
        if (!result.isNull()) {
            m_processor->saveImage(result, "test_basic_processing.png");
            qDebug() << "Saved processed image to test_basic_processing.png";
        }
    }
    
    void demonstrateAsyncProcessing(const QImage &image)
    {
        qDebug() << "\n--- 异步图像处理示例 ---";
        
        // 创建处理参数
        QList<ImageProcessingParameters> params;
        
        // 添加去噪
        ImageProcessingParameters denoiseParam(ImageProcessingAlgorithm::Denoise);
        denoiseParam.parameters["strength"] = 40;
        params.append(denoiseParam);
        
        // 添加伽马校正
        ImageProcessingParameters gammaParam(ImageProcessingAlgorithm::GammaCorrection);
        gammaParam.parameters["gamma"] = 1.2;
        params.append(gammaParam);
        
        // 异步处理
        auto future = m_processor->processImageAsync(image, params);
        qDebug() << "Async processing started";
        
        // 这里可以继续其他工作，结果会通过信号通知
    }
    
    void demonstrateBatchProcessing(const QImage &image)
    {
        qDebug() << "\n--- 批量处理示例 ---";
        
        // 创建多个图像
        QList<QImage> images;
        for (int i = 0; i < 3; i++) {
            QImage img = image.copy();
            // 添加一些变化
            QPainter painter(&img);
            painter.setPen(Qt::magenta);
            painter.drawText(10, 30 + i * 20, QString("Batch Image %1").arg(i + 1));
            images.append(img);
        }
        
        // 创建处理参数
        QList<ImageProcessingParameters> params;
        ImageProcessingParameters autoLevelParam(ImageProcessingAlgorithm::AutoLevel);
        params.append(autoLevelParam);
        
        // 批量处理
        QList<ImageProcessingResult> results = m_processor->processBatch(images, params);
        qDebug() << "Batch processing completed, results:" << results.size();
        
        // 保存结果
        for (int i = 0; i < results.size(); i++) {
            if (results[i].success) {
                QString filename = QString("test_batch_%1.png").arg(i + 1);
                m_processor->saveImage(results[i].processedImage, filename);
                qDebug() << "Saved batch result" << i + 1 << "to" << filename;
            }
        }
    }
    
    void demonstratePresets(const QImage &image)
    {
        qDebug() << "\n--- 预设配置示例 ---";
        
        // 创建文本优化预设
        QList<ImageProcessingParameters> textPreset;
        
        ImageProcessingParameters contrastParam(ImageProcessingAlgorithm::ContrastEnhance);
        contrastParam.parameters["contrast"] = 25;
        textPreset.append(contrastParam);
        
        ImageProcessingParameters sharpenParam(ImageProcessingAlgorithm::Sharpen);
        sharpenParam.parameters["strength"] = 50;
        textPreset.append(sharpenParam);
        
        ImageProcessingParameters deskewParam(ImageProcessingAlgorithm::Deskew);
        textPreset.append(deskewParam);
        
        // 添加预设
        m_processor->addPreset("文本优化", textPreset);
        
        // 创建照片优化预设
        QList<ImageProcessingParameters> photoPreset;
        
        ImageProcessingParameters colorParam(ImageProcessingAlgorithm::ColorCorrection);
        colorParam.parameters["whitePoint"] = QColor(240, 240, 240);
        photoPreset.append(colorParam);
        
        ImageProcessingParameters gammaParam(ImageProcessingAlgorithm::GammaCorrection);
        gammaParam.parameters["gamma"] = 1.1;
        photoPreset.append(gammaParam);
        
        // 添加预设
        m_processor->addPreset("照片优化", photoPreset);
        
        // 使用预设
        QStringList presetNames = m_processor->getPresetNames();
        qDebug() << "Available presets:" << presetNames;
        
        for (const QString &name : presetNames) {
            QList<ImageProcessingParameters> params = m_processor->getPreset(name);
            QImage result = m_processor->processImage(image, params);
            
            if (!result.isNull()) {
                QString filename = QString("test_preset_%1.png").arg(name);
                m_processor->saveImage(result, filename);
                qDebug() << "Applied preset" << name << "and saved to" << filename;
            }
        }
    }
    
    void demonstrateScanDataProcessing()
    {
        qDebug() << "\n--- 扫描数据处理示例 ---";
        
        // 创建模拟扫描数据
        QByteArray scanData = createMockScanData();
        
        // 创建扫描参数
        ScanParameters params;
        params.resolution = QSize(300, 300);
        params.colorMode = ColorMode::RGB24;
        params.scanMode = ScanMode::Photo;
        params.autoColorCorrection = true;
        params.autoContrast = true;
        params.brightness = 10;
        params.contrast = 5;
        params.gamma = 1.1;
        
        // 处理扫描数据
        QImage result = m_processor->processScanData(scanData, params);
        
        if (!result.isNull()) {
            m_processor->saveImage(result, "test_scan_data.png");
            qDebug() << "Processed scan data and saved to test_scan_data.png";
        } else {
            qDebug() << "Failed to process scan data";
        }
    }
    
    void demonstrateFormatConversion(const QImage &image)
    {
        qDebug() << "\n--- 格式转换示例 ---";
        
        // 转换为不同格式
        QList<ImageFormat> formats = {
            ImageFormat::PNG,
            ImageFormat::JPEG,
            ImageFormat::TIFF,
            ImageFormat::BMP,
            ImageFormat::PDF
        };
        
        for (ImageFormat format : formats) {
            QByteArray data = m_processor->convertToFormat(image, format, ImageQuality::High);
            if (!data.isEmpty()) {
                QString extension;
                switch (format) {
                case ImageFormat::PNG: extension = "png"; break;
                case ImageFormat::JPEG: extension = "jpg"; break;
                case ImageFormat::TIFF: extension = "tiff"; break;
                case ImageFormat::BMP: extension = "bmp"; break;
                case ImageFormat::PDF: extension = "pdf"; break;
                default: extension = "bin"; break;
                }
                
                QString filename = QString("test_format.%1").arg(extension);
                QFile file(filename);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(data);
                    file.close();
                    qDebug() << "Converted to" << extension << "format, size:" << data.size();
                }
            }
        }
    }
    
    QByteArray createMockScanData()
    {
        // 创建模拟的RGB扫描数据
        int width = 100;
        int height = 100;
        QByteArray data;
        data.reserve(width * height * 3);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // 创建渐变效果
                uchar r = static_cast<uchar>(x * 255 / width);
                uchar g = static_cast<uchar>(y * 255 / height);
                uchar b = static_cast<uchar>((x + y) * 255 / (width + height));
                
                data.append(r);
                data.append(g);
                data.append(b);
            }
        }
        
        return data;
    }

private:
    DScannerImageProcessor *m_processor;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("DeepinScan Image Processor Example");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("eric2023");
    
    qDebug() << "DeepinScan Image Processor Example";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Thread count:" << QThread::idealThreadCount();
    
    // 创建并运行示例
    ImageProcessorExample example;
    
    // 延迟启动以确保事件循环已启动
    QTimer::singleShot(100, &example, &ImageProcessorExample::runExample);
    
    return app.exec();
}

#include "image_processor_example.moc" 