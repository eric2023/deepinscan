// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dscannerimageprocessor_p.h"

#include <QTransform>
#include <QPainter>
#include <QDebug>
#include <QColor>
#include <QRgb>
#include <QPolygonF>
#include <QVector>

#include <algorithm>
#include <cmath>

DSCANNER_BEGIN_NAMESPACE

// ImageAlgorithms 实现

QImage ImageAlgorithms::denoise(const QImage &image, int strength)
{
    qCDebug(dscannerImageProcessor) << "Applying denoise filter with strength:" << strength;
    
    if (image.isNull() || strength <= 0) {
        return image;
    }
    
    // 使用中值滤波进行去噪
    int kernelSize = qBound(3, strength / 10 + 3, 9);
    if (kernelSize % 2 == 0) {
        kernelSize++;
    }
    
    return medianFilter(image, kernelSize);
}

QImage ImageAlgorithms::medianFilter(const QImage &image, int kernelSize)
{
    if (image.isNull() || kernelSize < 3) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    int half = kernelSize / 2;
    
    for (int y = half; y < height - half; y++) {
        for (int x = half; x < width - half; x++) {
            QVector<int> reds, greens, blues;
            
            // 收集邻域像素
            for (int ky = -half; ky <= half; ky++) {
                for (int kx = -half; kx <= half; kx++) {
                    QRgb pixel = result.pixel(x + kx, y + ky);
                    reds.append(qRed(pixel));
                    greens.append(qGreen(pixel));
                    blues.append(qBlue(pixel));
                }
            }
            
            // 排序并取中值
            std::sort(reds.begin(), reds.end());
            std::sort(greens.begin(), greens.end());
            std::sort(blues.begin(), blues.end());
            
            int mid = reds.size() / 2;
            QRgb medianPixel = qRgb(reds[mid], greens[mid], blues[mid]);
            result.setPixel(x, y, medianPixel);
        }
    }
    
    return result;
}

QImage ImageAlgorithms::gaussianBlur(const QImage &image, double sigma)
{
    if (image.isNull() || sigma <= 0) {
        return image;
    }
    
    // 生成高斯核
    int kernelSize = static_cast<int>(6 * sigma + 1);
    if (kernelSize % 2 == 0) {
        kernelSize++;
    }
    
    QVector<QVector<double>> kernel(kernelSize, QVector<double>(kernelSize));
    int half = kernelSize / 2;
    double sum = 0.0;
    
    for (int y = -half; y <= half; y++) {
        for (int x = -half; x <= half; x++) {
            double value = std::exp(-(x * x + y * y) / (2 * sigma * sigma));
            kernel[y + half][x + half] = value;
            sum += value;
        }
    }
    
    // 归一化
    for (int y = 0; y < kernelSize; y++) {
        for (int x = 0; x < kernelSize; x++) {
            kernel[y][x] /= sum;
        }
    }
    
    return applyKernel(image, kernel);
}

QImage ImageAlgorithms::sharpen(const QImage &image, int strength)
{
    qCDebug(dscannerImageProcessor) << "Applying sharpen filter with strength:" << strength;
    
    if (image.isNull() || strength <= 0) {
        return image;
    }
    
    // 锐化核
    double factor = strength / 100.0;
    QVector<QVector<double>> kernel = {
        {0, -factor, 0},
        {-factor, 1 + 4 * factor, -factor},
        {0, -factor, 0}
    };
    
    return applyKernel(image, kernel);
}

QImage ImageAlgorithms::unsharpMask(const QImage &image, double amount, double radius, int threshold)
{
    if (image.isNull()) {
        return image;
    }
    
    // 创建高斯模糊版本
    QImage blurred = gaussianBlur(image, radius);
    
    // 应用反锐化遮罩
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb original = image.pixel(x, y);
            QRgb blur = blurred.pixel(x, y);
            
            int r = qRed(original);
            int g = qGreen(original);
            int b = qBlue(original);
            
            int br = qRed(blur);
            int bg = qGreen(blur);
            int bb = qBlue(blur);
            
            // 计算差值
            int dr = r - br;
            int dg = g - bg;
            int db = b - bb;
            
            // 应用阈值
            if (std::abs(dr) > threshold || std::abs(dg) > threshold || std::abs(db) > threshold) {
                r = clamp(r + static_cast<int>(amount * dr));
                g = clamp(g + static_cast<int>(amount * dg));
                b = clamp(b + static_cast<int>(amount * db));
            }
            
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QImage ImageAlgorithms::adjustBrightness(const QImage &image, int brightness)
{
    qCDebug(dscannerImageProcessor) << "Adjusting brightness by:" << brightness;
    
    if (image.isNull() || brightness == 0) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = result.pixel(x, y);
            int r = clamp(qRed(pixel) + brightness);
            int g = clamp(qGreen(pixel) + brightness);
            int b = clamp(qBlue(pixel) + brightness);
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QImage ImageAlgorithms::adjustContrast(const QImage &image, int contrast)
{
    qCDebug(dscannerImageProcessor) << "Adjusting contrast by:" << contrast;
    
    if (image.isNull() || contrast == 0) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    
    double factor = (259.0 * (contrast + 255.0)) / (255.0 * (259.0 - contrast));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = result.pixel(x, y);
            int r = clamp(static_cast<int>(factor * (qRed(pixel) - 128) + 128));
            int g = clamp(static_cast<int>(factor * (qGreen(pixel) - 128) + 128));
            int b = clamp(static_cast<int>(factor * (qBlue(pixel) - 128) + 128));
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QImage ImageAlgorithms::adjustGamma(const QImage &image, double gamma)
{
    qCDebug(dscannerImageProcessor) << "Adjusting gamma to:" << gamma;
    
    if (image.isNull() || gamma <= 0) {
        return image;
    }
    
    // 生成伽马查找表
    QVector<int> lut = generateGammaLUT(gamma);
    
    return applyLUT(image, lut);
}

QImage ImageAlgorithms::adjustHSV(const QImage &image, int hue, int saturation, int value)
{
    if (image.isNull()) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = result.pixel(x, y);
            QColor color(pixel);
            
            int h, s, v;
            color.getHsv(&h, &s, &v);
            
            // 调整HSV值
            h = (h + hue) % 360;
            if (h < 0) h += 360;
            
            s = clamp(s + saturation, 0, 255);
            v = clamp(v + value, 0, 255);
            
            color.setHsv(h, s, v);
            result.setPixel(x, y, color.rgb());
        }
    }
    
    return result;
}

QImage ImageAlgorithms::colorCorrection(const QImage &image, const QColor &whitePoint)
{
    qCDebug(dscannerImageProcessor) << "Applying color correction with white point:" << whitePoint;
    
    if (image.isNull()) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    
    // 计算校正因子
    double rFactor = 255.0 / whitePoint.red();
    double gFactor = 255.0 / whitePoint.green();
    double bFactor = 255.0 / whitePoint.blue();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = result.pixel(x, y);
            int r = clamp(static_cast<int>(qRed(pixel) * rFactor));
            int g = clamp(static_cast<int>(qGreen(pixel) * gFactor));
            int b = clamp(static_cast<int>(qBlue(pixel) * bFactor));
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QImage ImageAlgorithms::autoLevel(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Applying auto level";
    
    if (image.isNull()) {
        return image;
    }
    
    // 计算直方图
    QVector<int> histR = calculateHistogram(image, 0);
    QVector<int> histG = calculateHistogram(image, 1);
    QVector<int> histB = calculateHistogram(image, 2);
    
    // 找到最小和最大值
    int minR = 0, maxR = 255;
    int minG = 0, maxG = 255;
    int minB = 0, maxB = 255;
    
    for (int i = 0; i < 256; i++) {
        if (histR[i] > 0) { minR = i; break; }
    }
    for (int i = 255; i >= 0; i--) {
        if (histR[i] > 0) { maxR = i; break; }
    }
    
    for (int i = 0; i < 256; i++) {
        if (histG[i] > 0) { minG = i; break; }
    }
    for (int i = 255; i >= 0; i--) {
        if (histG[i] > 0) { maxG = i; break; }
    }
    
    for (int i = 0; i < 256; i++) {
        if (histB[i] > 0) { minB = i; break; }
    }
    for (int i = 255; i >= 0; i--) {
        if (histB[i] > 0) { maxB = i; break; }
    }
    
    // 应用线性拉伸
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = result.pixel(x, y);
            
            int r = clamp(static_cast<int>(255.0 * (qRed(pixel) - minR) / (maxR - minR)));
            int g = clamp(static_cast<int>(255.0 * (qGreen(pixel) - minG) / (maxG - minG)));
            int b = clamp(static_cast<int>(255.0 * (qBlue(pixel) - minB) / (maxB - minB)));
            
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QImage ImageAlgorithms::histogramEqualization(const QImage &image)
{
    if (image.isNull()) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    int totalPixels = width * height;
    
    // 计算累积直方图
    QVector<int> histR = calculateHistogram(image, 0);
    QVector<int> histG = calculateHistogram(image, 1);
    QVector<int> histB = calculateHistogram(image, 2);
    
    QVector<int> cdfR(256), cdfG(256), cdfB(256);
    cdfR[0] = histR[0];
    cdfG[0] = histG[0];
    cdfB[0] = histB[0];
    
    for (int i = 1; i < 256; i++) {
        cdfR[i] = cdfR[i-1] + histR[i];
        cdfG[i] = cdfG[i-1] + histG[i];
        cdfB[i] = cdfB[i-1] + histB[i];
    }
    
    // 应用均衡化
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = result.pixel(x, y);
            
            int r = clamp(static_cast<int>(255.0 * cdfR[qRed(pixel)] / totalPixels));
            int g = clamp(static_cast<int>(255.0 * cdfG[qGreen(pixel)] / totalPixels));
            int b = clamp(static_cast<int>(255.0 * cdfB[qBlue(pixel)] / totalPixels));
            
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QImage ImageAlgorithms::deskew(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Applying deskew";
    
    if (image.isNull()) {
        return image;
    }
    
    // 检测倾斜角度
    double angle = detectSkewAngle(image);
    
    if (std::abs(angle) < 0.1) {
        return image; // 角度太小，不需要校正
    }
    
    return rotate(image, -angle);
}

QImage ImageAlgorithms::rotate(const QImage &image, double angle)
{
    if (image.isNull() || std::abs(angle) < 0.01) {
        return image;
    }
    
    QTransform transform;
    transform.rotate(angle);
    
    return image.transformed(transform, Qt::SmoothTransformation);
}

QImage ImageAlgorithms::perspective(const QImage &image, const QPolygonF &quad)
{
    if (image.isNull() || quad.size() != 4) {
        return image;
    }
    
    // 这里简化实现，实际需要更复杂的透视变换
    QTransform transform;
    QPolygonF source;
    source << QPointF(0, 0) << QPointF(image.width(), 0) 
           << QPointF(image.width(), image.height()) << QPointF(0, image.height());
    
    if (QTransform::quadToQuad(source, quad, transform)) {
        return image.transformed(transform, Qt::SmoothTransformation);
    }
    
    return image;
}

QRect ImageAlgorithms::detectCropArea(const QImage &image)
{
    qCDebug(dscannerImageProcessor) << "Detecting crop area";
    
    if (image.isNull()) {
        return QRect();
    }
    
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    int width = gray.width();
    int height = gray.height();
    
    // 简单的边缘检测
    int threshold = 30;
    int left = 0, right = width - 1;
    int top = 0, bottom = height - 1;
    
    // 检测左边界
    for (int x = 0; x < width; x++) {
        bool hasContent = false;
        for (int y = 0; y < height; y++) {
            if (qGray(gray.pixel(x, y)) > threshold) {
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
    for (int x = width - 1; x >= 0; x--) {
        bool hasContent = false;
        for (int y = 0; y < height; y++) {
            if (qGray(gray.pixel(x, y)) > threshold) {
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
    for (int y = 0; y < height; y++) {
        bool hasContent = false;
        for (int x = 0; x < width; x++) {
            if (qGray(gray.pixel(x, y)) > threshold) {
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
    for (int y = height - 1; y >= 0; y--) {
        bool hasContent = false;
        for (int x = 0; x < width; x++) {
            if (qGray(gray.pixel(x, y)) > threshold) {
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

{
    if (image.isNull()) {
        return 0.0;
    }
    
    // Hough变换倾斜检测实现
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    int width = gray.width();
    int height = gray.height();
    
    // Sobel边缘检测
    std::vector<std::vector<int>> edges(height, std::vector<int>(width, 0));
    int sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int gx = 0, gy = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int pixel = qGray(gray.pixel(x + dx, y + dy));
                    gx += pixel * sobelX[dy + 1][dx + 1];
                    gy += pixel * sobelY[dy + 1][dx + 1];
                }
            }
            int magnitude = static_cast<int>(std::sqrt(gx * gx + gy * gy));
            edges[y][x] = (magnitude > 50) ? 1 : 0;
        }
    }
    
    // Hough变换检测直线
    const double angleRange = 20.0;
    const double angleStep = 0.5;
    const int numAngles = static_cast<int>(2 * angleRange / angleStep) + 1;
    double maxDist = std::sqrt(width * width + height * height);
    const int distStep = 2;
    const int numDists = static_cast<int>(2 * maxDist / distStep) + 1;
    
    std::vector<std::vector<int>> accumulator(numAngles, std::vector<int>(numDists, 0));
    
    // Hough变换累积
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (edges[y][x] == 1) {
                for (int a = 0; a < numAngles; ++a) {
                    double angle = (a * angleStep - angleRange) * M_PI / 180.0;
                    double dist = x * std::cos(angle) + y * std::sin(angle);
                    int distIndex = static_cast<int>((dist + maxDist) / distStep);
                    if (distIndex >= 0 && distIndex < numDists) {
                        accumulator[a][distIndex]++;
                    }
                }
            }
        }
    }
    
    // 查找最强直线
    int maxVotes = 0;
    int bestAngleIndex = numAngles / 2;
    for (int a = 0; a < numAngles; ++a) {
        for (int d = 0; d < numDists; ++d) {
            if (accumulator[a][d] > maxVotes) {
                maxVotes = accumulator[a][d];
                bestAngleIndex = a;
            }
        }
    }
    
    double detectedAngle = bestAngleIndex * angleStep - angleRange;
    if (maxVotes < std::min(width, height) / 20) {
        return 0.0;
    }
    return detectedAngle;
    return 0.0;
}

QList<QRect> ImageAlgorithms::detectTextRegions(const QImage &image)
{
    if (image.isNull()) {
        return QList<QRect>();
    }
    
    // 形态学文本区域检测
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    int width = gray.width();
    int height = gray.height();
    
    // 创建二值化图像
    std::vector<std::vector<bool>> binary(height, std::vector<bool>(width, false));
    
    // Otsu自适应阈值二值化
    int histogram[256] = {0};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel = qGray(gray.pixel(x, y));
            histogram[pixel]++;
        }
    }
    
    int totalPixels = width * height;
    double sum = 0;
    for (int i = 0; i < 256; ++i) sum += i * histogram[i];
    
    double sumB = 0, wB = 0, maximum = 0;
    int threshold = 0;
    
    for (int t = 0; t < 256; ++t) {
        wB += histogram[t];
        if (wB == 0) continue;
        
        double wF = totalPixels - wB;
        if (wF == 0) break;
        
        sumB += t * histogram[t];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double between = wB * wF * (mB - mF) * (mB - mF);
        
        if (between > maximum) {
            maximum = between;
            threshold = t;
        }
    }
    
    // 应用阈值
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel = qGray(gray.pixel(x, y));
            binary[y][x] = (pixel < threshold);
        }
    }
    
    // 形态学操作 - 水平膨胀以连接文本字符
    int dilateKernelWidth = std::max(2, width / 100);
    std::vector<std::vector<bool>> dilated = binary;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (binary[y][x]) {
                for (int dx = -dilateKernelWidth; dx <= dilateKernelWidth; ++dx) {
                    int nx = x + dx;
                    if (nx >= 0 && nx < width) {
                        dilated[y][nx] = true;
                    }
                }
            }
        }
    }
    
    // 连通组件分析
    std::vector<std::vector<int>> labels(height, std::vector<int>(width, 0));
    int currentLabel = 1;
    std::vector<QRect> componentRects;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (dilated[y][x] && labels[y][x] == 0) {
                // 泛洪填充标记连通组件
                std::queue<std::pair<int, int>> queue;
                queue.push({x, y});
                labels[y][x] = currentLabel;
                
                int minX = x, maxX = x, minY = y, maxY = y;
                int componentSize = 0;
                
                while (!queue.empty()) {
                    auto [cx, cy] = queue.front();
                    queue.pop();
                    componentSize++;
                    
                    minX = std::min(minX, cx);
                    maxX = std::max(maxX, cx);
                    minY = std::min(minY, cy);
                    maxY = std::max(maxY, cy);
                    
                    // 检查8邻域
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            int nx = cx + dx, ny = cy + dy;
                            if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                                dilated[ny][nx] && labels[ny][nx] == 0) {
                                labels[ny][nx] = currentLabel;
                                queue.push({nx, ny});
                            }
                        }
                    }
                }
                
                // 过滤文本区域：尺寸和长宽比检查
                int regionWidth = maxX - minX + 1;
                int regionHeight = maxY - minY + 1;
                double aspectRatio = static_cast<double>(regionWidth) / regionHeight;
                
                int minTextSize = std::min(width, height) / 50;
                int maxTextSize = std::min(width, height) / 3;
                
                if (componentSize > minTextSize && componentSize < width * height / 4 &&
                    regionWidth > minTextSize && regionHeight > minTextSize / 4 &&
                    regionWidth < maxTextSize && regionHeight < maxTextSize &&
                    aspectRatio > 1.5 && aspectRatio < 20.0) {
                    componentRects.append(QRect(minX, minY, regionWidth, regionHeight));
                }
                
                currentLabel++;
            }
        }
    }
    
    // 合并相邻的文本区域
    QList<QRect> regions;
    for (const QRect& rect : componentRects) {
        bool merged = false;
        for (QRect& existing : regions) {
            // 检查是否可以合并（水平或垂直相邻）
            if ((std::abs(rect.bottom() - existing.top()) < height / 20) ||
                (std::abs(existing.bottom() - rect.top()) < height / 20) ||
                (std::abs(rect.right() - existing.left()) < width / 20) ||
                (std::abs(existing.right() - rect.left()) < width / 20)) {
                existing = existing.united(rect);
                merged = true;
                break;
            }
        }
        if (!merged) {
            regions.append(rect);
        }
    }
    
    return regions;
    return regions;
}

// 工具函数实现
QImage ImageAlgorithms::applyKernel(const QImage &image, const QVector<QVector<double>> &kernel)
{
    if (image.isNull() || kernel.isEmpty()) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    int kernelSize = kernel.size();
    int half = kernelSize / 2;
    
    for (int y = half; y < height - half; y++) {
        for (int x = half; x < width - half; x++) {
            QRgb newPixel = applyKernelToPixel(image, x, y, kernel);
            result.setPixel(x, y, newPixel);
        }
    }
    
    return result;
}

QRgb ImageAlgorithms::applyKernelToPixel(const QImage &image, int x, int y, 
                                        const QVector<QVector<double>> &kernel)
{
    double r = 0, g = 0, b = 0;
    int kernelSize = kernel.size();
    int half = kernelSize / 2;
    
    for (int ky = 0; ky < kernelSize; ky++) {
        for (int kx = 0; kx < kernelSize; kx++) {
            int px = x + kx - half;
            int py = y + ky - half;
            
            if (px >= 0 && px < image.width() && py >= 0 && py < image.height()) {
                QRgb pixel = image.pixel(px, py);
                double weight = kernel[ky][kx];
                
                r += qRed(pixel) * weight;
                g += qGreen(pixel) * weight;
                b += qBlue(pixel) * weight;
            }
        }
    }
    
    return qRgb(clamp(static_cast<int>(r)), 
                clamp(static_cast<int>(g)), 
                clamp(static_cast<int>(b)));
}

QImage ImageAlgorithms::applyLUT(const QImage &image, const QVector<int> &lut)
{
    if (image.isNull() || lut.size() != 256) {
        return image;
    }
    
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = result.pixel(x, y);
            int r = lut[qRed(pixel)];
            int g = lut[qGreen(pixel)];
            int b = lut[qBlue(pixel)];
            result.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return result;
}

QVector<int> ImageAlgorithms::calculateHistogram(const QImage &image, int channel)
{
    QVector<int> histogram(256, 0);
    
    if (image.isNull()) {
        return histogram;
    }
    
    int width = image.width();
    int height = image.height();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = image.pixel(x, y);
            
            switch (channel) {
            case 0: // Red
                histogram[qRed(pixel)]++;
                break;
            case 1: // Green
                histogram[qGreen(pixel)]++;
                break;
            case 2: // Blue
                histogram[qBlue(pixel)]++;
                break;
            default: // Grayscale
                histogram[qGray(pixel)]++;
                break;
            }
        }
    }
    
    return histogram;
}

QVector<int> ImageAlgorithms::generateGammaLUT(double gamma)
{
    QVector<int> lut(256);
    
    for (int i = 0; i < 256; i++) {
        double normalized = i / 255.0;
        double corrected = std::pow(normalized, 1.0 / gamma);
        lut[i] = clamp(static_cast<int>(corrected * 255.0));
    }
    
    return lut;
}

int ImageAlgorithms::clamp(int value, int min, int max)
{
    return qBound(min, value, max);
}

double ImageAlgorithms::clamp(double value, double min, double max)
{
    return qBound(min, value, max);
}

DSCANNER_END_NAMESPACE 