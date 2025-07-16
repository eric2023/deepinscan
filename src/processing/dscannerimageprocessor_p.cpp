// SPDX-FileCopyrightText: 2024-2025 eric2023
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dscannerimageprocessor_p.h"
#include "DScannerImageProcessor.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QImageWriter>
#include <QImageReader>
#include <QTransform>
#include <QPainter>
#include <QDebug>
#include <QFile>

#include <algorithm>
#include <cmath>

DSCANNER_BEGIN_NAMESPACE

// DScannerImageProcessorPrivate 实现
DScannerImageProcessorPrivate::DScannerImageProcessorPrivate(DScannerImageProcessor *q)
    : q_ptr(q)
    , m_threadPool(new QThreadPool(q))
    , m_pendingTasks(0)
    , m_settings(nullptr)
{
    // 设置线程池
    m_threadPool->setMaxThreadCount(m_maxThreads);
    
    // 设置配置路径
    m_settingsPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/deepinscan";
    QDir().mkpath(m_settingsPath);
    
    // 创建设置对象
    m_settings = new QSettings(m_settingsPath + "/imageprocessor.ini", QSettings::IniFormat, q);
}

DScannerImageProcessorPrivate::~DScannerImageProcessorPrivate()
{
    cleanup();
}

void DScannerImageProcessorPrivate::initialize()
{
    qCDebug(dscannerImageProcessor) << "Initializing DScannerImageProcessorPrivate";
    
    loadSettings();
    loadPresets();
    
    // 应用加载的设置
    m_threadPool->setMaxThreadCount(m_maxThreads);
    
    qCDebug(dscannerImageProcessor) << "Initialization complete - threads:" << m_maxThreads 
                                   << "memory limit:" << m_memoryLimit;
}

void DScannerImageProcessorPrivate::cleanup()
{
    qCDebug(dscannerImageProcessor) << "Cleaning up DScannerImageProcessorPrivate";
    
    // 取消所有任务
    cancelAllTasks();
    
    // 等待线程池完成
    m_threadPool->waitForDone(5000);
    
    // 清理监视器
    for (auto *watcher : m_watchers) {
        watcher->cancel();
        watcher->deleteLater();
    }
    m_watchers.clear();
    
    for (auto *watcher : m_batchWatchers) {
        watcher->cancel();
        watcher->deleteLater();
    }
    m_batchWatchers.clear();
    
    // 保存设置
    saveSettings();
    savePresets();
    
    qCDebug(dscannerImageProcessor) << "Cleanup complete";
}

void DScannerImageProcessorPrivate::submitTask(QRunnable *task)
{
    m_pendingTasks.fetchAndAddAcquire(1);
    m_threadPool->start(task);
}

void DScannerImageProcessorPrivate::cancelAllTasks()
{
    qCDebug(dscannerImageProcessor) << "Canceling all tasks";
    
    m_threadPool->clear();
    m_pendingTasks.storeRelease(0);
    
    // 取消所有Future监视器
    for (auto *watcher : m_watchers) {
        watcher->cancel();
    }
    
    for (auto *watcher : m_batchWatchers) {
        watcher->cancel();
    }
    
    QMutexLocker locker(&m_stateMutex);
    m_isProcessing = false;
}

ImageProcessingResult DScannerImageProcessorPrivate::processImageInternal(const QImage &image, 
                                                                        const QList<ImageProcessingParameters> &params)
{
    qCDebug(dscannerImageProcessor) << "Processing image internally with" << params.size() << "parameters";
    
    QElapsedTimer timer;
    timer.start();
    
    // 更新状态
    {
        QMutexLocker locker(&m_stateMutex);
        m_isProcessing = true;
    }
    
    ImageProcessingResult result(true);
    result.processedImage = image;
    
    try {
        // 按优先级排序参数
        QList<ImageProcessingParameters> sortedParams = params;
        std::sort(sortedParams.begin(), sortedParams.end(), 
                 [](const ImageProcessingParameters &a, const ImageProcessingParameters &b) {
                     return a.priority > b.priority;
                 });
        
        // 应用每个算法
        for (const ImageProcessingParameters &param : sortedParams) {
            if (!param.enabled) {
                continue;
            }
            
            QImage processedImage = applyAlgorithm(result.processedImage, param);
            if (processedImage.isNull()) {
                result.success = false;
                result.errorMessage = QString("Algorithm %1 failed").arg(static_cast<int>(param.algorithm));
                break;
            }
            
            result.processedImage = processedImage;
        }
        
        // 记录处理时间
        result.processingTime = timer.elapsed();
        
        // 更新统计信息
        m_performanceMonitor.recordImageProcessed(result.processingTime);
        
    } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = QString("Exception during processing: %1").arg(e.what());
    }
    
    // 更新状态
    {
        QMutexLocker locker(&m_stateMutex);
        m_isProcessing = false;
    }
    
    qCDebug(dscannerImageProcessor) << "Image processing completed in" << result.processingTime << "ms";
    
    return result;
}

ImageProcessingResult DScannerImageProcessorPrivate::processScanDataInternal(const QByteArray &rawData, 
                                                                           const ScanParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Processing scan data internally, size:" << rawData.size();
    
    QElapsedTimer timer;
    timer.start();
    
    ImageProcessingResult result(true);
    
    try {
        // 转换原始数据为图像
        result.processedImage = ScanDataProcessor::processScanData(rawData, params);
        
        if (result.processedImage.isNull()) {
            result.success = false;
            result.errorMessage = "Failed to convert scan data to image";
            return result;
        }
        
        // 应用自动校正
        if (params.autoColorCorrection || params.autoContrast) {
            result.processedImage = ScanDataProcessor::applyAutoCorrections(result.processedImage, params);
        }
        
        // 应用扫描模式优化
        result.processedImage = ScanDataProcessor::applyScanModeOptimizations(result.processedImage, params.scanMode);
        
        // 记录处理时间
        result.processingTime = timer.elapsed();
        
        // 更新统计信息
        m_performanceMonitor.recordImageProcessed(result.processingTime);
        
    } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = QString("Exception during scan data processing: %1").arg(e.what());
    }
    
    qCDebug(dscannerImageProcessor) << "Scan data processing completed in" << result.processingTime << "ms";
    
    return result;
}

QImage DScannerImageProcessorPrivate::applyAlgorithm(const QImage &image, const ImageProcessingParameters &params)
{
    qCDebug(dscannerImageProcessor) << "Applying algorithm:" << static_cast<int>(params.algorithm);
    
    switch (params.algorithm) {
    case ImageProcessingAlgorithm::None:
        return image;
        
    case ImageProcessingAlgorithm::Denoise:
        return ImageAlgorithms::denoise(image, params.parameters.value("strength", 50).toInt());
        
    case ImageProcessingAlgorithm::Sharpen:
        return ImageAlgorithms::sharpen(image, params.parameters.value("strength", 50).toInt());
        
    case ImageProcessingAlgorithm::BrightnessAdjust:
        return ImageAlgorithms::adjustBrightness(image, params.parameters.value("brightness", 0).toInt());
        
    case ImageProcessingAlgorithm::ContrastEnhance:
        return ImageAlgorithms::adjustContrast(image, params.parameters.value("contrast", 0).toInt());
        
    case ImageProcessingAlgorithm::GammaCorrection:
        return ImageAlgorithms::adjustGamma(image, params.parameters.value("gamma", 1.0).toDouble());
        
    case ImageProcessingAlgorithm::ColorCorrection:
        return ImageAlgorithms::colorCorrection(image, 
                                              params.parameters.value("whitePoint", QColor(255, 255, 255)).value<QColor>());
        
    case ImageProcessingAlgorithm::AutoLevel:
        return ImageAlgorithms::autoLevel(image);
        
    case ImageProcessingAlgorithm::Deskew:
        return ImageAlgorithms::deskew(image);
        
    case ImageProcessingAlgorithm::CropDetection:
        {
            QRect cropArea = ImageAlgorithms::detectCropArea(image);
            if (cropArea.isValid() && !cropArea.isEmpty()) {
                return image.copy(cropArea);
            }
            return image;
        }
        
    case ImageProcessingAlgorithm::OCRPreprocess:
        // OCR预处理通常包括去噪、二值化、倾斜校正等
        {
            QImage processed = ImageAlgorithms::denoise(image, 30);
            processed = ImageAlgorithms::deskew(processed);
            processed = ImageAlgorithms::adjustContrast(processed, 20);
            return processed;
        }
        
    default:
        qCWarning(dscannerImageProcessor) << "Unknown algorithm:" << static_cast<int>(params.algorithm);
        return image;
    }
}

void DScannerImageProcessorPrivate::onImageProcessed(const ImageProcessingResult &result)
{
    Q_Q(DScannerImageProcessor);
    
    m_pendingTasks.fetchAndSubAcquire(1);
    
    emit q->imageProcessed(result);
    
    if (m_pendingTasks.loadAcquire() == 0) {
        emit q->processingFinished();
    }
}

void DScannerImageProcessorPrivate::onBatchProcessed(const QList<ImageProcessingResult> &results)
{
    Q_Q(DScannerImageProcessor);
    
    m_pendingTasks.fetchAndSubAcquire(1);
    
    // 发送每个结果
    for (const ImageProcessingResult &result : results) {
        emit q->imageProcessed(result);
    }
    
    if (m_pendingTasks.loadAcquire() == 0) {
        emit q->processingFinished();
    }
}

void DScannerImageProcessorPrivate::loadPresets()
{
    qCDebug(dscannerImageProcessor) << "Loading presets";
    
    QString presetFile = m_settingsPath + "/presets.json";
    QFile file(presetFile);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(dscannerImageProcessor) << "No preset file found, using defaults";
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qCWarning(dscannerImageProcessor) << "Invalid preset file format";
        return;
    }
    
    QJsonObject root = doc.object();
    QMutexLocker locker(&m_presetMutex);
    
    for (auto it = root.begin(); it != root.end(); ++it) {
        QString presetName = it.key();
        QJsonArray paramArray = it.value().toArray();
        
        QList<ImageProcessingParameters> params;
        for (const QJsonValue &value : paramArray) {
            QJsonObject paramObj = value.toObject();
            
            ImageProcessingParameters param;
            param.algorithm = static_cast<ImageProcessingAlgorithm>(paramObj["algorithm"].toInt());
            param.priority = paramObj["priority"].toInt();
            param.enabled = paramObj["enabled"].toBool();
            
            // 解析参数
            QJsonObject paramMap = paramObj["parameters"].toObject();
            for (auto paramIt = paramMap.begin(); paramIt != paramMap.end(); ++paramIt) {
                param.parameters[paramIt.key()] = paramIt.value().toVariant();
            }
            
            params.append(param);
        }
        
        m_presets[presetName] = params;
    }
    
    qCDebug(dscannerImageProcessor) << "Loaded" << m_presets.size() << "presets";
}

void DScannerImageProcessorPrivate::savePresets()
{
    qCDebug(dscannerImageProcessor) << "Saving presets";
    
    QString presetFile = m_settingsPath + "/presets.json";
    QFile file(presetFile);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(dscannerImageProcessor) << "Failed to open preset file for writing";
        return;
    }
    
    QJsonObject root;
    QMutexLocker locker(&m_presetMutex);
    
    for (auto it = m_presets.begin(); it != m_presets.end(); ++it) {
        QString presetName = it.key();
        const QList<ImageProcessingParameters> &params = it.value();
        
        QJsonArray paramArray;
        for (const ImageProcessingParameters &param : params) {
            QJsonObject paramObj;
            paramObj["algorithm"] = static_cast<int>(param.algorithm);
            paramObj["priority"] = param.priority;
            paramObj["enabled"] = param.enabled;
            
            // 序列化参数
            QJsonObject paramMap;
            for (auto paramIt = param.parameters.begin(); paramIt != param.parameters.end(); ++paramIt) {
                paramMap[paramIt.key()] = QJsonValue::fromVariant(paramIt.value());
            }
            paramObj["parameters"] = paramMap;
            
            paramArray.append(paramObj);
        }
        
        root[presetName] = paramArray;
    }
    
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
    
    qCDebug(dscannerImageProcessor) << "Saved" << m_presets.size() << "presets";
}

void DScannerImageProcessorPrivate::loadSettings()
{
    qCDebug(dscannerImageProcessor) << "Loading settings";
    
    m_maxThreads = m_settings->value("performance/maxThreads", QThread::idealThreadCount()).toInt();
    m_memoryLimit = m_settings->value("performance/memoryLimit", 1024 * 1024 * 1024).toLongLong();
    
    // 验证设置
    m_maxThreads = qBound(1, m_maxThreads, QThread::idealThreadCount() * 2);
    m_memoryLimit = qMax(static_cast<qint64>(64 * 1024 * 1024), m_memoryLimit);
    
    qCDebug(dscannerImageProcessor) << "Settings loaded - threads:" << m_maxThreads 
                                   << "memory:" << m_memoryLimit;
}

void DScannerImageProcessorPrivate::saveSettings()
{
    qCDebug(dscannerImageProcessor) << "Saving settings";
    
    m_settings->setValue("performance/maxThreads", m_maxThreads);
    m_settings->setValue("performance/memoryLimit", m_memoryLimit);
    m_settings->sync();
    
    qCDebug(dscannerImageProcessor) << "Settings saved";
}

// PerformanceMonitor 实现
PerformanceMonitor::PerformanceMonitor()
{
    m_sessionTimer.start();
}

void PerformanceMonitor::startTask(const QString &taskName)
{
    QMutexLocker locker(&m_mutex);
    m_activeTasks[taskName].start();
}

void PerformanceMonitor::endTask(const QString &taskName)
{
    QMutexLocker locker(&m_mutex);
    if (m_activeTasks.contains(taskName)) {
        m_activeTasks.remove(taskName);
    }
}

void PerformanceMonitor::recordImageProcessed(qint64 processingTime)
{
    QMutexLocker locker(&m_mutex);
    m_totalProcessedImages++;
    m_totalProcessingTime += processingTime;
}

qint64 PerformanceMonitor::totalProcessedImages() const
{
    QMutexLocker locker(&m_mutex);
    return m_totalProcessedImages;
}

qint64 PerformanceMonitor::totalProcessingTime() const
{
    QMutexLocker locker(&m_mutex);
    return m_totalProcessingTime;
}

double PerformanceMonitor::averageProcessingTime() const
{
    QMutexLocker locker(&m_mutex);
    if (m_totalProcessedImages == 0) {
        return 0.0;
    }
    return static_cast<double>(m_totalProcessingTime) / m_totalProcessedImages;
}

void PerformanceMonitor::reset()
{
    QMutexLocker locker(&m_mutex);
    m_totalProcessedImages = 0;
    m_totalProcessingTime = 0;
    m_activeTasks.clear();
    m_sessionTimer.restart();
}

DSCANNER_END_NAMESPACE 