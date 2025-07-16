// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dscannerimageprocessor_p.h"

#include <QDebug>
#include <QColor>
#include <QRgb>
#include <QPainter>

#include <algorithm>
#include <cmath>

DSCANNER_BEGIN_NAMESPACE

// ScanDataProcessor 实现

QImage ScanDataProcessor::processScanData(const QByteArray &rawData, const ScanParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Processing scan data, size:" << rawData.size() 
                                   << "resolution:" << params.resolution 
                                   << "color mode:" << static_cast<int>(params.colorMode);
    
    if (rawData.isEmpty()) {
        qCWarning(dscannerImageProcessor) << "Raw data is empty";
        return QImage();
    }
    
    // 转换原始数据为图像
    QImage image = convertRawToImage(rawData, params);
    if (image.isNull()) {
        qCWarning(dscannerImageProcessor) << "Failed to convert raw data to image";
        return QImage();
    }
    
    // 应用自动校正
    if (params.autoColorCorrection || params.autoContrast) {
        image = applyAutoCorrections(image, params);
    }
    
    // 应用扫描模式优化
    image = applyScanModeOptimizations(image, params.scanMode);
    
    // 自动检测和裁剪
    if (params.autoDetectSize) {
        image = autoDetectAndCrop(image);
    }
    
    return image;
}

QImage ScanDataProcessor::convertRawToImage(const QByteArray &rawData, const ScanParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Converting raw data to image";
    
    // 计算图像尺寸
    QSize imageSize;
    if (params.scanArea.isValid()) {
        imageSize = params.scanArea.size();
    } else {
        // 根据分辨率和数据大小估算尺寸
        int bytesPerPixel = 1;
        switch (params.colorMode) {
        case ColorMode::Grayscale:
            bytesPerPixel = 1;
            break;
        case ColorMode::RGB24:
            bytesPerPixel = 3;
            break;
        case ColorMode::RGBA32:
            bytesPerPixel = 4;
            break;
        default:
            bytesPerPixel = 3;
            break;
        }
        
        int totalPixels = rawData.size() / bytesPerPixel;
        int width = static_cast<int>(std::sqrt(totalPixels));
        int height = totalPixels / width;
        
        imageSize = QSize(width, height);
    }
    
    // 根据颜色模式转换数据
    switch (params.colorMode) {
    case ColorMode::Grayscale:
        return convertGrayscale(rawData, imageSize);
    case ColorMode::RGB24:
        return convertRGB(rawData, imageSize);
    case ColorMode::RGBA32:
        return convertRGB(rawData, imageSize); // 简化处理
    default:
        return convertRGB(rawData, imageSize);
    }
}

QImage ScanDataProcessor::convertGrayscale(const QByteArray &data, const QSize &size)
{
    if (data.size() < size.width() * size.height()) {
        qCWarning(dscannerImageProcessor) << "Insufficient data for grayscale conversion";
        return QImage();
    }
    
    QImage image(size, QImage::Format_Grayscale8);
    const uchar *srcData = reinterpret_cast<const uchar*>(data.constData());
    
    for (int y = 0; y < size.height(); y++) {
        uchar *line = image.scanLine(y);
        for (int x = 0; x < size.width(); x++) {
            line[x] = srcData[y * size.width() + x];
        }
    }
    
    return image;
}

QImage ScanDataProcessor::convertRGB(const QByteArray &data, const QSize &size)
{
    if (data.size() < size.width() * size.height() * 3) {
        qCWarning(dscannerImageProcessor) << "Insufficient data for RGB conversion";
        return QImage();
    }
    
    QImage image(size, QImage::Format_RGB32);
    const uchar *srcData = reinterpret_cast<const uchar*>(data.constData());
    
    for (int y = 0; y < size.height(); y++) {
        for (int x = 0; x < size.width(); x++) {
            int index = (y * size.width() + x) * 3;
            uchar r = srcData[index];
            uchar g = srcData[index + 1];
            uchar b = srcData[index + 2];
            image.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return image;
}

QImage ScanDataProcessor::convertCMYK(const QByteArray &data, const QSize &size)
{
    if (data.size() < size.width() * size.height() * 4) {
        qCWarning(dscannerImageProcessor) << "Insufficient data for CMYK conversion";
        return QImage();
    }
    
    QImage image(size, QImage::Format_RGB32);
    const uchar *srcData = reinterpret_cast<const uchar*>(data.constData());
    
    for (int y = 0; y < size.height(); y++) {
        for (int x = 0; x < size.width(); x++) {
            int index = (y * size.width() + x) * 4;
            double c = srcData[index] / 255.0;
            double m = srcData[index + 1] / 255.0;
            double y_cmyk = srcData[index + 2] / 255.0;
            double k = srcData[index + 3] / 255.0;
            
            // CMYK to RGB conversion
            double r = 255 * (1 - c) * (1 - k);
            double g = 255 * (1 - m) * (1 - k);
            double b = 255 * (1 - y_cmyk) * (1 - k);
            
            image.setPixel(x, y, qRgb(
                static_cast<int>(qBound(0.0, r, 255.0)),
                static_cast<int>(qBound(0.0, g, 255.0)),
                static_cast<int>(qBound(0.0, b, 255.0))
            ));
        }
    }
    
    return image;
}

QImage ScanDataProcessor::applyAutoCorrections(const QImage &image, const ScanParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Applying auto corrections";
    
    QImage result = image;
    
    // 自动色彩校正
    if (params.autoColorCorrection) {
        result = ImageAlgorithms::autoLevel(result);
    }
    
    // 自动对比度
    if (params.autoContrast) {
        result = ImageAlgorithms::histogramEqualization(result);
    }
    
    // 应用用户设置的调整
    if (params.brightness != 0.0) {
        result = ImageAlgorithms::adjustBrightness(result, static_cast<int>(params.brightness));
    }
    
    if (params.contrast != 0.0) {
        result = ImageAlgorithms::adjustContrast(result, static_cast<int>(params.contrast));
    }
    
    if (params.gamma != 1.0) {
        result = ImageAlgorithms::adjustGamma(result, params.gamma);
    }
    
    return result;
}

QImage ScanDataProcessor::applyScanModeOptimizations(const QImage &image, ScanMode mode)
{
    qCDebug(dscannerImageProcessor) << "Applying scan mode optimizations for mode:" << static_cast<int>(mode);
    
    switch (mode) {
    case ScanMode::Text:
        return optimizeForText(image);
    case ScanMode::Photo:
        return optimizeForPhoto(image);
    case ScanMode::Mixed:
        // 混合模式使用平衡的设置
        {
            QImage result = ImageAlgorithms::adjustContrast(image, 10);
            result = ImageAlgorithms::sharpen(result, 30);
            return result;
        }
    default:
        return image;
    }
}

QImage ScanDataProcessor::autoDetectAndCrop(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Auto detecting and cropping image";
    
    QRect cropArea = ImageAlgorithms::detectCropArea(image);
    
    if (cropArea.isValid() && !cropArea.isEmpty()) {
        // 添加一些边距
        int margin = 10;
        cropArea.adjust(-margin, -margin, margin, margin);
        
        // 确保裁剪区域在图像范围内
        cropArea = cropArea.intersected(image.rect());
        
        if (cropArea.isValid() && !cropArea.isEmpty()) {
            return image.copy(cropArea);
        }
    }
    
    return image;
}

QImage ScanDataProcessor::optimizeForText(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Optimizing for text scanning";
    
    QImage result = image;
    
    // 文本优化：增强对比度和锐化
    result = ImageAlgorithms::adjustContrast(result, 20);
    result = ImageAlgorithms::sharpen(result, 40);
    
    // 去噪以清理文本
    result = ImageAlgorithms::denoise(result, 20);
    
    // 倾斜校正
    result = ImageAlgorithms::deskew(result);
    
    // 如果是彩色图像，可以考虑转换为灰度以提高文本识别效果
    if (result.format() != QImage::Format_Grayscale8) {
        // 这里保持原格式，用户可以后续选择转换
    }
    
    return result;
}

QImage ScanDataProcessor::optimizeForPhoto(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Optimizing for photo scanning";
    
    QImage result = image;
    
    // 照片优化：色彩校正和轻微锐化
    result = ImageAlgorithms::colorCorrection(result, QColor(240, 240, 240));
    result = ImageAlgorithms::adjustGamma(result, 1.1);
    
    // 轻微锐化以增强细节
    result = ImageAlgorithms::sharpen(result, 20);
    
    // 轻微去噪以减少颗粒感
    result = ImageAlgorithms::denoise(result, 15);
    
    return result;
}

DSCANNER_END_NAMESPACE 