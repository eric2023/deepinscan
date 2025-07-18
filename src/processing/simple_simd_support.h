#pragma once

#include <QImage>
#include <QString>

/**
 * @brief 简化的SIMD支持类
 * 提供基本的SIMD检测和优化图像处理算法
 */
class SimpleSIMDSupport
{
public:
    /**
     * @brief 检测系统SIMD支持情况
     * @return SIMD支持信息字符串
     */
    static QString detectSIMDSupport();
    
    /**
     * @brief 检查是否支持SSE2
     * @return true表示支持SSE2
     */
    static bool hasSSE2Support();
    
    /**
     * @brief 检查是否支持AVX2
     * @return true表示支持AVX2
     */
    static bool hasAVX2Support();
    
    /**
     * @brief SIMD优化的亮度调整
     * @param image 输入图像
     * @param factor 亮度因子 (0.1-3.0)
     * @return 调整后的图像
     */
    static QImage adjustBrightnessSIMD(const QImage &image, double factor);
    
    /**
     * @brief SIMD优化的对比度调整
     * @param image 输入图像
     * @param factor 对比度因子 (0.1-3.0)
     * @return 调整后的图像
     */
    static QImage adjustContrastSIMD(const QImage &image, double factor);

private:
    /**
     * @brief 使用标量实现的亮度调整（作为回退）
     * @param image 输入图像
     * @param factor 亮度因子
     * @return 调整后的图像
     */
    static QImage adjustBrightnessScalar(const QImage &image, double factor);
    
    /**
     * @brief 使用标量实现的对比度调整（作为回退）
     * @param image 输入图像
     * @param factor 对比度因子
     * @return 调整后的图像
     */
    static QImage adjustContrastScalar(const QImage &image, double factor);
}; 