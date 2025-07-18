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

#pragma once

#include <QImage>
#include <QRect>
#include <QtGlobal>

// SIMD支持检测
#ifdef __SSE2__
#include <emmintrin.h>
#define SIMD_SSE2_SUPPORTED
#endif

#ifdef __AVX2__
#include <immintrin.h>
#define SIMD_AVX2_SUPPORTED
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>
#define SIMD_NEON_SUPPORTED
#endif

/**
 * @brief SIMDImageAlgorithms SIMD优化的图像处理算法类
 * 
 * 提供高性能的图像处理算法实现，利用现代CPU的SIMD指令集
 * 进行向量化计算以提升处理速度。支持SSE2、AVX2和ARM NEON指令集。
 */
class SIMDImageAlgorithms
{
public:
    /**
     * @brief 检测SIMD支持情况
     * @return 支持的SIMD指令集信息
     */
    static QString detectSIMDSupport();
    
    /**
     * @brief 检查是否支持AVX2指令集
     * @return true表示支持AVX2
     */
    static bool hasAVX2Support();
    
    /**
     * @brief 检查是否支持SSE2指令集
     * @return true表示支持SSE2
     */
    static bool hasSSE2Support();
    
    /**
     * @brief 检查是否支持ARM NEON指令集
     * @return true表示支持NEON
     */
    static bool hasNEONSupport();

    // 图像增强算法
    /**
     * @brief SIMD优化的图像亮度调整
     * @param image 输入图像
     * @param factor 亮度调整因子 (0.1-3.0)
     * @return 调整后的图像
     */
    static QImage adjustBrightnessSIMD(const QImage &image, double factor);
    
    /**
     * @brief SIMD优化的图像对比度调整
     * @param image 输入图像
     * @param factor 对比度调整因子 (0.1-3.0)
     * @return 调整后的图像
     */
    static QImage adjustContrastSIMD(const QImage &image, double factor);
    
    /**
     * @brief SIMD优化的图像饱和度调整
     * @param image 输入图像
     * @param factor 饱和度调整因子 (0.0-2.0)
     * @return 调整后的图像
     */
    static QImage adjustSaturationSIMD(const QImage &image, double factor);

    // 滤波算法
    /**
     * @brief SIMD优化的高斯模糊
     * @param image 输入图像
     * @param radius 模糊半径 (1-20)
     * @param sigma 高斯标准差
     * @return 模糊后的图像
     */
    static QImage gaussianBlurSIMD(const QImage &image, int radius, double sigma = -1.0);
    
    /**
     * @brief SIMD优化的锐化滤波
     * @param image 输入图像
     * @param strength 锐化强度 (0.1-2.0)
     * @return 锐化后的图像
     */
    static QImage sharpenSIMD(const QImage &image, double strength = 1.0);
    
    /**
     * @brief SIMD优化的边缘检测
     * @param image 输入图像
     * @param threshold 边缘阈值 (0-255)
     * @return 边缘检测结果
     */
    static QImage edgeDetectionSIMD(const QImage &image, int threshold = 100);

    // 降噪算法
    /**
     * @brief SIMD优化的均值降噪
     * @param image 输入图像
     * @param kernelSize 降噪核大小 (3,5,7,9)
     * @return 降噪后的图像
     */
    static QImage meanDenoiseSimd(const QImage &image, int kernelSize = 3);
    
    /**
     * @brief SIMD优化的中值降噪
     * @param image 输入图像
     * @param kernelSize 降噪核大小 (3,5,7)
     * @return 降噪后的图像
     */
    static QImage medianDenoiseSIMD(const QImage &image, int kernelSize = 3);
    
    /**
     * @brief SIMD优化的双边滤波降噪
     * @param image 输入图像
     * @param d 像素邻域直径
     * @param sigmaColor 颜色空间标准差
     * @param sigmaSpace 坐标空间标准差
     * @return 降噪后的图像
     */
    static QImage bilateralFilterSIMD(const QImage &image, int d, double sigmaColor, double sigmaSpace);

    // 几何变换
    /**
     * @brief SIMD优化的图像缩放
     * @param image 输入图像
     * @param newSize 新尺寸
     * @param interpolation 插值方法
     * @return 缩放后的图像
     */
    static QImage scaleSIMD(const QImage &image, const QSize &newSize, Qt::TransformationMode interpolation = Qt::SmoothTransformation);
    
    /**
     * @brief SIMD优化的图像旋转
     * @param image 输入图像
     * @param angle 旋转角度（度）
     * @param interpolation 插值方法
     * @return 旋转后的图像
     */
    static QImage rotateSIMD(const QImage &image, double angle, Qt::TransformationMode interpolation = Qt::SmoothTransformation);

    // 颜色空间转换
    /**
     * @brief SIMD优化的RGB到灰度转换
     * @param image 输入彩色图像
     * @return 灰度图像
     */
    static QImage convertToGrayscaleSIMD(const QImage &image);
    
    /**
     * @brief SIMD优化的RGB到HSV转换
     * @param image 输入RGB图像
     * @return HSV图像数据
     */
    static QImage convertRGBtoHSVSIMD(const QImage &image);
    
    /**
     * @brief SIMD优化的HSV到RGB转换
     * @param hsvData HSV图像数据
     * @return RGB图像
     */
    static QImage convertHSVtoRGBSIMD(const QImage &hsvData);

    // 统计计算
    /**
     * @brief SIMD优化的图像直方图计算
     * @param image 输入图像
     * @param channel 颜色通道 (0=红, 1=绿, 2=蓝, -1=灰度)
     * @return 256元素的直方图数组
     */
    static QVector<int> calculateHistogramSIMD(const QImage &image, int channel = -1);
    
    /**
     * @brief SIMD优化的图像均值计算
     * @param image 输入图像
     * @return RGB通道的均值
     */
    static QVector<double> calculateMeanSIMD(const QImage &image);
    
    /**
     * @brief SIMD优化的图像方差计算
     * @param image 输入图像
     * @return RGB通道的方差
     */
    static QVector<double> calculateVarianceSIMD(const QImage &image);

private:
    // SSE2实现
#ifdef SIMD_SSE2_SUPPORTED
    static QImage adjustBrightnessSSE2(const QImage &image, double factor);
    static QImage adjustContrastSSE2(const QImage &image, double factor);
    static QImage gaussianBlurSSE2(const QImage &image, int radius, double sigma);
    static QImage convertToGrayscaleSSE2(const QImage &image);
    static QVector<int> calculateHistogramSSE2(const QImage &image, int channel);
#endif

    // AVX2实现
#ifdef SIMD_AVX2_SUPPORTED
    static QImage adjustBrightnessAVX2(const QImage &image, double factor);
    static QImage adjustContrastAVX2(const QImage &image, double factor);
    static QImage gaussianBlurAVX2(const QImage &image, int radius, double sigma);
    static QImage convertToGrayscaleAVX2(const QImage &image);
    static QVector<int> calculateHistogramAVX2(const QImage &image, int channel);
#endif

    // ARM NEON实现
#ifdef SIMD_NEON_SUPPORTED
    static QImage adjustBrightnessNEON(const QImage &image, double factor);
    static QImage adjustContrastNEON(const QImage &image, double factor);
    static QImage gaussianBlurNEON(const QImage &image, int radius, double sigma);
    static QImage convertToGrayscaleNEON(const QImage &image);
    static QVector<int> calculateHistogramNEON(const QImage &image, int channel);
#endif

    // Scalar后备实现（当SIMD不可用时）
    static QImage adjustBrightnessScalar(const QImage &image, double factor);
    static QImage adjustContrastScalar(const QImage &image, double factor);
    static QImage convertToGrayscaleScalar(const QImage &image);
    static QImage gaussianBlurScalar(const QImage &image, int radius, double sigma);
    static QImage adjustSaturationInHSV(const QImage &hsvImage, double factor);

    // 辅助方法
    /**
     * @brief 生成高斯核
     * @param radius 核半径
     * @param sigma 标准差
     * @return 高斯核数据
     */
    static QVector<float> generateGaussianKernel(int radius, double sigma);
    
    /**
     * @brief 应用可分离卷积核
     * @param image 输入图像
     * @param kernel 卷积核
     * @param horizontal 是否为水平方向
     * @return 卷积结果
     */
    static QImage applySeparableKernel(const QImage &image, const QVector<float> &kernel, bool horizontal);
    
    /**
     * @brief 检查图像格式兼容性
     * @param image 输入图像
     * @return 是否兼容SIMD处理
     */
    static bool isImageFormatCompatible(const QImage &image);
    
    /**
     * @brief 确保图像为32位ARGB格式
     * @param image 输入图像
     * @return 转换后的图像
     */
    static QImage ensureARGB32Format(const QImage &image);
    
    /**
     * @brief 夹持值到指定范围
     * @param value 输入值
     * @param min 最小值
     * @param max 最大值
     * @return 夹持后的值
     */
    template<typename T>
    static T clamp(T value, T min, T max) {
        return value < min ? min : (value > max ? max : value);
    }
    
    /**
     * @brief 将浮点值转换为8位整数
     * @param value 浮点值
     * @return 8位整数值
     */
    static quint8 floatToUint8(float value) {
        return static_cast<quint8>(clamp(value + 0.5f, 0.0f, 255.0f));
    }
}; 