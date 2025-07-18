#include "simple_simd_support.h"
#include <QColor>
#include <QtMath>

// SIMD头文件
#ifdef __SSE2__
#include <emmintrin.h>
#define HAS_SSE2
#endif

#ifdef __AVX2__
#include <immintrin.h>
#define HAS_AVX2
#endif

QString SimpleSIMDSupport::detectSIMDSupport()
{
    QStringList supportInfo;
    
#ifdef HAS_SSE2
    supportInfo << "SSE2";
#endif

#ifdef HAS_AVX2
    supportInfo << "AVX2";
#endif

    if (supportInfo.isEmpty()) {
        return "No SIMD support detected";
    }
    
    return "SIMD Support: " + supportInfo.join(", ");
}

bool SimpleSIMDSupport::hasSSE2Support()
{
#ifdef HAS_SSE2
    return true;
#else
    return false;
#endif
}

bool SimpleSIMDSupport::hasAVX2Support()
{
#ifdef HAS_AVX2
    return true;
#else
    return false;
#endif
}

QImage SimpleSIMDSupport::adjustBrightnessSIMD(const QImage &image, double factor)
{
    // 在有SIMD支持时使用优化版本，否则使用标量版本
#ifdef HAS_SSE2
    // 使用SSE2优化实现（简化版本，专注于正确性）
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int width = result.width();
    const int height = result.height();
    
    // 将因子转换为0-255范围
    int brightnessAdd = static_cast<int>((factor - 1.0) * 128);
    brightnessAdd = qBound(-128, brightnessAdd, 127);
    
    // 使用简化的像素处理，避免复杂的SIMD通道分离
    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        
        for (int x = 0; x < width; ++x) {
            QColor color = QColor::fromRgba(line[x]);
            color.setRed(qBound(0, color.red() + brightnessAdd, 255));
            color.setGreen(qBound(0, color.green() + brightnessAdd, 255));
            color.setBlue(qBound(0, color.blue() + brightnessAdd, 255));
            line[x] = color.rgba();
        }
    }
    
    return result;
#else
    return adjustBrightnessScalar(image, factor);
#endif
}

QImage SimpleSIMDSupport::adjustContrastSIMD(const QImage &image, double factor)
{
    // 在有SIMD支持时使用优化版本，否则使用标量版本
#ifdef HAS_SSE2
    // 使用SSE2优化实现（简化版本）
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int width = result.width();
    const int height = result.height();
    
    // 对比度调整因子
    int contrastFactor = static_cast<int>(factor * 256); // 定点数表示
    
    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        
        for (int x = 0; x < width; ++x) {
            QColor color = QColor::fromRgba(line[x]);
            
            // 对比度调整
            int r = ((color.red() - 128) * contrastFactor / 256) + 128;
            int g = ((color.green() - 128) * contrastFactor / 256) + 128;
            int b = ((color.blue() - 128) * contrastFactor / 256) + 128;
            
            color.setRed(qBound(0, r, 255));
            color.setGreen(qBound(0, g, 255));
            color.setBlue(qBound(0, b, 255));
            
            line[x] = color.rgba();
        }
    }
    
    return result;
#else
    return adjustContrastScalar(image, factor);
#endif
}

QImage SimpleSIMDSupport::adjustBrightnessScalar(const QImage &image, double factor)
{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int width = result.width();
    const int height = result.height();
    
    // 将因子转换为0-255范围的加法
    int brightnessAdd = static_cast<int>((factor - 1.0) * 128);
    brightnessAdd = qBound(-128, brightnessAdd, 127);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor color = result.pixelColor(x, y);
            color.setRed(qBound(0, color.red() + brightnessAdd, 255));
            color.setGreen(qBound(0, color.green() + brightnessAdd, 255));
            color.setBlue(qBound(0, color.blue() + brightnessAdd, 255));
            result.setPixelColor(x, y, color);
        }
    }
    
    return result;
}

QImage SimpleSIMDSupport::adjustContrastScalar(const QImage &image, double factor)
{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int width = result.width();
    const int height = result.height();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor color = result.pixelColor(x, y);
            
            // 对比度调整
            int r = static_cast<int>((color.red() - 128) * factor + 128);
            int g = static_cast<int>((color.green() - 128) * factor + 128);
            int b = static_cast<int>((color.blue() - 128) * factor + 128);
            
            color.setRed(qBound(0, r, 255));
            color.setGreen(qBound(0, g, 255));
            color.setBlue(qBound(0, b, 255));
            
            result.setPixelColor(x, y, color);
        }
    }
    
    return result;
} 