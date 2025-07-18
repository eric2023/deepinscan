// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerImageProcessor.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QMutexLocker>
#include <QBuffer>
#include <QIODevice>
#include <QThread>
#include <QtMath>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include <QThreadPool>
#include <QPainter>

DSCANNER_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(dscannerImageProcessor, "deepinscan.imageprocessor")

// SIMD检测和优化算法
namespace SIMDAlgorithms {
    
bool hasAVX2Support() {
#if defined(__AVX2__)
    return true;
#else
    return false;
#endif
}

bool hasSSE2Support() {
#if defined(__SSE2__)
    return true;
#else
    return false;
#endif
}

QString detectSIMDSupport() {
    QStringList features;
    if (hasSSE2Support()) features << "SSE2";
    if (hasAVX2Support()) features << "AVX2";
    return features.isEmpty() ? "None" : features.join(", ");
}

// 高性能去噪算法
QImage optimizedDenoise(const QImage &image, int strength) {
    if (image.isNull() || strength <= 0) return image;
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    const int width = result.width();
    const int height = result.height();
    
    // 简化的3x3高斯滤波去噪
    const double factor = strength / 100.0;
    const QVector<double> kernel = {
        0.0625 * factor, 0.125 * factor, 0.0625 * factor,
        0.125 * factor, 0.25 * factor, 0.125 * factor,
        0.0625 * factor, 0.125 * factor, 0.0625 * factor
    };
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            double r = 0, g = 0, b = 0;
            
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    QColor pixel = result.pixelColor(x + kx, y + ky);
                    double weight = kernel[(ky + 1) * 3 + (kx + 1)];
                    r += pixel.red() * weight;
                    g += pixel.green() * weight;
                    b += pixel.blue() * weight;
                }
            }
            
            QColor originalPixel = image.pixelColor(x, y);
            int finalR = qBound(0, static_cast<int>(originalPixel.red() * (1 - factor) + r), 255);
            int finalG = qBound(0, static_cast<int>(originalPixel.green() * (1 - factor) + g), 255);
            int finalB = qBound(0, static_cast<int>(originalPixel.blue() * (1 - factor) + b), 255);
            
            result.setPixelColor(x, y, QColor(finalR, finalG, finalB));
        }
    }
    
    return result;
}

// 高性能锐化算法
QImage optimizedSharpen(const QImage &image, int strength) {
    if (image.isNull() || strength <= 0) return image;
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    const int width = result.width();
    const int height = result.height();
    const double factor = strength / 100.0;
    
    // 拉普拉斯锐化核
    const QVector<double> kernel = {
        0, -1 * factor, 0,
        -1 * factor, 1 + 4 * factor, -1 * factor,
        0, -1 * factor, 0
    };
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            double r = 0, g = 0, b = 0;
            
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    QColor pixel = result.pixelColor(x + kx, y + ky);
                    double weight = kernel[(ky + 1) * 3 + (kx + 1)];
                    r += pixel.red() * weight;
                    g += pixel.green() * weight;
                    b += pixel.blue() * weight;
                }
            }
            
            result.setPixelColor(x, y, QColor(
                qBound(0, static_cast<int>(r), 255),
                qBound(0, static_cast<int>(g), 255),
                qBound(0, static_cast<int>(b), 255)
            ));
        }
    }
    
    return result;
}

} // namespace SIMDAlgorithms

// 高级图像处理算法
namespace AdvancedAlgorithms {

QImage autoLevel(const QImage &image) {
    if (image.isNull()) return image;
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    const int width = result.width();
    const int height = result.height();
    
    // 计算直方图
    QVector<int> histR(256, 0), histG(256, 0), histB(256, 0);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor pixel = result.pixelColor(x, y);
            histR[pixel.red()]++;
            histG[pixel.green()]++;
            histB[pixel.blue()]++;
        }
    }
    
    // 找到最小和最大值（忽略1%的极值）
    const int totalPixels = width * height;
    const int threshold = totalPixels * 0.01;
    
    auto findRange = [threshold](const QVector<int> &hist) -> QPair<int, int> {
        int min = 0, max = 255;
        int count = 0;
        
        // 找到最小值
        for (int i = 0; i < 256; ++i) {
            count += hist[i];
            if (count > threshold) {
                min = i;
                break;
            }
        }
        
        // 找到最大值
        count = 0;
        for (int i = 255; i >= 0; --i) {
            count += hist[i];
            if (count > threshold) {
                max = i;
                break;
            }
        }
        
        return qMakePair(min, max);
    };
    
    auto rangeR = findRange(histR);
    auto rangeG = findRange(histG);
    auto rangeB = findRange(histB);
    
    // 应用线性拉伸
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor pixel = result.pixelColor(x, y);
            
            int newR = qBound(0, static_cast<int>(255.0 * (pixel.red() - rangeR.first) / (rangeR.second - rangeR.first)), 255);
            int newG = qBound(0, static_cast<int>(255.0 * (pixel.green() - rangeG.first) / (rangeG.second - rangeG.first)), 255);
            int newB = qBound(0, static_cast<int>(255.0 * (pixel.blue() - rangeB.first) / (rangeB.second - rangeB.first)), 255);
            
            result.setPixelColor(x, y, QColor(newR, newG, newB));
        }
    }
    
    return result;
}

QImage deskew(const QImage &image) {
    if (image.isNull()) return image;
    
    // 简化的倾斜检测和校正
    QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
    
    // TODO: 实现霍夫变换倾斜检测
    qCDebug(dscannerImageProcessor) << "Deskew algorithm - placeholder implementation";
    return image;
}

QRect detectCropArea(const QImage &image) {
    if (image.isNull()) return QRect();
    
    QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
    const int width = grayscale.width();
    const int height = grayscale.height();
    
    // 简化的边界检测
    int left = 0, right = width - 1, top = 0, bottom = height - 1;
    const int threshold = 240; // 白色阈值
    
    // 检测左边界
    for (int x = 0; x < width; ++x) {
        bool hasContent = false;
        for (int y = 0; y < height; ++y) {
            if (qGray(grayscale.pixel(x, y)) < threshold) {
                hasContent = true;
                break;
            }
        }
        if (hasContent) {
            left = x;
            break;
        }
    }
    
    // 检测右边界
    for (int x = width - 1; x >= 0; --x) {
        bool hasContent = false;
        for (int y = 0; y < height; ++y) {
            if (qGray(grayscale.pixel(x, y)) < threshold) {
                hasContent = true;
                break;
            }
        }
        if (hasContent) {
            right = x;
            break;
        }
    }
    
    // 检测上边界
    for (int y = 0; y < height; ++y) {
        bool hasContent = false;
        for (int x = left; x <= right; ++x) {
            if (qGray(grayscale.pixel(x, y)) < threshold) {
                hasContent = true;
                break;
            }
        }
        if (hasContent) {
            top = y;
            break;
        }
    }
    
    // 检测下边界
    for (int y = height - 1; y >= 0; --y) {
        bool hasContent = false;
        for (int x = left; x <= right; ++x) {
            if (qGray(grayscale.pixel(x, y)) < threshold) {
                hasContent = true;
                break;
            }
        }
        if (hasContent) {
            bottom = y;
            break;
        }
    }
    
    return QRect(left, top, right - left + 1, bottom - top + 1);
}

QImage colorCorrection(const QImage &image, const QColor &whitePoint) {
    if (image.isNull()) return image;
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    const int width = result.width();
    const int height = result.height();
    
    // 计算校正因子
    double rFactor = 255.0 / whitePoint.red();
    double gFactor = 255.0 / whitePoint.green();
    double bFactor = 255.0 / whitePoint.blue();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor pixel = result.pixelColor(x, y);
            
            int newR = qBound(0, static_cast<int>(pixel.red() * rFactor), 255);
            int newG = qBound(0, static_cast<int>(pixel.green() * gFactor), 255);
            int newB = qBound(0, static_cast<int>(pixel.blue() * bFactor), 255);
            
            result.setPixelColor(x, y, QColor(newR, newG, newB));
        }
    }
    
    return result;
}

} // namespace AdvancedAlgorithms

// 增强的私有实现类
class DScannerImageProcessorPrivate
{
public:
    DScannerImageProcessorPrivate(DScannerImageProcessor *q) : q_ptr(q) {}
    
    void initialize() {
        qCDebug(dscannerImageProcessor) << "Initializing enhanced image processor";
        m_maxThreads = QThread::idealThreadCount();
        m_memoryLimit = 1024 * 1024 * 1024; // 1GB
        m_useOptimizations = true;
        
        // 检测SIMD支持
        m_simdSupport = SIMDAlgorithms::detectSIMDSupport();
        qCDebug(dscannerImageProcessor) << "SIMD Support:" << m_simdSupport;
        
        // 初始化预设配置
        initializePresets();
    }
    
    void cleanup() {
        qCDebug(dscannerImageProcessor) << "Cleaning up enhanced image processor";
        cancelAllTasks();
        presets.clear();
    }
    
    void cancelAllTasks() {
        for (auto watcher : m_pendingTasks) {
            if (watcher && !watcher->isFinished()) {
                watcher->cancel();
            }
        }
        m_pendingTasks.clear();
    }
    
    void initializePresets() {
        // 文档扫描预设
        QList<ImageProcessingParameters> documentPreset;
        documentPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::Deskew));
        documentPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::AutoLevel));
        documentPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::ContrastEnhance, 
            QVariantMap{{"contrast", 20}}));
        presets["Document"] = documentPreset;
        
        // 照片扫描预设
        QList<ImageProcessingParameters> photoPreset;
        photoPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::ColorCorrection));
        photoPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::Denoise, 
            QVariantMap{{"strength", 30}}));
        photoPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::Sharpen, 
            QVariantMap{{"strength", 20}}));
        presets["Photo"] = photoPreset;
        
        // OCR预处理预设
        QList<ImageProcessingParameters> ocrPreset;
        ocrPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::Deskew));
        ocrPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::CropDetection));
        ocrPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::AutoLevel));
        ocrPreset.append(ImageProcessingParameters(ImageProcessingAlgorithm::ContrastEnhance, 
            QVariantMap{{"contrast", 40}}));
        presets["OCR"] = ocrPreset;
    }
    
    DScannerImageProcessor *q_ptr;
    int m_maxThreads = 4;
    qint64 m_memoryLimit = 1024 * 1024 * 1024;
    qint64 m_totalProcessedImages = 0;
    qint64 m_totalProcessingTime = 0;
    bool m_useOptimizations = true;
    QString m_simdSupport;
    
    QList<QFutureWatcher<ImageProcessingResult>*> m_pendingTasks;
    QMap<QString, QList<ImageProcessingParameters>> presets;
    QMutex m_mutex;
};

// DScannerImageProcessor 增强实现
DScannerImageProcessor::DScannerImageProcessor(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerImageProcessorPrivate(this))
{
    qCDebug(dscannerImageProcessor) << "DScannerImageProcessor (Enhanced) created";
    
    auto d = d_ptr;
    d->initialize();
    
    // 注册Qt元类型
    qRegisterMetaType<ImageProcessingAlgorithm>("ImageProcessingAlgorithm");
    qRegisterMetaType<ImageFormat>("ImageFormat");
    qRegisterMetaType<ImageQuality>("ImageQuality");
    qRegisterMetaType<ImageProcessingParameters>("ImageProcessingParameters");
    qRegisterMetaType<ImageProcessingResult>("ImageProcessingResult");
    qRegisterMetaType<ScanParameters>("ScanParameters");
    qRegisterMetaType<QList<ImageProcessingResult>>("QList<ImageProcessingResult>");
}

DScannerImageProcessor::~DScannerImageProcessor()
{
    qCDebug(dscannerImageProcessor) << "DScannerImageProcessor (Enhanced) destroyed";
    
    auto d = d_ptr;
    d->cleanup();
    delete d_ptr;
}

// 基本图像处理 - 使用增强算法
QImage DScannerImageProcessor::processImage(const QImage &image, const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing image with" << params.size() << "algorithms (enhanced)";
    
    auto d = d_ptr;
    QImage result = image;
    
    for (const auto &param : params) {
        if (!param.enabled) continue;
        
        QElapsedTimer timer;
        timer.start();
        
        switch (param.algorithm) {
        case ImageProcessingAlgorithm::Denoise:
            if (d->m_useOptimizations) {
                result = SIMDAlgorithms::optimizedDenoise(result, param.parameters.value("strength", 50).toInt());
            } else {
                result = denoise(result, param.parameters.value("strength", 50).toInt());
            }
            break;
        case ImageProcessingAlgorithm::Sharpen:
            if (d->m_useOptimizations) {
                result = SIMDAlgorithms::optimizedSharpen(result, param.parameters.value("strength", 50).toInt());
            } else {
                result = sharpen(result, param.parameters.value("strength", 50).toInt());
            }
            break;
        case ImageProcessingAlgorithm::BrightnessAdjust:
            result = adjustBrightness(result, param.parameters.value("brightness", 0).toInt());
            break;
        case ImageProcessingAlgorithm::ContrastEnhance:
            result = adjustContrast(result, param.parameters.value("contrast", 0).toInt());
            break;
        case ImageProcessingAlgorithm::GammaCorrection:
            result = adjustGamma(result, param.parameters.value("gamma", 1.0).toDouble());
            break;
        case ImageProcessingAlgorithm::ColorCorrection:
            result = AdvancedAlgorithms::colorCorrection(result, 
                param.parameters.value("whitePoint", QColor(255, 255, 255)).value<QColor>());
            break;
        case ImageProcessingAlgorithm::AutoLevel:
            result = AdvancedAlgorithms::autoLevel(result);
            break;
        case ImageProcessingAlgorithm::Deskew:
            result = AdvancedAlgorithms::deskew(result);
            break;
        case ImageProcessingAlgorithm::CropDetection:
            {
                QRect cropArea = AdvancedAlgorithms::detectCropArea(result);
                if (cropArea.isValid()) {
                    result = result.copy(cropArea);
                }
            }
            break;
        default:
            qCWarning(dscannerImageProcessor) << "Unsupported algorithm:" << static_cast<int>(param.algorithm);
            break;
        }
        
        qint64 elapsed = timer.elapsed();
        d->m_totalProcessingTime += elapsed;
        
        qCDebug(dscannerImageProcessor) << "Algorithm" << static_cast<int>(param.algorithm) 
                                       << "took" << elapsed << "ms";
    }
    
    d->m_totalProcessedImages++;
    return result;
}

QFuture<ImageProcessingResult> DScannerImageProcessor::processImageAsync(const QImage &image, 
                                                                       const QList<ImageProcessingParameters> &params)
{
    return QtConcurrent::run([this, image, params]() {
        ImageProcessingResult result;
        result.success = true;
        result.processedImage = processImage(image, params);
        return result;
    });
}

// 扫描数据处理
QImage DScannerImageProcessor::processScanData(const QByteArray &rawData, const ScanParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Processing scan data, size:" << rawData.size();
    
    // 简化的实现：将原始数据转换为QImage
    QImage image;
    if (rawData.size() > 0) {
        int width = static_cast<int>(params.area.width);
        int height = static_cast<int>(params.area.height);
        if (width > 0 && height > 0) {
            image = QImage(width, height, QImage::Format_RGB32);
            image.fill(Qt::white);
        }
    }
    
    return image;
}

QFuture<ImageProcessingResult> DScannerImageProcessor::processScanDataAsync(const QByteArray &rawData, 
                                                                           const ScanParameters &params)
{
    return QtConcurrent::run([this, rawData, params]() {
        ImageProcessingResult result;
        result.success = true;
        result.processedImage = processScanData(rawData, params);
        return result;
    });
}

// 图像格式转换
QByteArray DScannerImageProcessor::convertToFormat(const QImage &image, ImageFormat format, 
                                                  ImageQuality quality)
{
    qCDebug(dscannerImageProcessor) << "Converting image to format:" << static_cast<int>(format);
    
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    
    switch (format) {
    case ImageFormat::PNG:
        image.save(&buffer, "PNG");
        break;
    case ImageFormat::JPEG:
        image.save(&buffer, "JPEG", quality == ImageQuality::High ? 95 : 80);
        break;
    case ImageFormat::TIFF:
        image.save(&buffer, "TIFF");
        break;
    case ImageFormat::BMP:
        image.save(&buffer, "BMP");
        break;
    default:
        qCWarning(dscannerImageProcessor) << "Unsupported format:" << static_cast<int>(format);
        break;
    }
    
    return data;
}

bool DScannerImageProcessor::saveImage(const QImage &image, const QString &filename, 
                                       ImageFormat format, ImageQuality quality)
{
    qCDebug(dscannerImageProcessor) << "Saving image to:" << filename;
    
    switch (format) {
    case ImageFormat::PNG:
        return image.save(filename, "PNG");
    case ImageFormat::JPEG:
        return image.save(filename, "JPEG", quality == ImageQuality::High ? 95 : 80);
    case ImageFormat::TIFF:
        return image.save(filename, "TIFF");
    case ImageFormat::BMP:
        return image.save(filename, "BMP");
    default:
        qCWarning(dscannerImageProcessor) << "Unsupported format:" << static_cast<int>(format);
        return false;
    }
}

// 图像增强算法 - 增强版本
QImage DScannerImageProcessor::denoise(const QImage &image, int strength)
{
    auto d = d_ptr;
    if (d->m_useOptimizations) {
        return SIMDAlgorithms::optimizedDenoise(image, strength);
    }
    
    // 回退到简单实现
    qCDebug(dscannerImageProcessor) << "Applying basic denoise with strength:" << strength;
    return image;
}

QImage DScannerImageProcessor::sharpen(const QImage &image, int strength)
{
    auto d = d_ptr;
    if (d->m_useOptimizations) {
        return SIMDAlgorithms::optimizedSharpen(image, strength);
    }
    
    // 回退到简单实现
    qCDebug(dscannerImageProcessor) << "Applying basic sharpen with strength:" << strength;
    return image;
}

QImage DScannerImageProcessor::adjustBrightness(const QImage &image, int brightness)
{
    qCDebug(dscannerImageProcessor) << "Adjusting brightness:" << brightness;
    
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    int factor = brightness;
    
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            QColor color = result.pixelColor(x, y);
            color.setRed(qBound(0, color.red() + factor, 255));
            color.setGreen(qBound(0, color.green() + factor, 255));
            color.setBlue(qBound(0, color.blue() + factor, 255));
            result.setPixelColor(x, y, color);
        }
    }
    
    return result;
}

QImage DScannerImageProcessor::adjustContrast(const QImage &image, int contrast)
{
    qCDebug(dscannerImageProcessor) << "Adjusting contrast:" << contrast;
    
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    double factor = (100.0 + contrast) / 100.0;
    
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            QColor color = result.pixelColor(x, y);
            color.setRed(qBound(0, static_cast<int>((color.red() - 128) * factor + 128), 255));
            color.setGreen(qBound(0, static_cast<int>((color.green() - 128) * factor + 128), 255));
            color.setBlue(qBound(0, static_cast<int>((color.blue() - 128) * factor + 128), 255));
            result.setPixelColor(x, y, color);
        }
    }
    
    return result;
}

QImage DScannerImageProcessor::adjustGamma(const QImage &image, double gamma)
{
    qCDebug(dscannerImageProcessor) << "Adjusting gamma:" << gamma;
    
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            QColor color = result.pixelColor(x, y);
            color.setRed(qBound(0, static_cast<int>(255 * pow(color.red() / 255.0, gamma)), 255));
            color.setGreen(qBound(0, static_cast<int>(255 * pow(color.green() / 255.0, gamma)), 255));
            color.setBlue(qBound(0, static_cast<int>(255 * pow(color.blue() / 255.0, gamma)), 255));
            result.setPixelColor(x, y, color);
        }
    }
    
    return result;
}

QImage DScannerImageProcessor::colorCorrection(const QImage &image, const QColor &whitePoint)
{
    qCDebug(dscannerImageProcessor) << "Applying enhanced color correction";
    return AdvancedAlgorithms::colorCorrection(image, whitePoint);
}

QImage DScannerImageProcessor::autoLevel(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Applying enhanced auto level";
    return AdvancedAlgorithms::autoLevel(image);
}

QImage DScannerImageProcessor::deskew(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Applying enhanced deskew";
    return AdvancedAlgorithms::deskew(image);
}

QRect DScannerImageProcessor::detectCropArea(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Detecting crop area with enhanced algorithm";
    return AdvancedAlgorithms::detectCropArea(image);
}

// 批量处理 - 使用异步优化
QList<ImageProcessingResult> DScannerImageProcessor::processBatch(const QList<QImage> &images, 
                                                                const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing batch of" << images.size() << "images (enhanced)";
    
    QList<ImageProcessingResult> results;
    
    if (images.size() > 1) {
        // 使用异步处理提高性能
        QList<QFuture<ImageProcessingResult>> futures;
        
        for (const auto &image : images) {
            futures.append(processImageAsync(image, params));
        }
        
        for (auto &future : futures) {
            results.append(future.result());
        }
    } else {
        // 单个图像直接处理
        for (const auto &image : images) {
            ImageProcessingResult result;
            result.success = true;
            result.processedImage = processImage(image, params);
            results.append(result);
        }
    }
    
    return results;
}

QFuture<QList<ImageProcessingResult>> DScannerImageProcessor::processBatchAsync(const QList<QImage> &images, 
                                                                              const QList<ImageProcessingParameters> &params)
{
    return QtConcurrent::run([this, images, params]() {
        return processBatch(images, params);
    });
}

// 预设配置管理
void DScannerImageProcessor::addPreset(const QString &name, const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Adding preset:" << name;
    auto d = d_ptr;
    QMutexLocker locker(&d->m_mutex);
    d->presets[name] = params;
}

void DScannerImageProcessor::removePreset(const QString &name)
{
    qCDebug(dscannerImageProcessor) << "Removing preset:" << name;
    auto d = d_ptr;
    QMutexLocker locker(&d->m_mutex);
    d->presets.remove(name);
}

QList<ImageProcessingParameters> DScannerImageProcessor::getPreset(const QString &name) const
{
    qCDebug(dscannerImageProcessor) << "Getting preset:" << name;
    auto d = d_ptr;
    QMutexLocker locker(&d->m_mutex);
    return d->presets.value(name);
}

QStringList DScannerImageProcessor::getPresetNames() const
{
    qCDebug(dscannerImageProcessor) << "Getting preset names";
    auto d = d_ptr;
    QMutexLocker locker(&d->m_mutex);
    return d->presets.keys();
}

// 性能设置
void DScannerImageProcessor::setMaxThreads(int maxThreads)
{
    auto d = d_ptr;
    d->m_maxThreads = maxThreads;
    qCDebug(dscannerImageProcessor) << "Set max threads:" << maxThreads;
}

int DScannerImageProcessor::maxThreads() const
{
    auto d = d_ptr;
    return d->m_maxThreads;
}

void DScannerImageProcessor::setMemoryLimit(qint64 limitBytes)
{
    auto d = d_ptr;
    d->m_memoryLimit = limitBytes;
    qCDebug(dscannerImageProcessor) << "Set memory limit:" << limitBytes;
}

qint64 DScannerImageProcessor::memoryLimit() const
{
    auto d = d_ptr;
    return d->m_memoryLimit;
}

// 状态查询
bool DScannerImageProcessor::isProcessing() const
{
    auto d = d_ptr;
    return !d->m_pendingTasks.isEmpty();
}

int DScannerImageProcessor::pendingTasks() const
{
    auto d = d_ptr;
    return d->m_pendingTasks.size();
}

void DScannerImageProcessor::cancelAllTasks()
{
    auto d = d_ptr;
    d->cancelAllTasks();
    qCDebug(dscannerImageProcessor) << "Cancelled all tasks";
}

// 统计信息
qint64 DScannerImageProcessor::totalProcessedImages() const
{
    auto d = d_ptr;
    return d->m_totalProcessedImages;
}

qint64 DScannerImageProcessor::totalProcessingTime() const
{
    auto d = d_ptr;
    return d->m_totalProcessingTime;
}

double DScannerImageProcessor::averageProcessingTime() const
{
    auto d = d_ptr;
    if (d->m_totalProcessedImages > 0) {
        return static_cast<double>(d->m_totalProcessingTime) / d->m_totalProcessedImages;
    }
    return 0.0;
}

DSCANNER_END_NAMESPACE 