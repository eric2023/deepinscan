// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dscannerimageprocessor_p.h"

#include <QBuffer>
#include <QImageWriter>
#include <QImageReader>
#include <QDebug>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QPrinter>
#include <QPageSize>
#include <QTextDocument>

DSCANNER_BEGIN_NAMESPACE

// ImageFormatHandler 实现

QByteArray ImageFormatHandler::convertToFormat(const QImage &image, ImageFormat format, ImageQuality quality)
{
    if (image.isNull()) {
        return QByteArray();
    }
    
    switch (format) {
    case ImageFormat::PNG:
        return saveAsPNG(image, quality);
    case ImageFormat::JPEG:
        return saveAsJPEG(image, quality);
    case ImageFormat::TIFF:
        return saveAsTIFF(image, quality);
    case ImageFormat::PDF:
        return saveAsPDF(image, quality);
    case ImageFormat::BMP:
        return saveAsBMP(image, quality);
    case ImageFormat::RAW:
        // RAW格式通常不用于输出
        return saveAsPNG(image, quality);
    default:
        return saveAsPNG(image, quality);
    }
}

QByteArray ImageFormatHandler::saveAsPNG(const QImage &image, ImageQuality quality)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    
    QImageWriter writer(&buffer, "PNG");
    
    // PNG是无损格式，质量设置影响压缩级别
    int compressionLevel = 6; // 默认压缩级别
    switch (quality) {
    case ImageQuality::Low:
        compressionLevel = 9; // 最高压缩
        break;
    case ImageQuality::Medium:
        compressionLevel = 6;
        break;
    case ImageQuality::High:
        compressionLevel = 3;
        break;
    case ImageQuality::Lossless:
        compressionLevel = 0; // 无压缩
        break;
    }
    
    writer.setCompression(compressionLevel);
    
    if (!writer.write(image)) {
        qCWarning(dscannerImageProcessor) << "Failed to write PNG image:" << writer.errorString();
        return QByteArray();
    }
    
    return data;
}

QByteArray ImageFormatHandler::saveAsJPEG(const QImage &image, ImageQuality quality)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    
    QImageWriter writer(&buffer, "JPEG");
    
    int jpegQuality = qualityToInt(quality);
    writer.setQuality(jpegQuality);
    
    // JPEG不支持透明度，转换为RGB
    QImage rgbImage = image;
    if (image.hasAlphaChannel()) {
        rgbImage = QImage(image.size(), QImage::Format_RGB32);
        rgbImage.fill(Qt::white);
        QPainter painter(&rgbImage);
        painter.drawImage(0, 0, image);
    }
    
    if (!writer.write(rgbImage)) {
        qCWarning(dscannerImageProcessor) << "Failed to write JPEG image:" << writer.errorString();
        return QByteArray();
    }
    
    return data;
}

QByteArray ImageFormatHandler::saveAsTIFF(const QImage &image, ImageQuality quality)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    
    QImageWriter writer(&buffer, "TIFF");
    
    // TIFF支持无损和有损压缩
    if (quality == ImageQuality::Lossless) {
        writer.setCompression(0); // 无压缩
    } else {
        writer.setCompression(1); // LZW压缩
    }
    
    if (!writer.write(image)) {
        qCWarning(dscannerImageProcessor) << "Failed to write TIFF image:" << writer.errorString();
        return QByteArray();
    }
    
    return data;
}

QByteArray ImageFormatHandler::saveAsPDF(const QImage &image, ImageQuality quality)
{
    return createPDFFromImage(image, quality);
}

QByteArray ImageFormatHandler::saveAsBMP(const QImage &image, ImageQuality quality)
{
    Q_UNUSED(quality) // BMP是无损格式
    
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    
    QImageWriter writer(&buffer, "BMP");
    
    if (!writer.write(image)) {
        qCWarning(dscannerImageProcessor) << "Failed to write BMP image:" << writer.errorString();
        return QByteArray();
    }
    
    return data;
}

QImage ImageFormatHandler::loadFromData(const QByteArray &data, ImageFormat format)
{
    if (data.isEmpty()) {
        return QImage();
    }
    
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    
    QImageReader reader(&buffer);
    
    if (format != ImageFormat::Unknown) {
        reader.setFormat(formatToString(format).toUtf8());
    } else {
        // 自动检测格式
        reader.setAutoDetectImageFormat(true);
    }
    
    QImage image = reader.read();
    if (image.isNull()) {
        qCWarning(dscannerImageProcessor) << "Failed to load image:" << reader.errorString();
    }
    
    return image;
}

ImageFormat ImageFormatHandler::detectFormat(const QByteArray &data)
{
    if (data.isEmpty()) {
        return ImageFormat::Unknown;
    }
    
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    
    QImageReader reader(&buffer);
    QByteArray format = reader.format();
    
    if (format == "PNG") {
        return ImageFormat::PNG;
    } else if (format == "JPEG") {
        return ImageFormat::JPEG;
    } else if (format == "TIFF") {
        return ImageFormat::TIFF;
    } else if (format == "BMP") {
        return ImageFormat::BMP;
    } else if (format == "PDF") {
        return ImageFormat::PDF;
    }
    
    return ImageFormat::Unknown;
}

QString ImageFormatHandler::formatToString(ImageFormat format)
{
    switch (format) {
    case ImageFormat::PNG:
        return "PNG";
    case ImageFormat::JPEG:
        return "JPEG";
    case ImageFormat::TIFF:
        return "TIFF";
    case ImageFormat::PDF:
        return "PDF";
    case ImageFormat::BMP:
        return "BMP";
    case ImageFormat::RAW:
        return "RAW";
    default:
        return "Unknown";
    }
}

QString ImageFormatHandler::formatToMimeType(ImageFormat format)
{
    switch (format) {
    case ImageFormat::PNG:
        return "image/png";
    case ImageFormat::JPEG:
        return "image/jpeg";
    case ImageFormat::TIFF:
        return "image/tiff";
    case ImageFormat::PDF:
        return "application/pdf";
    case ImageFormat::BMP:
        return "image/bmp";
    case ImageFormat::RAW:
        return "image/raw";
    default:
        return "application/octet-stream";
    }
}

int ImageFormatHandler::qualityToInt(ImageQuality quality)
{
    switch (quality) {
    case ImageQuality::Low:
        return 25;
    case ImageQuality::Medium:
        return 50;
    case ImageQuality::High:
        return 85;
    case ImageQuality::Lossless:
        return 100;
    default:
        return 85;
    }
}

QByteArray ImageFormatHandler::createPDFFromImage(const QImage &image, ImageQuality quality)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    
    QPrinter printer;
    printer.setOutputDevice(&buffer);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(0, 0, 0, 0));
    
    // 根据质量设置分辨率
    int resolution = 300; // 默认300 DPI
    switch (quality) {
    case ImageQuality::Low:
        resolution = 150;
        break;
    case ImageQuality::Medium:
        resolution = 300;
        break;
    case ImageQuality::High:
        resolution = 600;
        break;
    case ImageQuality::Lossless:
        resolution = 1200;
        break;
    }
    
    printer.setResolution(resolution);
    
    QPainter painter(&printer);
    if (!painter.isActive()) {
        qCWarning(dscannerImageProcessor) << "Failed to create PDF painter";
        return QByteArray();
    }
    
    // 计算图像在页面中的位置和大小
    QRect pageRect = printer.pageRect(QPrinter::DevicePixel);
    QSize imageSize = image.size();
    
    // 保持宽高比缩放
    imageSize.scale(pageRect.size(), Qt::KeepAspectRatio);
    
    // 居中显示
    int x = (pageRect.width() - imageSize.width()) / 2;
    int y = (pageRect.height() - imageSize.height()) / 2;
    
    QRect targetRect(x, y, imageSize.width(), imageSize.height());
    
    painter.drawImage(targetRect, image);
    painter.end();
    
    return data;
}

DSCANNER_END_NAMESPACE 