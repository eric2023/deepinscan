// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERIMAGEPROCESSOR_P_H
#define DSCANNERIMAGEPROCESSOR_P_H

#include "DScannerImageProcessor.h"
#include <QMutex>
#include <QWaitCondition>
#include <QThreadPool>
#include <QRunnable>
#include <QTimer>
#include <QHash>
#include <QQueue>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QBuffer>
#include <QImageWriter>
#include <QImageReader>
#include <QApplication>
#include <QDebug>
#include <QLoggingCategory>

// 前向声明性能优化组件
class SIMDImageAlgorithms;
class MemoryOptimizedProcessor;
class MultithreadedProcessor;

#include <memory>
#include <functional>
#include <algorithm>
#include <cmath>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dscannerImageProcessor)

// 图像处理任务
class ImageProcessingTask : public QRunnable
{
public:
    explicit ImageProcessingTask(const QImage &image, 
                               const QList<ImageProcessingParameters> &params,
                               std::function<void(const ImageProcessingResult&)> callback);
    
    void run() override;
    
private:
    QImage m_image;
    QList<ImageProcessingParameters> m_parameters;
    std::function<void(const ImageProcessingResult&)> m_callback;
};

// 扫描数据处理任务
class ScanDataProcessingTask : public QRunnable
{
public:
    explicit ScanDataProcessingTask(const QByteArray &rawData,
                                  const ScanParameters &params,
                                  std::function<void(const ImageProcessingResult&)> callback);
    
    void run() override;
    
private:
    QByteArray m_rawData;
    ScanParameters m_parameters;
    std::function<void(const ImageProcessingResult&)> m_callback;
};

// 批量处理任务
class BatchProcessingTask : public QRunnable
{
public:
    explicit BatchProcessingTask(const QList<QImage> &images,
                               const QList<ImageProcessingParameters> &params,
                               std::function<void(const QList<ImageProcessingResult>&)> callback);
    
    void run() override;
    
private:
    QList<QImage> m_images;
    QList<ImageProcessingParameters> m_parameters;
    std::function<void(const QList<ImageProcessingResult>&)> m_callback;
};

// 图像处理算法实现
class ImageAlgorithms
{
public:
    // 去噪算法
    static QImage denoise(const QImage &image, int strength);
    static QImage medianFilter(const QImage &image, int kernelSize);
    static QImage gaussianBlur(const QImage &image, double sigma);
    
    // 锐化算法
    static QImage sharpen(const QImage &image, int strength);
    static QImage unsharpMask(const QImage &image, double amount, double radius, int threshold);
    
    // 色彩调整
    static QImage adjustBrightness(const QImage &image, int brightness);
    static QImage adjustContrast(const QImage &image, int contrast);
    static QImage adjustGamma(const QImage &image, double gamma);
    static QImage adjustHSV(const QImage &image, int hue, int saturation, int value);
    
    // 色彩校正
    static QImage colorCorrection(const QImage &image, const QColor &whitePoint);
    static QImage autoLevel(const QImage &image);
    static QImage histogramEqualization(const QImage &image);
    
    // 几何变换
    static QImage deskew(const QImage &image);
    static QImage rotate(const QImage &image, double angle);
    static QImage perspective(const QImage &image, const QPolygonF &quad);
    
    // 检测算法
    static QRect detectCropArea(const QImage &image);
    static double detectSkewAngle(const QImage &image);
    static QList<QRect> detectTextRegions(const QImage &image);
    
    // 格式转换
    static QImage convertColorMode(const QImage &image, ColorMode mode);
    static QByteArray convertToFormat(const QImage &image, ImageFormat format, ImageQuality quality);
    
    // 工具函数
    static QImage applyKernel(const QImage &image, const QVector<QVector<double>> &kernel);
    static QImage applyLUT(const QImage &image, const QVector<int> &lut);
    static QVector<int> calculateHistogram(const QImage &image, int channel = -1);
    static QVector<int> generateGammaLUT(double gamma);
    
private:
    static QRgb applyKernelToPixel(const QImage &image, int x, int y, 
                                  const QVector<QVector<double>> &kernel);
    static int clamp(int value, int min = 0, int max = 255);
    static double clamp(double value, double min = 0.0, double max = 1.0);
};

// 图像格式处理器
class ImageFormatHandler
{
public:
    static QByteArray saveAsPNG(const QImage &image, ImageQuality quality);
    static QByteArray saveAsJPEG(const QImage &image, ImageQuality quality);
    static QByteArray saveAsTIFF(const QImage &image, ImageQuality quality);
    static QByteArray saveAsPDF(const QImage &image, ImageQuality quality);
    static QByteArray saveAsBMP(const QImage &image, ImageQuality quality);
    
    static QImage loadFromData(const QByteArray &data, ImageFormat format = ImageFormat::Unknown);
    static ImageFormat detectFormat(const QByteArray &data);
    static QString formatToString(ImageFormat format);
    static QString formatToMimeType(ImageFormat format);
    
private:
    static int qualityToInt(ImageQuality quality);
    static QByteArray createPDFFromImage(const QImage &image, ImageQuality quality);
};

// 扫描数据处理器
class ScanDataProcessor
{
public:
    static QImage processScanData(const QByteArray &rawData, const ScanParameters &params);
    static QImage convertRawToImage(const QByteArray &rawData, const ScanParameters &params);
    static QImage applyAutoCorrections(const QImage &image, const ScanParameters &params);
    static QImage applyScanModeOptimizations(const QImage &image, ScanMode mode);
    
private:
    static QImage convertGrayscale(const QByteArray &data, const QSize &size);
    static QImage convertRGB(const QByteArray &data, const QSize &size);
    static QImage convertCMYK(const QByteArray &data, const QSize &size);
    static QImage autoDetectAndCrop(const QImage &image);
    static QImage optimizeForText(const QImage &image);
    static QImage optimizeForPhoto(const QImage &image);
};

// 性能监控器
class PerformanceMonitor
{
public:
    PerformanceMonitor();
    
    void startTask(const QString &taskName);
    void endTask(const QString &taskName);
    void recordImageProcessed(qint64 processingTime);
    
    qint64 totalProcessedImages() const;
    qint64 totalProcessingTime() const;
    double averageProcessingTime() const;
    
    void reset();
    
private:
    mutable QMutex m_mutex;
    QHash<QString, QElapsedTimer> m_activeTasks;
    qint64 m_totalProcessedImages = 0;
    qint64 m_totalProcessingTime = 0;
    QElapsedTimer m_sessionTimer;
};

// 图像处理器私有实现
class DScannerImageProcessorPrivate
{
    Q_DECLARE_PUBLIC(DScannerImageProcessor)
    
public:
    explicit DScannerImageProcessorPrivate(DScannerImageProcessor *q);
    ~DScannerImageProcessorPrivate();
    
    // 初始化和清理
    void initialize();
    void cleanup();
    
    // 任务管理
    void submitTask(QRunnable *task);
    void cancelAllTasks();
    
    // 预设管理
    void loadPresets();
    void savePresets();
    
    // 设置管理
    void loadSettings();
    void saveSettings();
    
    // 内部处理方法
    ImageProcessingResult processImageInternal(const QImage &image, 
                                             const QList<ImageProcessingParameters> &params);
    ImageProcessingResult processScanDataInternal(const QByteArray &rawData, 
                                                const ScanParameters &params);
    
    // 算法应用
    QImage applyAlgorithm(const QImage &image, const ImageProcessingParameters &params);
    
    // 回调处理
    void onImageProcessed(const ImageProcessingResult &result);
    void onBatchProcessed(const QList<ImageProcessingResult> &results);
    
public:
    DScannerImageProcessor *q_ptr;
    
    // 线程池和任务管理
    QThreadPool *m_threadPool;
    QAtomicInt m_pendingTasks;
    QMutex m_taskMutex;
    QWaitCondition m_taskCondition;
    
    // 性能优化组件
    SIMDImageAlgorithms *m_simdProcessor;
    MemoryOptimizedProcessor *m_memoryProcessor;
    MultithreadedProcessor *m_multithreadedProcessor;
    
    // 预设配置
    QHash<QString, QList<ImageProcessingParameters>> m_presets;
    QMutex m_presetMutex;
    
    // 性能设置
    int m_maxThreads = QThread::idealThreadCount();
    qint64 m_memoryLimit = 1024 * 1024 * 1024; // 1GB
    bool m_enableSIMD = true;
    bool m_enableMemoryOptimization = true;
    bool m_enableMultithreading = true;
    
    // 统计信息
    PerformanceMonitor m_performanceMonitor;
    
    // 设置存储
    QString m_settingsPath;
    QSettings *m_settings;
    
    // 状态管理
    bool m_isProcessing = false;
    QMutex m_stateMutex;
    
    // 信号连接
    QList<QFutureWatcher<ImageProcessingResult>*> m_watchers;
    QList<QFutureWatcher<QList<ImageProcessingResult>>*> m_batchWatchers;
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERIMAGEPROCESSOR_P_H 