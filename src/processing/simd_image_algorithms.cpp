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

#include "simd_image_algorithms.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QtMath>
#include <QRgb>
#include <QPainter>
#include <algorithm>
#include <cstring>

// CPUID检测支持（x86/x64平台）
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <cpuid.h>
#define CPUID_SUPPORTED
#endif

QString SIMDImageAlgorithms::detectSIMDSupport()
{
    qDebug() << "SIMDImageAlgorithms::detectSIMDSupport: 检测SIMD指令集支持";
    
    QStringList supportedSets;
    
#ifdef SIMD_SSE2_SUPPORTED
    if (hasSSE2Support()) {
        supportedSets << "SSE2";
    }
#endif

#ifdef SIMD_AVX2_SUPPORTED
    if (hasAVX2Support()) {
        supportedSets << "AVX2";
    }
#endif

#ifdef SIMD_NEON_SUPPORTED
    if (hasNEONSupport()) {
        supportedSets << "ARM NEON";
    }
#endif

    if (supportedSets.isEmpty()) {
        supportedSets << "标量处理 (无SIMD)";
    }
    
    QString result = supportedSets.join(", ");
    qDebug() << "支持的SIMD指令集:" << result;
    return result;
}

bool SIMDImageAlgorithms::hasAVX2Support()
{
#ifdef SIMD_AVX2_SUPPORTED
#ifdef CPUID_SUPPORTED
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(7, &eax, &ebx, &ecx, &edx)) {
        return (ebx & (1 << 5)) != 0; // AVX2 bit
    }
#endif
    return true; // 编译时支持，假设运行时也支持
#else
    return false;
#endif
}

bool SIMDImageAlgorithms::hasSSE2Support()
{
#ifdef SIMD_SSE2_SUPPORTED
#ifdef CPUID_SUPPORTED
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return (edx & (1 << 26)) != 0; // SSE2 bit
    }
#endif
    return true; // 编译时支持，假设运行时也支持
#else
    return false;
#endif
}

bool SIMDImageAlgorithms::hasNEONSupport()
{
#ifdef SIMD_NEON_SUPPORTED
    return true; // ARM NEON在编译时确定
#else
    return false;
#endif
}

QImage SIMDImageAlgorithms::adjustBrightnessSIMD(const QImage &image, double factor)
{
    qDebug() << "SIMDImageAlgorithms::adjustBrightnessSIMD: 开始SIMD亮度调整，因子:" << factor;
    
    if (image.isNull() || factor < 0.1 || factor > 3.0) {
        qWarning() << "无效的输入参数";
        return QImage();
    }
    
    QElapsedTimer timer;
    timer.start();
    
    QImage result;
    
    // 选择最优的SIMD实现
#ifdef SIMD_AVX2_SUPPORTED
    if (hasAVX2Support()) {
        result = adjustBrightnessAVX2(image, factor);
        qDebug() << "使用AVX2优化实现";
    } else
#endif
#ifdef SIMD_SSE2_SUPPORTED
    if (hasSSE2Support()) {
        result = adjustBrightnessSSE2(image, factor);
        qDebug() << "使用SSE2优化实现";
    } else
#endif
#ifdef SIMD_NEON_SUPPORTED
    if (hasNEONSupport()) {
        result = adjustBrightnessNEON(image, factor);
        qDebug() << "使用NEON优化实现";
    } else
#endif
    {
        // 标量实现回退
        result = adjustBrightnessScalar(image, factor);
        qDebug() << "使用标量实现";
    }
    
    qDebug() << "SIMD亮度调整完成，用时:" << timer.elapsed() << "毫秒";
    return result;
}

QImage SIMDImageAlgorithms::adjustContrastSIMD(const QImage &image, double factor)
{
    qDebug() << "SIMDImageAlgorithms::adjustContrastSIMD: 开始SIMD对比度调整，因子:" << factor;
    
    if (image.isNull() || factor < 0.1 || factor > 3.0) {
        qWarning() << "无效的输入参数";
        return QImage();
    }
    
    QElapsedTimer timer;
    timer.start();
    
    QImage result;
    
    // 选择最优的SIMD实现
#ifdef SIMD_AVX2_SUPPORTED
    if (hasAVX2Support()) {
        result = adjustContrastAVX2(image, factor);
        qDebug() << "使用AVX2优化实现";
    } else
#endif
#ifdef SIMD_SSE2_SUPPORTED
    if (hasSSE2Support()) {
        result = adjustContrastSSE2(image, factor);
        qDebug() << "使用SSE2优化实现";
    } else
#endif
#ifdef SIMD_NEON_SUPPORTED
    if (hasNEONSupport()) {
        result = adjustContrastNEON(image, factor);
        qDebug() << "使用NEON优化实现";
    } else
#endif
    {
        // 标量实现回退
        result = adjustContrastScalar(image, factor);
        qDebug() << "使用标量实现";
    }
    
    qDebug() << "SIMD对比度调整完成，用时:" << timer.elapsed() << "毫秒";
    return result;
}

QImage SIMDImageAlgorithms::adjustSaturationSIMD(const QImage &image, double factor)
{
    qDebug() << "SIMDImageAlgorithms::adjustSaturationSIMD: 开始SIMD饱和度调整，因子:" << factor;
    
    if (image.isNull() || factor < 0.0 || factor > 2.0) {
        qWarning() << "无效的输入参数";
        return QImage();
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // 饱和度调整需要RGB到HSV转换，然后调整S通道，再转换回RGB
    QImage hsvImage = convertRGBtoHSVSIMD(image);
    
    // 调整饱和度通道
    QImage adjustedHSV = adjustSaturationInHSV(hsvImage, factor);
    
    // 转换回RGB
    QImage result = convertHSVtoRGBSIMD(adjustedHSV);
    
    qDebug() << "SIMD饱和度调整完成，用时:" << timer.elapsed() << "毫秒";
    return result;
}

QImage SIMDImageAlgorithms::gaussianBlurSIMD(const QImage &image, int radius, double sigma)
{
    qDebug() << "SIMDImageAlgorithms::gaussianBlurSIMD: 开始SIMD高斯模糊，半径:" << radius << "sigma:" << sigma;
    
    if (image.isNull() || radius < 1 || radius > 20) {
        qWarning() << "无效的输入参数";
        return QImage();
    }
    
    if (sigma <= 0) {
        sigma = radius / 3.0; // 默认sigma值
    }
    
    QElapsedTimer timer;
    timer.start();
    
    QImage result;
    
    // 选择最优的SIMD实现
#ifdef SIMD_AVX2_SUPPORTED
    if (hasAVX2Support()) {
        result = gaussianBlurAVX2(image, radius, sigma);
        qDebug() << "使用AVX2优化实现";
    } else
#endif
#ifdef SIMD_SSE2_SUPPORTED
    if (hasSSE2Support()) {
        result = gaussianBlurSSE2(image, radius, sigma);
        qDebug() << "使用SSE2优化实现";
    } else
#endif
#ifdef SIMD_NEON_SUPPORTED
    if (hasNEONSupport()) {
        result = gaussianBlurNEON(image, radius, sigma);
        qDebug() << "使用NEON优化实现";
    } else
#endif
    {
        // 标量实现回退
        result = gaussianBlurScalar(image, radius, sigma);
        qDebug() << "使用标量实现";
    }
    
    qDebug() << "SIMD高斯模糊完成，用时:" << timer.elapsed() << "毫秒";
    return result;
}

QImage SIMDImageAlgorithms::convertToGrayscaleSIMD(const QImage &image)
{
    qDebug() << "SIMDImageAlgorithms::convertToGrayscaleSIMD: 开始SIMD灰度转换";
    
    if (image.isNull()) {
        qWarning() << "输入图像为空";
        return QImage();
    }
    
    QElapsedTimer timer;
    timer.start();
    
    QImage result;
    
    // 选择最优的SIMD实现
#ifdef SIMD_AVX2_SUPPORTED
    if (hasAVX2Support()) {
        result = convertToGrayscaleAVX2(image);
        qDebug() << "使用AVX2优化实现";
    } else
#endif
#ifdef SIMD_SSE2_SUPPORTED
    if (hasSSE2Support()) {
        result = convertToGrayscaleSSE2(image);
        qDebug() << "使用SSE2优化实现";
    } else
#endif
#ifdef SIMD_NEON_SUPPORTED
    if (hasNEONSupport()) {
        result = convertToGrayscaleNEON(image);
        qDebug() << "使用NEON优化实现";
    } else
#endif
    {
        // 标量实现回退
        result = convertToGrayscaleScalar(image);
        qDebug() << "使用标量实现";
    }
    
    qDebug() << "SIMD灰度转换完成，用时:" << timer.elapsed() << "毫秒";
    return result;
}

// SSE2实现
#ifdef SIMD_SSE2_SUPPORTED
QImage SIMDImageAlgorithms::adjustBrightnessSSE2(const QImage &image, double factor)
{
    qDebug() << "SIMDImageAlgorithms::adjustBrightnessSSE2: SSE2亮度调整实现";
    
    QImage srcImage = ensureARGB32Format(image);
    QImage result(srcImage.size(), QImage::Format_ARGB32);
    
    const int width = srcImage.width();
    const int height = srcImage.height();
    
    // 准备SIMD常量
    const __m128i factor_i = _mm_set1_epi16(static_cast<short>(factor * 256));
    const __m128i zero = _mm_setzero_si128();
    const __m128i maxval = _mm_set1_epi16(255);
    
    for (int y = 0; y < height; ++y) {
        const quint32 *srcLine = reinterpret_cast<const quint32*>(srcImage.constScanLine(y));
        quint32 *dstLine = reinterpret_cast<quint32*>(result.scanLine(y));
        
        int x = 0;
        // 处理4个像素为一组（SSE2一次处理128位）
        for (; x < width - 3; x += 4) {
            // 加载4个ARGB像素
            __m128i pixels = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&srcLine[x]));
            
            // 分离低8位和高8位
            __m128i pixels_lo = _mm_unpacklo_epi8(pixels, zero);
            __m128i pixels_hi = _mm_unpackhi_epi8(pixels, zero);
            
            // 应用亮度调整（保持Alpha通道不变）
            __m128i alpha_mask = _mm_set1_epi16(0xFF00);
            __m128i color_mask = _mm_set1_epi16(0x00FF);
            
            // 提取并调整RGB通道
            __m128i rgb_lo = _mm_and_si128(pixels_lo, color_mask);
            __m128i rgb_hi = _mm_and_si128(pixels_hi, color_mask);
            
            rgb_lo = _mm_mulhi_epi16(rgb_lo, factor_i);
            rgb_hi = _mm_mulhi_epi16(rgb_hi, factor_i);
            
            // 夹持到0-255范围
            rgb_lo = _mm_min_epi16(_mm_max_epi16(rgb_lo, zero), maxval);
            rgb_hi = _mm_min_epi16(_mm_max_epi16(rgb_hi, zero), maxval);
            
            // 重新组合Alpha和RGB
            __m128i alpha_lo = _mm_and_si128(pixels_lo, alpha_mask);
            __m128i alpha_hi = _mm_and_si128(pixels_hi, alpha_mask);
            
            pixels_lo = _mm_or_si128(rgb_lo, alpha_lo);
            pixels_hi = _mm_or_si128(rgb_hi, alpha_hi);
            
            // 打包回8位
            __m128i result_pixels = _mm_packus_epi16(pixels_lo, pixels_hi);
            
            // 存储结果
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dstLine[x]), result_pixels);
        }
        
        // 处理剩余像素
        for (; x < width; ++x) {
            QRgb pixel = srcLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            int a = qAlpha(pixel);
            
            r = clamp(static_cast<int>(r * factor), 0, 255);
            g = clamp(static_cast<int>(g * factor), 0, 255);
            b = clamp(static_cast<int>(b * factor), 0, 255);
            
            dstLine[x] = qRgba(r, g, b, a);
        }
    }
    
    return result;
}

QImage SIMDImageAlgorithms::adjustContrastSSE2(const QImage &image, double factor)
{
    qDebug() << "SIMDImageAlgorithms::adjustContrastSSE2: SSE2对比度调整实现";
    
    QImage srcImage = ensureARGB32Format(image);
    QImage result(srcImage.size(), QImage::Format_ARGB32);
    
    const int width = srcImage.width();
    const int height = srcImage.height();
    
    // 对比度调整公式: new_value = (old_value - 128) * factor + 128
    const __m128i factor_i = _mm_set1_epi16(static_cast<short>(factor * 256));
    const __m128i offset = _mm_set1_epi16(128);
    const __m128i zero = _mm_setzero_si128();
    const __m128i maxval = _mm_set1_epi16(255);
    
    for (int y = 0; y < height; ++y) {
        const quint32 *srcLine = reinterpret_cast<const quint32*>(srcImage.constScanLine(y));
        quint32 *dstLine = reinterpret_cast<quint32*>(result.scanLine(y));
        
        int x = 0;
        for (; x < width - 3; x += 4) {
            __m128i pixels = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&srcLine[x]));
            
            __m128i pixels_lo = _mm_unpacklo_epi8(pixels, zero);
            __m128i pixels_hi = _mm_unpackhi_epi8(pixels, zero);
            
            // 应用对比度调整（保持Alpha通道）
            __m128i alpha_mask = _mm_set1_epi16(0xFF00);
            
            // 提取RGB通道
            __m128i rgb_lo = _mm_and_si128(pixels_lo, _mm_set1_epi16(0x00FF));
            __m128i rgb_hi = _mm_and_si128(pixels_hi, _mm_set1_epi16(0x00FF));
            
            // 对比度调整: (value - 128) * factor + 128
            rgb_lo = _mm_sub_epi16(rgb_lo, offset);
            rgb_hi = _mm_sub_epi16(rgb_hi, offset);
            
            rgb_lo = _mm_mulhi_epi16(rgb_lo, factor_i);
            rgb_hi = _mm_mulhi_epi16(rgb_hi, factor_i);
            
            rgb_lo = _mm_add_epi16(rgb_lo, offset);
            rgb_hi = _mm_add_epi16(rgb_hi, offset);
            
            // 夹持到0-255范围
            rgb_lo = _mm_min_epi16(_mm_max_epi16(rgb_lo, zero), maxval);
            rgb_hi = _mm_min_epi16(_mm_max_epi16(rgb_hi, zero), maxval);
            
            // 重新组合
            __m128i alpha_lo = _mm_and_si128(pixels_lo, alpha_mask);
            __m128i alpha_hi = _mm_and_si128(pixels_hi, alpha_mask);
            
            pixels_lo = _mm_or_si128(rgb_lo, alpha_lo);
            pixels_hi = _mm_or_si128(rgb_hi, alpha_hi);
            
            __m128i result_pixels = _mm_packus_epi16(pixels_lo, pixels_hi);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dstLine[x]), result_pixels);
        }
        
        // 处理剩余像素
        for (; x < width; ++x) {
            QRgb pixel = srcLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            int a = qAlpha(pixel);
            
            r = clamp(static_cast<int>((r - 128) * factor + 128), 0, 255);
            g = clamp(static_cast<int>((g - 128) * factor + 128), 0, 255);
            b = clamp(static_cast<int>((b - 128) * factor + 128), 0, 255);
            
            dstLine[x] = qRgba(r, g, b, a);
        }
    }
    
    return result;
}

QImage SIMDImageAlgorithms::convertToGrayscaleSSE2(const QImage &image)
{
    qDebug() << "SIMDImageAlgorithms::convertToGrayscaleSSE2: SSE2灰度转换实现";
    
    QImage srcImage = ensureARGB32Format(image);
    QImage result(srcImage.size(), QImage::Format_ARGB32);
    
    const int width = srcImage.width();
    const int height = srcImage.height();
    
    // RGB到灰度的加权系数（ITU-R BT.709标准）
    // Y = 0.2126*R + 0.7152*G + 0.0722*B
    const __m128i coeff_r = _mm_set1_epi16(static_cast<short>(0.2126 * 256));
    const __m128i coeff_g = _mm_set1_epi16(static_cast<short>(0.7152 * 256));
    const __m128i coeff_b = _mm_set1_epi16(static_cast<short>(0.0722 * 256));
    const __m128i zero = _mm_setzero_si128();
    
    for (int y = 0; y < height; ++y) {
        const quint32 *srcLine = reinterpret_cast<const quint32*>(srcImage.constScanLine(y));
        quint32 *dstLine = reinterpret_cast<quint32*>(result.scanLine(y));
        
        int x = 0;
        for (; x < width - 3; x += 4) {
            __m128i pixels = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&srcLine[x]));
            
            __m128i pixels_lo = _mm_unpacklo_epi8(pixels, zero);
            __m128i pixels_hi = _mm_unpackhi_epi8(pixels, zero);
            
            // 提取RGB通道
            __m128i r_lo = _mm_and_si128(pixels_lo, _mm_set1_epi16(0x00FF));
            __m128i g_lo = _mm_and_si128(_mm_srli_epi16(pixels_lo, 8), _mm_set1_epi16(0x00FF));
            __m128i b_lo = _mm_and_si128(_mm_srli_epi16(pixels_lo, 16), _mm_set1_epi16(0x00FF));
            
            __m128i r_hi = _mm_and_si128(pixels_hi, _mm_set1_epi16(0x00FF));
            __m128i g_hi = _mm_and_si128(_mm_srli_epi16(pixels_hi, 8), _mm_set1_epi16(0x00FF));
            __m128i b_hi = _mm_and_si128(_mm_srli_epi16(pixels_hi, 16), _mm_set1_epi16(0x00FF));
            
            // 计算加权灰度值
            __m128i gray_lo = _mm_add_epi16(
                _mm_add_epi16(_mm_mulhi_epi16(r_lo, coeff_r), _mm_mulhi_epi16(g_lo, coeff_g)),
                _mm_mulhi_epi16(b_lo, coeff_b)
            );
            
            __m128i gray_hi = _mm_add_epi16(
                _mm_add_epi16(_mm_mulhi_epi16(r_hi, coeff_r), _mm_mulhi_epi16(g_hi, coeff_g)),
                _mm_mulhi_epi16(b_hi, coeff_b)
            );
            
            // 构造灰度ARGB像素（R=G=B=灰度值，A保持原值）
            __m128i alpha_lo = _mm_and_si128(pixels_lo, _mm_set1_epi16(0xFF00));
            __m128i alpha_hi = _mm_and_si128(pixels_hi, _mm_set1_epi16(0xFF00));
            
            __m128i gray_pixel_lo = _mm_or_si128(_mm_or_si128(_mm_or_si128(
                gray_lo,                           // R
                _mm_slli_epi16(gray_lo, 8)),       // G
                _mm_slli_epi16(gray_lo, 16)),      // B
                alpha_lo);                         // A
                
            __m128i gray_pixel_hi = _mm_or_si128(_mm_or_si128(_mm_or_si128(
                gray_hi,                           // R
                _mm_slli_epi16(gray_hi, 8)),       // G
                _mm_slli_epi16(gray_hi, 16)),      // B
                alpha_hi);                         // A
            
            __m128i result_pixels = _mm_packus_epi16(gray_pixel_lo, gray_pixel_hi);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&dstLine[x]), result_pixels);
        }
        
        // 处理剩余像素
        for (; x < width; ++x) {
            QRgb pixel = srcLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            int a = qAlpha(pixel);
            
            int gray = static_cast<int>(0.2126 * r + 0.7152 * g + 0.0722 * b);
            dstLine[x] = qRgba(gray, gray, gray, a);
        }
    }
    
    return result;
}
#endif // SIMD_SSE2_SUPPORTED

// 标量实现作为回退方案
QImage SIMDImageAlgorithms::adjustBrightnessScalar(const QImage &image, double factor)
{
    qDebug() << "SIMDImageAlgorithms::adjustBrightnessScalar: 标量亮度调整实现";
    
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int width = result.width();
    const int height = result.height();
    
    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = line[x];
            int r = clamp(static_cast<int>(qRed(pixel) * factor), 0, 255);
            int g = clamp(static_cast<int>(qGreen(pixel) * factor), 0, 255);
            int b = clamp(static_cast<int>(qBlue(pixel) * factor), 0, 255);
            line[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }
    
    return result;
}

QImage SIMDImageAlgorithms::adjustContrastScalar(const QImage &image, double factor)
{
    qDebug() << "SIMDImageAlgorithms::adjustContrastScalar: 标量对比度调整实现";
    
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int width = result.width();
    const int height = result.height();
    
    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = line[x];
            int r = clamp(static_cast<int>((qRed(pixel) - 128) * factor + 128), 0, 255);
            int g = clamp(static_cast<int>((qGreen(pixel) - 128) * factor + 128), 0, 255);
            int b = clamp(static_cast<int>((qBlue(pixel) - 128) * factor + 128), 0, 255);
            line[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }
    
    return result;
}

QImage SIMDImageAlgorithms::convertToGrayscaleScalar(const QImage &image)
{
    qDebug() << "SIMDImageAlgorithms::convertToGrayscaleScalar: 标量灰度转换实现";
    
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int width = result.width();
    const int height = result.height();
    
    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = line[x];
            int gray = static_cast<int>(0.2126 * qRed(pixel) + 0.7152 * qGreen(pixel) + 0.0722 * qBlue(pixel));
            line[x] = qRgba(gray, gray, gray, qAlpha(pixel));
        }
    }
    
    return result;
}

// 辅助方法实现
QVector<float> SIMDImageAlgorithms::generateGaussianKernel(int radius, double sigma)
{
    qDebug() << "SIMDImageAlgorithms::generateGaussianKernel: 生成高斯核，半径:" << radius << "sigma:" << sigma;
    
    int size = 2 * radius + 1;
    QVector<float> kernel(size);
    
    float sum = 0.0f;
    float sigma2 = static_cast<float>(sigma * sigma);
    
    for (int i = 0; i < size; ++i) {
        int x = i - radius;
        float value = std::exp(-(x * x) / (2.0f * sigma2));
        kernel[i] = value;
        sum += value;
    }
    
    // 归一化
    for (int i = 0; i < size; ++i) {
        kernel[i] /= sum;
    }
    
    return kernel;
}

bool SIMDImageAlgorithms::isImageFormatCompatible(const QImage &image)
{
    return image.format() == QImage::Format_ARGB32 ||
           image.format() == QImage::Format_RGB32 ||
           image.format() == QImage::Format_ARGB32_Premultiplied;
}

QImage SIMDImageAlgorithms::ensureARGB32Format(const QImage &image)
{
    if (image.format() == QImage::Format_ARGB32) {
        return image;
    }
    return image.convertToFormat(QImage::Format_ARGB32);
}

// AVX2和NEON实现的占位符（实际实现会很复杂，这里提供框架）
#ifdef SIMD_AVX2_SUPPORTED
QImage SIMDImageAlgorithms::adjustBrightnessAVX2(const QImage &image, double factor)
{
    // AVX2实现会使用256位寄存器，一次处理8个像素
    // 这里暂时使用SSE2实现作为替代
    return adjustBrightnessSSE2(image, factor);
}

QImage SIMDImageAlgorithms::adjustContrastAVX2(const QImage &image, double factor)
{
    return adjustContrastSSE2(image, factor);
}

QImage SIMDImageAlgorithms::convertToGrayscaleAVX2(const QImage &image)
{
    return convertToGrayscaleSSE2(image);
}
#endif

#ifdef SIMD_NEON_SUPPORTED
QImage SIMDImageAlgorithms::adjustBrightnessNEON(const QImage &image, double factor)
{
    // ARM NEON实现
    // 这里暂时使用标量实现作为替代
    return adjustBrightnessScalar(image, factor);
}

QImage SIMDImageAlgorithms::adjustContrastNEON(const QImage &image, double factor)
{
    return adjustContrastScalar(image, factor);
}

QImage SIMDImageAlgorithms::convertToGrayscaleNEON(const QImage &image)
{
    return convertToGrayscaleScalar(image);
}
#endif 