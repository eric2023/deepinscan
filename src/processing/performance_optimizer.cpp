#include "performance_optimizer.h"
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QMutexLocker>
#include <QtMath>
#include <QRegularExpression>
#include <QTextStream>
#include <QFile>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/resource.h>
#endif

PerformanceOptimizer::PerformanceOptimizer(QObject *parent)
    : QObject(parent)
    , m_memoryLimit(1024 * 1024 * 1024) // 默认1GB限制
    , m_nextMonitorId(1)
{
    // 初始化性能统计
    resetPerformanceStats();
}

PerformanceOptimizer::~PerformanceOptimizer()
{
}

void PerformanceOptimizer::setMemoryLimit(qint64 limitMB)
{
    QMutexLocker locker(&m_mutex);
    m_memoryLimit = limitMB * 1024 * 1024; // 转换为字节
    qDebug() << "Memory limit set to" << limitMB << "MB";
}

qint64 PerformanceOptimizer::memoryLimit() const
{
    QMutexLocker locker(&m_mutex);
    return m_memoryLimit / (1024 * 1024); // 转换为MB
}

bool PerformanceOptimizer::needsTileProcessing(const QImage &image) const
{
    qint64 imageMemory = estimateImageMemoryUsage(image);
    QMutexLocker locker(&m_mutex);
    
    // 如果图像内存使用超过限制的50%，则需要分块处理
    return imageMemory > (m_memoryLimit / 2);
}

QSize PerformanceOptimizer::calculateOptimalTileSize(const QImage &image) const
{
    QMutexLocker locker(&m_mutex);
    
    // 计算可用内存
    qint64 availableMemory = m_memoryLimit / 4; // 使用25%的内存限制作为单个分块的限制
    
    // 计算单个像素的内存使用
    int bytesPerPixel = image.depth() / 8;
    if (bytesPerPixel == 0) bytesPerPixel = 4; // 默认32位
    
    // 计算可以容纳的最大像素数
    qint64 maxPixels = availableMemory / bytesPerPixel;
    
    // 计算正方形分块的边长
    int tileSize = static_cast<int>(qSqrt(maxPixels));
    
    // 限制分块大小在合理范围内
    tileSize = qBound(256, tileSize, 2048);
    
    // 确保分块大小不超过原图像大小
    int maxWidth = qMin(tileSize, image.width());
    int maxHeight = qMin(tileSize, image.height());
    
    qDebug() << "Calculated optimal tile size:" << QSize(maxWidth, maxHeight)
             << "for image" << image.size()
             << "with" << bytesPerPixel << "bytes per pixel";
    
    return QSize(maxWidth, maxHeight);
}

qint64 PerformanceOptimizer::estimateImageMemoryUsage(const QImage &image)
{
    if (image.isNull()) return 0;
    
    // 计算图像的基本内存使用
    qint64 basicMemory = static_cast<qint64>(image.width()) * image.height() * (image.depth() / 8);
    
    // 考虑可能的额外开销（如格式转换、临时缓冲区等）
    qint64 overheadMemory = basicMemory * 2; // 假设200%的开销
    
    return basicMemory + overheadMemory;
}

int PerformanceOptimizer::startPerformanceMonitoring(const QString &operationName)
{
    QMutexLocker locker(&m_mutex);
    
    int monitorId = m_nextMonitorId.fetchAndAddAcquire(1);
    
    MonitoringSession session;
    session.operationName = operationName;
    session.timer.start();
    session.startMemory = getCurrentMemoryUsage();
    
    m_activeSessions[monitorId] = session;
    
    qDebug() << "Started performance monitoring for" << operationName 
             << "with ID" << monitorId;
    
    return monitorId;
}

void PerformanceOptimizer::endPerformanceMonitoring(int monitorId)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_activeSessions.find(monitorId);
    if (it == m_activeSessions.end()) {
        qWarning() << "Invalid monitoring ID:" << monitorId;
        return;
    }
    
    MonitoringSession session = it.value();
    qint64 elapsedTime = session.timer.elapsed();
    qint64 endMemory = getCurrentMemoryUsage();
    qint64 memoryDelta = endMemory - session.startMemory;
    
    // 更新性能统计
    m_stats.totalProcessingTime += elapsedTime;
    m_stats.totalImagesProcessed++;
    m_stats.memoryUsed = endMemory;
    if (endMemory > m_stats.peakMemoryUsage) {
        m_stats.peakMemoryUsage = endMemory;
    }
    
    if (m_stats.totalImagesProcessed > 0) {
        m_stats.averageProcessingTime = static_cast<double>(m_stats.totalProcessingTime) / m_stats.totalImagesProcessed;
    }
    
    qDebug() << "Finished monitoring" << session.operationName
             << "- Time:" << elapsedTime << "ms"
             << "Memory delta:" << (memoryDelta / 1024) << "KB";
    
    // 检查是否需要发出警告
    if (elapsedTime > 5000) { // 超过5秒
        emit performanceWarning(QString("Slow operation: %1 took %2ms")
                               .arg(session.operationName).arg(elapsedTime));
    }
    
    if (memoryDelta > (50 * 1024 * 1024)) { // 超过50MB
        emit performanceWarning(QString("High memory usage: %1 used %2MB")
                               .arg(session.operationName).arg(memoryDelta / 1024 / 1024));
    }
    
    m_activeSessions.erase(it);
    updateMemoryStats();
}

PerformanceOptimizer::PerformanceStats PerformanceOptimizer::getPerformanceStats() const
{
    QMutexLocker locker(&m_mutex);
    PerformanceStats stats = m_stats;
    stats.activeThreads = m_activeSessions.size();
    return stats;
}

void PerformanceOptimizer::resetPerformanceStats()
{
    QMutexLocker locker(&m_mutex);
    m_stats = PerformanceStats();
    qDebug() << "Performance statistics reset";
}

QImage PerformanceOptimizer::optimizeImageFormat(const QImage &image)
{
    if (image.isNull()) return image;
    
    // 选择最优的图像格式以提高处理速度
    QImage::Format optimalFormat = QImage::Format_ARGB32;
    
    // 如果是灰度图像，使用灰度格式
    if (image.isGrayscale()) {
        optimalFormat = QImage::Format_Grayscale8;
    }
    // 如果不需要透明度，使用RGB32
    else if (!image.hasAlphaChannel()) {
        optimalFormat = QImage::Format_RGB32;
    }
    
    // 如果已经是最优格式，直接返回
    if (image.format() == optimalFormat) {
        return image;
    }
    
    // 转换格式
    QImage optimized = image.convertToFormat(optimalFormat);
    
    qDebug() << "Optimized image format from" << image.format() 
             << "to" << optimalFormat
             << "Size:" << image.size();
    
    return optimized;
}

bool PerformanceOptimizer::checkSystemResources() const
{
    // 检查可用内存
    qint64 currentMemory = getCurrentMemoryUsage();
    QMutexLocker locker(&m_mutex);
    
    bool memoryOk = currentMemory < (m_memoryLimit * 0.8); // 不超过80%限制
    
    // 检查CPU使用率（简化实现）
    int cpuCores = QThread::idealThreadCount();
    bool cpuOk = (m_activeSessions.size() < cpuCores * 2); // 不超过CPU核心数的2倍
    
    bool resourcesOk = memoryOk && cpuOk;
    
    if (!resourcesOk) {
        qDebug() << "System resources check:"
                 << "Memory OK:" << memoryOk
                 << "CPU OK:" << cpuOk
                 << "Current memory:" << (currentMemory / 1024 / 1024) << "MB"
                 << "Active sessions:" << m_activeSessions.size();
    }
    
    return resourcesOk;
}

void PerformanceOptimizer::updateMemoryStats()
{
    qint64 currentMemory = getCurrentMemoryUsage();
    m_stats.memoryUsed = currentMemory;
    
    if (currentMemory > m_stats.peakMemoryUsage) {
        m_stats.peakMemoryUsage = currentMemory;
    }
    
    // 发出内存使用变化信号
    emit memoryUsageChanged(currentMemory / 1024 / 1024, m_memoryLimit / 1024 / 1024);
}

qint64 PerformanceOptimizer::getCurrentMemoryUsage()
{
#ifdef Q_OS_LINUX
    // 在Linux上读取/proc/self/status获取内存使用情况
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("VmRSS:")) {
                // 提取数字部分（单位为kB）
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    bool ok;
                    qint64 memoryKB = parts[1].toLongLong(&ok);
                    if (ok) {
                        return memoryKB * 1024; // 转换为字节
                    }
                }
            }
        }
    }
#endif
    
    // 如果无法获取实际内存使用，返回估算值
    return 50 * 1024 * 1024; // 50MB的默认估算
} 