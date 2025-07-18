#pragma once

#include <QObject>
#include <QImage>
#include <QElapsedTimer>
#include <QMutex>
#include <QAtomicInt>
#include <QHash>
#include <memory>

/**
 * @brief 性能优化器
 * 提供内存管理、性能监控和优化功能
 */
class PerformanceOptimizer : public QObject
{
    Q_OBJECT
    
public:
    explicit PerformanceOptimizer(QObject *parent = nullptr);
    ~PerformanceOptimizer();
    
    /**
     * @brief 性能统计结构
     */
    struct PerformanceStats {
        qint64 totalProcessingTime = 0;
        qint64 totalImagesProcessed = 0;
        qint64 memoryUsed = 0;
        qint64 peakMemoryUsage = 0;
        double averageProcessingTime = 0.0;
        int activeThreads = 0;
    };
    
    /**
     * @brief 设置内存使用限制
     * @param limitMB 内存限制（MB）
     */
    void setMemoryLimit(qint64 limitMB);
    
    /**
     * @brief 获取当前内存使用限制
     * @return 内存限制（MB）
     */
    qint64 memoryLimit() const;
    
    /**
     * @brief 检查图像是否需要分块处理
     * @param image 输入图像
     * @return true表示需要分块处理
     */
    bool needsTileProcessing(const QImage &image) const;
    
    /**
     * @brief 计算最优的图像分块大小
     * @param image 输入图像
     * @return 建议的分块大小
     */
    QSize calculateOptimalTileSize(const QImage &image) const;
    
    /**
     * @brief 估算图像的内存使用量
     * @param image 输入图像
     * @return 内存使用量（字节）
     */
    static qint64 estimateImageMemoryUsage(const QImage &image);
    
    /**
     * @brief 开始性能监控
     * @param operationName 操作名称
     * @return 监控ID
     */
    int startPerformanceMonitoring(const QString &operationName);
    
    /**
     * @brief 结束性能监控
     * @param monitorId 监控ID
     */
    void endPerformanceMonitoring(int monitorId);
    
    /**
     * @brief 获取性能统计
     * @return 性能统计数据
     */
    PerformanceStats getPerformanceStats() const;
    
    /**
     * @brief 重置性能统计
     */
    void resetPerformanceStats();
    
    /**
     * @brief 优化图像格式以提高处理速度
     * @param image 输入图像
     * @return 优化后的图像
     */
    static QImage optimizeImageFormat(const QImage &image);
    
    /**
     * @brief 检查系统资源可用性
     * @return true表示系统资源充足
     */
    bool checkSystemResources() const;
    
signals:
    /**
     * @brief 内存使用量变化信号
     * @param usedMB 当前使用的内存（MB）
     * @param totalMB 总内存限制（MB）
     */
    void memoryUsageChanged(qint64 usedMB, qint64 totalMB);
    
    /**
     * @brief 性能警告信号
     * @param message 警告信息
     */
    void performanceWarning(const QString &message);

private:
    struct MonitoringSession {
        QString operationName;
        QElapsedTimer timer;
        qint64 startMemory;
    };
    
    mutable QMutex m_mutex;
    qint64 m_memoryLimit;          // 内存限制（字节）
    QAtomicInt m_nextMonitorId;    // 下一个监控ID
    QHash<int, MonitoringSession> m_activeSessions; // 活动的监控会话
    
    // 性能统计
    mutable PerformanceStats m_stats;
    
    /**
     * @brief 更新内存使用统计
     */
    void updateMemoryStats();
    
    /**
     * @brief 获取当前进程内存使用量
     * @return 内存使用量（字节）
     */
    static qint64 getCurrentMemoryUsage();
}; 