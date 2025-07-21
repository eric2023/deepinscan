// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERIMAGEPROCESSOR_H
#define DSCANNERIMAGEPROCESSOR_H

#include "DScannerGlobal.h"
#include "DScannerTypes.h"
#include "DScannerException.h"

#include <QObject>
#include <QImage>
#include <QByteArray>
#include <QSize>
#include <QRect>
#include <QColor>
#include <QFuture>
#include <QMutex>
#include <QThread>
#include <QSharedPointer>
#include <QVariant>

DSCANNER_BEGIN_NAMESPACE

// 图像处理算法类型
enum class ImageProcessingAlgorithm {
    None = 0,
    Denoise,           // 去噪
    Sharpen,           // 锐化
    ColorCorrection,   // 色彩校正
    ContrastEnhance,   // 对比度增强
    BrightnessAdjust,  // 亮度调整
    GammaCorrection,   // 伽马校正
    AutoLevel,         // 自动色阶
    Deskew,           // 倾斜校正
    CropDetection,    // 自动裁剪
    OCRPreprocess     // OCR预处理
};

// 图像格式类型 - 使用DScannerTypes.h中的定义
// enum class ImageFormat {
//     Unknown = 0,
//     PNG,
//     JPEG,
//     TIFF,
//     PDF,
//     BMP,
//     RAW
// };

// 图像质量设置
enum class ImageQuality {
    Low = 0,
    Medium,
    High,
    Lossless
};

// 图像处理参数
struct ImageProcessingParameters {
    ImageProcessingAlgorithm algorithm = ImageProcessingAlgorithm::None;
    QVariantMap parameters;           // 算法特定参数
    int priority = 0;                 // 处理优先级
    bool enabled = true;              // 是否启用
    
    ImageProcessingParameters() = default;
    ImageProcessingParameters(ImageProcessingAlgorithm algo, const QVariantMap &params = {})
        : algorithm(algo), parameters(params) {}
};

// 图像处理结果
struct ImageProcessingResult {
    bool success = false;
    QString errorMessage;
    QImage processedImage;
    QVariantMap metadata;
    qint64 processingTime = 0;        // 处理时间(毫秒)
    
    ImageProcessingResult() = default;
    ImageProcessingResult(bool ok, const QString &error = QString())
        : success(ok), errorMessage(error) {}
};

// 扫描参数 - 使用DScannerTypes.h中的定义
// struct ScanParameters {
//     QSize resolution = QSize(300, 300);  // DPI
//     QRect scanArea;                      // 扫描区域
//     ColorMode colorMode = ColorMode::RGB24;
//     ScanMode scanMode = ScanMode::Photo;
//     ImageFormat outputFormat = ImageFormat::PNG;
//     ImageQuality quality = ImageQuality::High;
//     bool autoDetectSize = true;
//     bool autoColorCorrection = true;
//     bool autoContrast = true;
//     double brightness = 0.0;             // -100 to 100
//     double contrast = 0.0;               // -100 to 100
//     double gamma = 1.0;                  // 0.1 to 3.0
//     
//     ScanParameters() = default;
// };

// 图像处理管道
class DScannerImageProcessorPrivate;
class DSCANNER_EXPORT DScannerImageProcessor : public QObject
{
    // Q_OBJECT  // 暂时注释掉，避免MOC问题
    
public:
    explicit DScannerImageProcessor(QObject *parent = nullptr);
    ~DScannerImageProcessor();
    
    // 基本图像处理
    QImage processImage(const QImage &image, const QList<ImageProcessingParameters> &params);
    QFuture<ImageProcessingResult> processImageAsync(const QImage &image, 
                                                   const QList<ImageProcessingParameters> &params);
    
    // 扫描数据处理
    QImage processScanData(const QByteArray &rawData, const ScanParameters &params);
    QFuture<ImageProcessingResult> processScanDataAsync(const QByteArray &rawData, 
                                                       const ScanParameters &params);
    
    // 图像格式转换
    QByteArray convertToFormat(const QImage &image, ImageFormat format, 
                              ImageQuality quality = ImageQuality::High);
    bool saveImage(const QImage &image, const QString &filename, 
                   ImageFormat format = ImageFormat::PNG, 
                   ImageQuality quality = ImageQuality::High);
    
    // 图像增强算法
    QImage denoise(const QImage &image, int strength = 50);
    QImage sharpen(const QImage &image, int strength = 50);
    QImage adjustBrightness(const QImage &image, int brightness);
    QImage adjustContrast(const QImage &image, int contrast);
    QImage adjustGamma(const QImage &image, double gamma);
    QImage colorCorrection(const QImage &image, const QColor &whitePoint = QColor(255, 255, 255));
    QImage autoLevel(const QImage &image);
    QImage deskew(const QImage &image);
    QRect detectCropArea(const QImage &image);
    
    // 批量处理
    QList<ImageProcessingResult> processBatch(const QList<QImage> &images, 
                                            const QList<ImageProcessingParameters> &params);
    QFuture<QList<ImageProcessingResult>> processBatchAsync(const QList<QImage> &images, 
                                                          const QList<ImageProcessingParameters> &params);
    
    // 预设配置
    void addPreset(const QString &name, const QList<ImageProcessingParameters> &params);
    void removePreset(const QString &name);
    QList<ImageProcessingParameters> getPreset(const QString &name) const;
    QStringList getPresetNames() const;
    
    // 性能设置
    void setMaxThreads(int maxThreads);
    int maxThreads() const;
    void setMemoryLimit(qint64 limitBytes);
    qint64 memoryLimit() const;
    
    // 状态查询
    bool isProcessing() const;
    int pendingTasks() const;
    void cancelAllTasks();
    
    // 统计信息
    qint64 totalProcessedImages() const;
    qint64 totalProcessingTime() const;
    double averageProcessingTime() const;
    
signals:
    void imageProcessed(const ImageProcessingResult &result);
    void processingProgress(int percentage);
    void processingStarted();
    void processingFinished();
    void errorOccurred(const QString &error);
    
private:
    // 预设文件操作
    void savePresetsToFile() const;
    void loadPresetsFromFile();
    
private:
    class DScannerImageProcessorPrivate
    {
    public:
        DScannerImageProcessorPrivate(DScannerImageProcessor *q) : q_ptr(q) {}
        
        void initialize() {
            m_maxThreads = 4;
            m_memoryLimit = 1024 * 1024 * 1024; // 1GB
        }
        
        void cleanup() {
            cancelAllTasks();
        }
        
        void cancelAllTasks() {
            m_pendingTasks.clear();
        }
        
        DScannerImageProcessor *q_ptr;
        int m_maxThreads = 4;
        qint64 m_memoryLimit = 1024 * 1024 * 1024;
        qint64 m_totalProcessedImages = 0;
        qint64 m_totalProcessingTime = 0;
        QList<QFutureWatcher<ImageProcessingResult>*> m_pendingTasks;
        QMutex m_mutex;
        
        // 预设管理
        QHash<QString, QList<ImageProcessingParameters>> presets;
        QMutex presetMutex;
    };
    DScannerImageProcessorPrivate *d_ptr;
};

DSCANNER_END_NAMESPACE

// Qt元类型注册 - 只注册processing特有的类型
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ImageProcessingAlgorithm)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ImageQuality)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ImageProcessingParameters)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ImageProcessingResult)

#endif // DSCANNERIMAGEPROCESSOR_H 