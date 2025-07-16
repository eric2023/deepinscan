// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DScannerImageProcessor.h"
#include "dscannerimageprocessor_p.h"
#include "simd_image_algorithms.h"
#include "memory_optimized_processor.h"
#include "multithreaded_processor.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QTimer>
#include <QDebug>

DSCANNER_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(dscannerImageProcessor, "deepinscan.imageprocessor")

// DScannerImageProcessor 实现
DScannerImageProcessor::DScannerImageProcessor(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerImageProcessorPrivate(this))
{
    qCDebug(dscannerImageProcessor) << "DScannerImageProcessor created";
    
    Q_D(DScannerImageProcessor);
    d->initialize();
    
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
    
    Q_D(DScannerImageProcessor);
    d->cleanup();
}

QImage DScannerImageProcessor::processImage(const QImage &image, const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing image synchronously with" << params.size() << "parameters";
    
    Q_D(DScannerImageProcessor);
    
    if (image.isNull()) {
        qCWarning(dscannerImageProcessor) << "Input image is null";
        return QImage();
    }
    
    ImageProcessingResult result = d->processImageInternal(image, params);
    
    if (!result.success) {
        qCWarning(dscannerImageProcessor) << "Image processing failed:" << result.errorMessage;
        return QImage();
    }
    
    return result.processedImage;
}

QFuture<ImageProcessingResult> DScannerImageProcessor::processImageAsync(const QImage &image, 
                                                                       const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing image asynchronously with" << params.size() << "parameters";
    
    Q_D(DScannerImageProcessor);
    
    if (image.isNull()) {
        qCWarning(dscannerImageProcessor) << "Input image is null";
        ImageProcessingResult result(false, "Input image is null");
        return QtConcurrent::run([result]() { return result; });
    }
    
    // 创建异步任务
    auto future = QtConcurrent::run([this, image, params]() -> ImageProcessingResult {
        Q_D(const DScannerImageProcessor);
        return const_cast<DScannerImageProcessorPrivate*>(d)->processImageInternal(image, params);
    });
    
    // 创建监视器
    auto *watcher = new QFutureWatcher<ImageProcessingResult>(this);
    connect(watcher, &QFutureWatcher<ImageProcessingResult>::finished, [this, watcher]() {
        Q_D(DScannerImageProcessor);
        ImageProcessingResult result = watcher->result();
        d->onImageProcessed(result);
        watcher->deleteLater();
    });
    
    watcher->setFuture(future);
    d->m_watchers.append(watcher);
    
    return future;
}

QImage DScannerImageProcessor::processScanData(const QByteArray &rawData, const ScanParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Processing scan data synchronously, size:" << rawData.size();
    
    Q_D(DScannerImageProcessor);
    
    if (rawData.isEmpty()) {
        qCWarning(dscannerImageProcessor) << "Raw data is empty";
        return QImage();
    }
    
    ImageProcessingResult result = d->processScanDataInternal(rawData, params);
    
    if (!result.success) {
        qCWarning(dscannerImageProcessor) << "Scan data processing failed:" << result.errorMessage;
        return QImage();
    }
    
    return result.processedImage;
}

QFuture<ImageProcessingResult> DScannerImageProcessor::processScanDataAsync(const QByteArray &rawData, 
                                                                          const ScanParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Processing scan data asynchronously, size:" << rawData.size();
    
    Q_D(DScannerImageProcessor);
    
    if (rawData.isEmpty()) {
        qCWarning(dscannerImageProcessor) << "Raw data is empty";
        ImageProcessingResult result(false, "Raw data is empty");
        return QtConcurrent::run([result]() { return result; });
    }
    
    // 创建异步任务
    auto future = QtConcurrent::run([this, rawData, params]() -> ImageProcessingResult {
        Q_D(const DScannerImageProcessor);
        return const_cast<DScannerImageProcessorPrivate*>(d)->processScanDataInternal(rawData, params);
    });
    
    // 创建监视器
    auto *watcher = new QFutureWatcher<ImageProcessingResult>(this);
    connect(watcher, &QFutureWatcher<ImageProcessingResult>::finished, [this, watcher]() {
        Q_D(DScannerImageProcessor);
        ImageProcessingResult result = watcher->result();
        d->onImageProcessed(result);
        watcher->deleteLater();
    });
    
    watcher->setFuture(future);
    d->m_watchers.append(watcher);
    
    return future;
}

QByteArray DScannerImageProcessor::convertToFormat(const QImage &image, ImageFormat format, ImageQuality quality)
{
    qCDebug(dscannerImageProcessor) << "Converting image to format:" << static_cast<int>(format);
    
    if (image.isNull()) {
        qCWarning(dscannerImageProcessor) << "Input image is null";
        return QByteArray();
    }
    
    return ImageFormatHandler::convertToFormat(image, format, quality);
}

bool DScannerImageProcessor::saveImage(const QImage &image, const QString &filename, 
                                     ImageFormat format, ImageQuality quality)
{
    qCDebug(dscannerImageProcessor) << "Saving image to:" << filename;
    
    if (image.isNull()) {
        qCWarning(dscannerImageProcessor) << "Input image is null";
        return false;
    }
    
    QByteArray data = convertToFormat(image, format, quality);
    if (data.isEmpty()) {
        qCWarning(dscannerImageProcessor) << "Failed to convert image to format";
        return false;
    }
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(dscannerImageProcessor) << "Failed to open file for writing:" << filename;
        return false;
    }
    
    qint64 written = file.write(data);
    file.close();
    
    if (written != data.size()) {
        qCWarning(dscannerImageProcessor) << "Failed to write complete data to file";
        return false;
    }
    
    qCDebug(dscannerImageProcessor) << "Image saved successfully, size:" << data.size();
    return true;
}

// 图像增强算法的简单实现
QImage DScannerImageProcessor::denoise(const QImage &image, int strength)
{
    return ImageAlgorithms::denoise(image, strength);
}

QImage DScannerImageProcessor::sharpen(const QImage &image, int strength)
{
    return ImageAlgorithms::sharpen(image, strength);
}

QImage DScannerImageProcessor::adjustBrightness(const QImage &image, int brightness)
{
    return ImageAlgorithms::adjustBrightness(image, brightness);
}

QImage DScannerImageProcessor::adjustContrast(const QImage &image, int contrast)
{
    return ImageAlgorithms::adjustContrast(image, contrast);
}

QImage DScannerImageProcessor::adjustGamma(const QImage &image, double gamma)
{
    return ImageAlgorithms::adjustGamma(image, gamma);
}

QImage DScannerImageProcessor::colorCorrection(const QImage &image, const QColor &whitePoint)
{
    return ImageAlgorithms::colorCorrection(image, whitePoint);
}

QImage DScannerImageProcessor::autoLevel(const QImage &image)
{
    return ImageAlgorithms::autoLevel(image);
}

QImage DScannerImageProcessor::deskew(const QImage &image)
{
    return ImageAlgorithms::deskew(image);
}

QRect DScannerImageProcessor::detectCropArea(const QImage &image)
{
    return ImageAlgorithms::detectCropArea(image);
}

QList<ImageProcessingResult> DScannerImageProcessor::processBatch(const QList<QImage> &images, 
                                                                const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing batch of" << images.size() << "images synchronously";
    
    Q_D(DScannerImageProcessor);
    
    QList<ImageProcessingResult> results;
    results.reserve(images.size());
    
    for (const QImage &image : images) {
        if (image.isNull()) {
            results.append(ImageProcessingResult(false, "Image is null"));
            continue;
        }
        
        ImageProcessingResult result = d->processImageInternal(image, params);
        results.append(result);
    }
    
    return results;
}

QFuture<QList<ImageProcessingResult>> DScannerImageProcessor::processBatchAsync(const QList<QImage> &images, 
                                                                               const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing batch of" << images.size() << "images asynchronously";
    
    Q_D(DScannerImageProcessor);
    
    // 创建异步任务
    auto future = QtConcurrent::run([this, images, params]() -> QList<ImageProcessingResult> {
        return processBatch(images, params);
    });
    
    // 创建监视器
    auto *watcher = new QFutureWatcher<QList<ImageProcessingResult>>(this);
    connect(watcher, &QFutureWatcher<QList<ImageProcessingResult>>::finished, [this, watcher]() {
        Q_D(DScannerImageProcessor);
        QList<ImageProcessingResult> results = watcher->result();
        d->onBatchProcessed(results);
        watcher->deleteLater();
    });
    
    watcher->setFuture(future);
    d->m_batchWatchers.append(watcher);
    
    return future;
}

// 预设管理
void DScannerImageProcessor::addPreset(const QString &name, const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Adding preset:" << name;
    
    Q_D(DScannerImageProcessor);
    QMutexLocker locker(&d->m_presetMutex);
    
    d->m_presets[name] = params;
    d->savePresets();
}

void DScannerImageProcessor::removePreset(const QString &name)
{
    qCDebug(dscannerImageProcessor) << "Removing preset:" << name;
    
    Q_D(DScannerImageProcessor);
    QMutexLocker locker(&d->m_presetMutex);
    
    d->m_presets.remove(name);
    d->savePresets();
}

QList<ImageProcessingParameters> DScannerImageProcessor::getPreset(const QString &name) const
{
    Q_D(const DScannerImageProcessor);
    QMutexLocker locker(&d->m_presetMutex);
    
    return d->m_presets.value(name);
}

QStringList DScannerImageProcessor::getPresetNames() const
{
    Q_D(const DScannerImageProcessor);
    QMutexLocker locker(&d->m_presetMutex);
    
    return d->m_presets.keys();
}

// 性能设置
void DScannerImageProcessor::setMaxThreads(int maxThreads)
{
    qCDebug(dscannerImageProcessor) << "Setting max threads to:" << maxThreads;
    
    Q_D(DScannerImageProcessor);
    d->m_maxThreads = qMax(1, maxThreads);
    d->m_threadPool->setMaxThreadCount(d->m_maxThreads);
    d->saveSettings();
}

int DScannerImageProcessor::maxThreads() const
{
    Q_D(const DScannerImageProcessor);
    return d->m_maxThreads;
}

void DScannerImageProcessor::setMemoryLimit(qint64 limitBytes)
{
    qCDebug(dscannerImageProcessor) << "Setting memory limit to:" << limitBytes;
    
    Q_D(DScannerImageProcessor);
    d->m_memoryLimit = qMax(static_cast<qint64>(64 * 1024 * 1024), limitBytes); // 最小64MB
    d->saveSettings();
}

qint64 DScannerImageProcessor::memoryLimit() const
{
    Q_D(const DScannerImageProcessor);
    return d->m_memoryLimit;
}

// 状态查询
bool DScannerImageProcessor::isProcessing() const
{
    Q_D(const DScannerImageProcessor);
    QMutexLocker locker(&d->m_stateMutex);
    return d->m_isProcessing;
}

int DScannerImageProcessor::pendingTasks() const
{
    Q_D(const DScannerImageProcessor);
    return d->m_pendingTasks.loadAcquire();
}

void DScannerImageProcessor::cancelAllTasks()
{
    qCDebug(dscannerImageProcessor) << "Canceling all tasks";
    
    Q_D(DScannerImageProcessor);
    d->cancelAllTasks();
}

// 统计信息
qint64 DScannerImageProcessor::totalProcessedImages() const
{
    Q_D(const DScannerImageProcessor);
    return d->m_performanceMonitor.totalProcessedImages();
}

qint64 DScannerImageProcessor::totalProcessingTime() const
{
    Q_D(const DScannerImageProcessor);
    return d->m_performanceMonitor.totalProcessingTime();
}

double DScannerImageProcessor::averageProcessingTime() const
{
    Q_D(const DScannerImageProcessor);
    return d->m_performanceMonitor.averageProcessingTime();
}

DSCANNER_END_NAMESPACE

#include "moc_DScannerImageProcessor.cpp" 