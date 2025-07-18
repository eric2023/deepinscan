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
#include <QPainter>
#include <QTransform>
#include <QVector>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include "simple_simd_support.h"
#include <QFutureWatcher>
#include <QTimer>
#include <QDebug>

DSCANNER_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(dscannerImageProcessor, "deepinscan.imageprocessor")

// 私有类已在头文件中定义

// DScannerImageProcessor 实现
DScannerImageProcessor::DScannerImageProcessor(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerImageProcessorPrivate(this))
{
    qCDebug(dscannerImageProcessor) << "DScannerImageProcessor created";
    
    auto d = d_ptr;
    d->initialize();
    
    // 加载预设
    loadPresetsFromFile();
    
    // 检测SIMD支持
    QString simdInfo = SimpleSIMDSupport::detectSIMDSupport();
    qCDebug(dscannerImageProcessor) << "SIMD Support:" << simdInfo;
    
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
    qCDebug(dscannerImageProcessor) << "DScannerImageProcessor destroyed";
    
    auto d = d_ptr;
    d->cleanup();
    delete d_ptr;
}

// 基本图像处理
QImage DScannerImageProcessor::processImage(const QImage &image, const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing image with" << params.size() << "algorithms";
    
    // 记录开始时间
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    
    QImage result = image;
    
    for (const auto &param : params) {
        if (!param.enabled) continue;
        
        switch (param.algorithm) {
        case ImageProcessingAlgorithm::Denoise:
            result = denoise(result, param.parameters.value("strength", 50).toInt());
            break;
        case ImageProcessingAlgorithm::Sharpen:
            result = sharpen(result, param.parameters.value("strength", 50).toInt());
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
            result = colorCorrection(result, param.parameters.value("whitePoint", QColor(255, 255, 255)).value<QColor>());
            break;
        case ImageProcessingAlgorithm::AutoLevel:
            result = autoLevel(result);
            break;
        case ImageProcessingAlgorithm::Deskew:
            result = deskew(result);
            break;
        case ImageProcessingAlgorithm::CropDetection:
            {
                QRect cropArea = detectCropArea(result);
                if (cropArea.isValid() && cropArea != result.rect()) {
                    result = result.copy(cropArea);
                }
            }
            break;
        default:
            qCWarning(dscannerImageProcessor) << "Unsupported algorithm:" << static_cast<int>(param.algorithm);
            break;
        }
    }
    
    // 更新性能统计
    qint64 endTime = QDateTime::currentMSecsSinceEpoch();
    auto d = d_ptr;
    d->m_totalProcessedImages++;
    d->m_totalProcessingTime += (endTime - startTime);
    
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
    // 这里应该根据实际的扫描数据格式进行解析
    QImage image;
    if (rawData.size() > 0) {
        // 创建一个简单的测试图像
        image = QImage(static_cast<int>(params.area.width), static_cast<int>(params.area.height), QImage::Format_RGB32);
        image.fill(Qt::white);
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

// 图像增强算法
QImage DScannerImageProcessor::denoise(const QImage &image, int strength)
{
    qCDebug(dscannerImageProcessor) << "Applying denoise with strength:" << strength;
    
    // 转换强度为高斯模糊半径
    int radius = qBound(1, strength / 10, 20); // 将0-100转换为1-20的半径
    
    // 暂时不使用SIMD去噪，因为gaussianBlurSIMD有依赖问题
    // 后续可以实现一个简单的SIMD去噪算法
    qCDebug(dscannerImageProcessor) << "Denoising not fully implemented yet";
    
    // 回退到简单实现
    qCDebug(dscannerImageProcessor) << "Using standard denoising";
    return image; // 简化实现，直接返回原图
}

QImage DScannerImageProcessor::sharpen(const QImage &image, int strength)
{
    qCDebug(dscannerImageProcessor) << "Applying sharpen with strength:" << strength;
    
    // 转换强度为锐化因子
    double sharpnessFactor = 1.0 + (strength / 100.0); // 将0-100转换为1.0-2.0
    sharpnessFactor = qBound(0.1, sharpnessFactor, 2.0);
    
    // 暂时不使用SIMD锐化，因为sharpenSIMD函数未完全实现
    // 后续可以实现一个简单的SIMD锐化算法
    qCDebug(dscannerImageProcessor) << "Sharpening not fully implemented yet";
    
    // 回退到简单实现
    qCDebug(dscannerImageProcessor) << "Using standard sharpening";
    return image; // 简化实现，直接返回原图
}

QImage DScannerImageProcessor::adjustBrightness(const QImage &image, int brightness)
{
    qCDebug(dscannerImageProcessor) << "Adjusting brightness:" << brightness;
    
    // 转换亮度值为因子格式 (SIMD函数使用0.1-3.0的因子)
    double brightnessFactor = 1.0 + (brightness / 100.0); // -100到+100转换为0.0到2.0
    brightnessFactor = qBound(0.1, brightnessFactor, 3.0);
    
    // 尝试使用SIMD优化版本
    if (SimpleSIMDSupport::hasSSE2Support() || SimpleSIMDSupport::hasAVX2Support()) {
        qCDebug(dscannerImageProcessor) << "Using SIMD optimized brightness adjustment";
        return SimpleSIMDSupport::adjustBrightnessSIMD(image, brightnessFactor);
    }
    
    // 回退到标准实现
    qCDebug(dscannerImageProcessor) << "Using standard brightness adjustment";
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
    
    // 转换对比度值为因子格式 (SIMD函数使用0.1-3.0的因子)
    double contrastFactor = (100.0 + contrast) / 100.0;
    contrastFactor = qBound(0.1, contrastFactor, 3.0);
    
    // 尝试使用SIMD优化版本
    if (SimpleSIMDSupport::hasSSE2Support() || SimpleSIMDSupport::hasAVX2Support()) {
        qCDebug(dscannerImageProcessor) << "Using SIMD optimized contrast adjustment";
        return SimpleSIMDSupport::adjustContrastSIMD(image, contrastFactor);
    }
    
    // 回退到标准实现
    qCDebug(dscannerImageProcessor) << "Using standard contrast adjustment";
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
    qCDebug(dscannerImageProcessor) << "Applying color correction";
    
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

QImage DScannerImageProcessor::autoLevel(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Applying auto level";
    
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

QImage DScannerImageProcessor::deskew(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Applying deskew";
    
    if (image.isNull()) return image;
    
    // 转换为灰度图像以便于边缘检测
    QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
    const int width = grayscale.width();
    const int height = grayscale.height();
    
    // 使用Sobel算子进行边缘检测
    QImage edgeImage(width, height, QImage::Format_Grayscale8);
    edgeImage.fill(Qt::black);
    
    // Sobel核
    const int sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int gx = 0, gy = 0;
            
            // 计算Sobel梯度
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int pixel = qGray(grayscale.pixel(x + dx, y + dy));
                    gx += pixel * sobelX[dy + 1][dx + 1];
                    gy += pixel * sobelY[dy + 1][dx + 1];
                }
            }
            
            int magnitude = qSqrt(gx * gx + gy * gy);
            edgeImage.setPixel(x, y, qRgb(magnitude, magnitude, magnitude));
        }
    }
    
    // 使用Hough变换检测直线来确定倾斜角度
    QVector<double> angles;
    const int threshold = 128; // 边缘阈值
    
    // 简化的角度检测：检查不同角度的直线投影
    const double angleRange = 10.0; // 检测±10度
    const double angleStep = 0.5;   // 0.5度步长
    
    double bestAngle = 0.0;
    int maxScore = 0;
    
    for (double angle = -angleRange; angle <= angleRange; angle += angleStep) {
        int score = 0;
        double radians = angle * M_PI / 180.0;
        double cosA = cos(radians);
        double sinA = sin(radians);
        
        // 投影线检测
        for (int y = height / 4; y < 3 * height / 4; y += 5) { // 取样检测
            for (int x = 0; x < width; ++x) {
                if (qGray(edgeImage.pixel(x, y)) > threshold) {
                    // 沿着角度方向计算投影强度
                    int projectedX = x + static_cast<int>(50 * cosA);
                    int projectedY = y + static_cast<int>(50 * sinA);
                    
                    if (projectedX >= 0 && projectedX < width && 
                        projectedY >= 0 && projectedY < height &&
                        qGray(edgeImage.pixel(projectedX, projectedY)) > threshold) {
                        score++;
                    }
                }
            }
        }
        
        if (score > maxScore) {
            maxScore = score;
            bestAngle = angle;
        }
    }
    
    qCDebug(dscannerImageProcessor) << "Detected skew angle:" << bestAngle << "degrees";
    
    // 如果检测到的角度太小，不进行旋转
    if (qAbs(bestAngle) < 0.1) {
        return image;
    }
    
    // 使用QTransform进行图像旋转校正
    QTransform transform;
    transform.translate(width / 2.0, height / 2.0);
    transform.rotate(-bestAngle); // 负角度校正倾斜
    transform.translate(-width / 2.0, -height / 2.0);
    
    // 计算旋转后的边界框
    QRectF boundingRect = transform.mapRect(QRectF(image.rect()));
    
    // 创建新的图像
    QImage result(boundingRect.size().toSize(), image.format());
    result.fill(Qt::white);
    
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    // 调整变换矩阵以适应新的边界
    QTransform adjustedTransform;
    adjustedTransform.translate(-boundingRect.x(), -boundingRect.y());
    adjustedTransform *= transform;
    
    painter.setTransform(adjustedTransform);
    painter.drawImage(0, 0, image);
    painter.end();
    
    qCDebug(dscannerImageProcessor) << "Deskew completed, angle:" << bestAngle << "degrees";
    
    return result;
}

QRect DScannerImageProcessor::detectCropArea(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Detecting crop area";
    
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

// 批量处理
QList<ImageProcessingResult> DScannerImageProcessor::processBatch(const QList<QImage> &images, 
                                                                const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing batch of" << images.size() << "images";
    
    QList<ImageProcessingResult> results;
    for (const auto &image : images) {
        ImageProcessingResult result;
        result.success = true;
        result.processedImage = processImage(image, params);
        results.append(result);
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

// 预设配置
void DScannerImageProcessor::addPreset(const QString &name, const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Adding preset:" << name << "with" << params.size() << "parameters";
    
    auto d = d_ptr;
    QMutexLocker locker(&d->presetMutex);
    
    // 保存预设到内存
    d->presets[name] = params;
    
    // 同时保存到磁盘
    savePresetsToFile();
    
    qCDebug(dscannerImageProcessor) << "Preset" << name << "added successfully";
}

void DScannerImageProcessor::removePreset(const QString &name)
{
    qCDebug(dscannerImageProcessor) << "Removing preset:" << name;
    
    auto d = d_ptr;
    QMutexLocker locker(&d->presetMutex);
    
    if (d->presets.remove(name) > 0) {
        // 保存更新后的预设到磁盘
        savePresetsToFile();
        qCDebug(dscannerImageProcessor) << "Preset" << name << "removed successfully";
    } else {
        qCWarning(dscannerImageProcessor) << "Preset" << name << "not found";
    }
}

QList<ImageProcessingParameters> DScannerImageProcessor::getPreset(const QString &name) const
{
    qCDebug(dscannerImageProcessor) << "Getting preset:" << name;
    
    auto d = d_ptr;
    QMutexLocker locker(&d->presetMutex);
    
    return d->presets.value(name, QList<ImageProcessingParameters>());
}

QStringList DScannerImageProcessor::getPresetNames() const
{
    qCDebug(dscannerImageProcessor) << "Getting preset names";
    
    auto d = d_ptr;
    QMutexLocker locker(&d->presetMutex);
    
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

void DScannerImageProcessor::savePresetsToFile() const
{
    auto d = d_ptr;
    
    // 获取用户配置目录
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir dir(configDir);
    if (!dir.exists("deepinscan")) {
        dir.mkpath("deepinscan");
    }
    
    QString presetFilePath = configDir + "/deepinscan/presets.json";
    QFile file(presetFilePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(dscannerImageProcessor) << "Failed to save presets to file:" << presetFilePath;
        return;
    }
    
    // 将预设转换为JSON格式
    QJsonDocument doc;
    QJsonObject presetsObj;
    
    for (auto it = d->presets.begin(); it != d->presets.end(); ++it) {
        QJsonArray paramsArray;
        for (const auto &param : it.value()) {
            QJsonObject paramObj;
            paramObj["algorithm"] = static_cast<int>(param.algorithm);
            paramObj["enabled"] = param.enabled;
            
            QJsonObject parametersObj;
            for (auto paramIt = param.parameters.begin(); paramIt != param.parameters.end(); ++paramIt) {
                parametersObj[paramIt.key()] = QJsonValue::fromVariant(paramIt.value());
            }
            paramObj["parameters"] = parametersObj;
            
            paramsArray.append(paramObj);
        }
        presetsObj[it.key()] = paramsArray;
    }
    
    doc.setObject(presetsObj);
    file.write(doc.toJson());
    file.close();
    
    qCDebug(dscannerImageProcessor) << "Presets saved to:" << presetFilePath;
}

void DScannerImageProcessor::loadPresetsFromFile()
{
    auto d = d_ptr;
    QMutexLocker locker(&d->presetMutex);
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString presetFilePath = configDir + "/deepinscan/presets.json";
    
    QFile file(presetFilePath);
    if (!file.exists()) {
        qCDebug(dscannerImageProcessor) << "No preset file found, starting with empty presets";
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(dscannerImageProcessor) << "Failed to load presets from file:" << presetFilePath;
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dscannerImageProcessor) << "JSON parse error:" << error.errorString();
        return;
    }
    
    QJsonObject presetsObj = doc.object();
    d->presets.clear();
    
    for (auto it = presetsObj.begin(); it != presetsObj.end(); ++it) {
        QJsonArray paramsArray = it.value().toArray();
        QList<ImageProcessingParameters> paramsList;
        
        for (const auto &value : paramsArray) {
            QJsonObject paramObj = value.toObject();
            
            ImageProcessingParameters param;
            param.algorithm = static_cast<ImageProcessingAlgorithm>(paramObj["algorithm"].toInt());
            param.enabled = paramObj["enabled"].toBool();
            
            QJsonObject parametersObj = paramObj["parameters"].toObject();
            for (auto paramIt = parametersObj.begin(); paramIt != parametersObj.end(); ++paramIt) {
                param.parameters[paramIt.key()] = paramIt.value().toVariant();
            }
            
            paramsList.append(param);
        }
        
        d->presets[it.key()] = paramsList;
    }
    
    qCDebug(dscannerImageProcessor) << "Loaded" << d->presets.size() << "presets from:" << presetFilePath;
}

DSCANNER_END_NAMESPACE 