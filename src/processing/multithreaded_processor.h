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

#include <QObject>
#include <QImage>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <functional>

// 前向声明
class MultithreadedProcessor;

/**
 * @brief 线程工作器类
 */
class ThreadWorker : public QObject
{
    Q_OBJECT
public:
    explicit ThreadWorker(int workerId, MultithreadedProcessor *processor);
    void setThreadAffinity(int cpuCore);
    
public slots:
    void processTask();
    
private:
    int m_workerId;
    MultithreadedProcessor *m_processor;
    int m_assignedCore;
};

/**
 * @brief MultithreadedProcessor 多线程优化的图像处理器
 * 
 * 专门为高性能图像处理设计的多线程处理器，包含以下特性：
 * - 智能线程调度
 * - 负载均衡
 * - 任务队列管理
 * - 并行管道处理
 * - 线程池优化
 * - 内存友好的并发处理
 */
class MultithreadedProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 处理任务类型
     */
    enum TaskType {
        Enhancement,    ///< 图像增强
        Filtering,      ///< 图像滤波
        Scaling,        ///< 图像缩放
        Rotation,       ///< 图像旋转
        ColorConversion,///< 色彩空间转换
        Denoising,      ///< 降噪处理
        Custom          ///< 自定义处理
    };
    
    /**
     * @brief 任务优先级
     */
    enum TaskPriority {
        Low = 0,        ///< 低优先级
        Normal = 1,     ///< 普通优先级
        High = 2,       ///< 高优先级
        Critical = 3    ///< 关键优先级
    };
    
    /**
     * @brief 处理结果结构
     */
    struct ProcessingResult {
        QImage result;              ///< 处理结果图像
        bool success;               ///< 是否成功
        QString errorMessage;       ///< 错误消息
        qint64 processingTime;      ///< 处理时间（毫秒）
        int threadId;               ///< 处理线程ID
        
        ProcessingResult() : success(false), processingTime(0), threadId(-1) {}
    };
    
    /**
     * @brief 线程池配置
     */
    struct ThreadPoolConfig {
        int maxThreadCount;         ///< 最大线程数
        int idealThreadCount;       ///< 理想线程数
        int queueCapacity;          ///< 队列容量
        bool enableLoadBalancing;   ///< 启用负载均衡
        bool enableThreadAffinity;  ///< 启用线程亲和性
        int threadStackSize;        ///< 线程栈大小
        
        ThreadPoolConfig()
            : maxThreadCount(QThread::idealThreadCount())
            , idealThreadCount(QThread::idealThreadCount())
            , queueCapacity(100)
            , enableLoadBalancing(true)
            , enableThreadAffinity(false)
            , threadStackSize(0) {}
    };
    
    /**
     * @brief 性能统计
     */
    struct PerformanceStats {
        int activeThreads;          ///< 活跃线程数
        int queuedTasks;            ///< 排队任务数
        int completedTasks;         ///< 已完成任务数
        int failedTasks;            ///< 失败任务数
        double averageProcessingTime; ///< 平均处理时间
        double throughput;          ///< 吞吐量（任务/秒）
        double cpuUtilization;      ///< CPU利用率
        qint64 totalMemoryUsage;    ///< 总内存使用
        
        PerformanceStats()
            : activeThreads(0), queuedTasks(0), completedTasks(0), failedTasks(0)
            , averageProcessingTime(0.0), throughput(0.0), cpuUtilization(0.0)
            , totalMemoryUsage(0) {}
    };

public:
    explicit MultithreadedProcessor(QObject *parent = nullptr);
    ~MultithreadedProcessor();
    
    /**
     * @brief 设置线程池配置
     * @param config 配置参数
     */
    void setThreadPoolConfig(const ThreadPoolConfig &config);
    
    /**
     * @brief 获取线程池配置
     * @return 配置参数
     */
    ThreadPoolConfig getThreadPoolConfig() const;
    
    /**
     * @brief 获取性能统计信息
     * @return 性能统计
     */
    PerformanceStats getPerformanceStats() const;

    // 异步处理方法
    /**
     * @brief 异步图像增强
     * @param image 输入图像
     * @param params 增强参数
     * @param priority 任务优先级
     * @return Future对象
     */
    QFuture<ProcessingResult> enhanceImageAsync(const QImage &image, 
                                               const QVariantMap &params = QVariantMap(),
                                               TaskPriority priority = Normal);
    
    /**
     * @brief 异步图像缩放
     * @param image 输入图像
     * @param newSize 新尺寸
     * @param interpolation 插值方法
     * @param priority 任务优先级
     * @return Future对象
     */
    QFuture<ProcessingResult> scaleImageAsync(const QImage &image, const QSize &newSize,
                                             Qt::TransformationMode interpolation = Qt::SmoothTransformation,
                                             TaskPriority priority = Normal);
    
    /**
     * @brief 异步图像滤波
     * @param image 输入图像
     * @param filterType 滤波类型
     * @param params 滤波参数
     * @param priority 任务优先级
     * @return Future对象
     */
    QFuture<ProcessingResult> filterImageAsync(const QImage &image, const QString &filterType,
                                              const QVariantMap &params = QVariantMap(),
                                              TaskPriority priority = Normal);
    
    /**
     * @brief 异步自定义处理
     * @param image 输入图像
     * @param processor 处理函数
     * @param priority 任务优先级
     * @return Future对象
     */
    QFuture<ProcessingResult> processImageAsync(const QImage &image,
                                               std::function<QImage(const QImage&)> processor,
                                               TaskPriority priority = Normal);

    // 批量处理方法
    /**
     * @brief 批量异步处理
     * @param images 输入图像列表
     * @param processor 处理函数
     * @param priority 任务优先级
     * @return Future对象列表
     */
    QList<QFuture<ProcessingResult>> processBatchAsync(const QList<QImage> &images,
                                                       std::function<QImage(const QImage&)> processor,
                                                       TaskPriority priority = Normal);
    
    /**
     * @brief 管道式处理
     * @param image 输入图像
     * @param processors 处理函数链
     * @param priority 任务优先级
     * @return Future对象
     */
    QFuture<ProcessingResult> processPipelineAsync(const QImage &image,
                                                   const QList<std::function<QImage(const QImage&)>> &processors,
                                                   TaskPriority priority = Normal);

    // 同步处理方法（基于异步实现）
    /**
     * @brief 同步图像增强
     * @param image 输入图像
     * @param params 增强参数
     * @return 处理结果
     */
    ProcessingResult enhanceImageSync(const QImage &image, const QVariantMap &params = QVariantMap());
    
    /**
     * @brief 同步图像缩放
     * @param image 输入图像
     * @param newSize 新尺寸
     * @param interpolation 插值方法
     * @return 处理结果
     */
    ProcessingResult scaleImageSync(const QImage &image, const QSize &newSize,
                                   Qt::TransformationMode interpolation = Qt::SmoothTransformation);

    // 线程池管理
    /**
     * @brief 启动线程池
     */
    void startThreadPool();
    
    /**
     * @brief 停止线程池
     * @param waitForCompletion 是否等待所有任务完成
     */
    void stopThreadPool(bool waitForCompletion = true);
    
    /**
     * @brief 暂停处理
     */
    void pauseProcessing();
    
    /**
     * @brief 恢复处理
     */
    void resumeProcessing();
    
    /**
     * @brief 清空任务队列
     */
    void clearQueue();
    
    /**
     * @brief 等待所有任务完成
     * @param timeout 超时时间（毫秒，-1表示无限等待）
     * @return 是否在超时前完成
     */
    bool waitForAll(int timeout = -1);

signals:
    /**
     * @brief 任务完成信号
     * @param taskId 任务ID
     * @param result 处理结果
     */
    void taskCompleted(int taskId, const ProcessingResult &result);
    
    /**
     * @brief 任务失败信号
     * @param taskId 任务ID
     * @param error 错误消息
     */
    void taskFailed(int taskId, const QString &error);
    
    /**
     * @brief 进度更新信号
     * @param completed 已完成任务数
     * @param total 总任务数
     */
    void progressUpdated(int completed, int total);
    
    /**
     * @brief 性能统计更新信号
     * @param stats 性能统计
     */
    void performanceStatsUpdated(const PerformanceStats &stats);

private:
    /**
     * @brief 处理任务类
     */
    class ProcessingTask : public QRunnable
    {
    public:
        ProcessingTask(MultithreadedProcessor *processor, int taskId, TaskType type, TaskPriority priority,
                      const QImage &image, std::function<QImage(const QImage&)> func);
        
        void run() override;
        
        int getTaskId() const { return m_taskId; }
        TaskPriority getPriority() const { return m_priority; }
        TaskType getType() const { return m_type; }
        
    private:
        MultithreadedProcessor *m_processor;
        int m_taskId;
        TaskType m_type;
        TaskPriority m_priority;
        QImage m_image;
        std::function<QImage(const QImage&)> m_function;
    };
    
    /**
     * @brief 任务比较器（用于优先级队列）
     */
    class TaskComparator
    {
    public:
        bool operator()(ProcessingTask *a, ProcessingTask *b) const {
            return a->getPriority() < b->getPriority();
        }
    };
    
    // ThreadWorker 现在已移至类外定义

private slots:
    /**
     * @brief 更新性能统计
     */
    void updatePerformanceStats();
    
    /**
     * @brief 处理任务完成
     * @param taskId 任务ID
     * @param result 处理结果
     */
    void handleTaskCompleted(int taskId, const ProcessingResult &result);

private:
    /**
     * @brief 提交任务到队列
     * @param task 处理任务
     * @return 任务ID
     */
    int submitTask(ProcessingTask *task);
    
    /**
     * @brief 获取下一个任务
     * @return 处理任务（如果队列为空则返回nullptr）
     */
    ProcessingTask* getNextTask();
    
    /**
     * @brief 创建处理函数
     * @param type 任务类型
     * @param params 参数
     * @return 处理函数
     */
    std::function<QImage(const QImage&)> createProcessor(TaskType type, const QVariantMap &params);
    
    /**
     * @brief 优化线程数量
     * @param imageSize 图像尺寸
     * @param taskType 任务类型
     * @return 最优线程数
     */
    int optimizeThreadCount(const QSize &imageSize, TaskType taskType) const;
    
    /**
     * @brief 检测CPU核心数和架构
     */
    void detectCPUInfo();
    
    /**
     * @brief 设置线程亲和性
     * @param threadId 线程ID
     * @param cpuCore CPU核心
     */
    void setThreadAffinity(int threadId, int cpuCore);
    
    /**
     * @brief 负载均衡调度
     * @return 最佳工作线程ID
     */
    int balanceLoad() const;

private:
    ThreadPoolConfig m_config;                          ///< 线程池配置
    mutable QMutex m_mutex;                            ///< 线程安全锁
    QWaitCondition m_condition;                        ///< 条件变量
    
    // 线程池管理
    QThreadPool *m_threadPool;                         ///< Qt线程池
    QList<ThreadWorker*> m_workers;                   ///< 工作线程列表
    QList<QThread*> m_threads;                        ///< 线程对象列表
    
    // 任务队列
    QQueue<ProcessingTask*> m_taskQueue;              ///< 任务队列
    QMap<int, QFutureInterface<ProcessingResult>*> m_futures; ///< Future接口映射
    QAtomicInt m_nextTaskId;                          ///< 下一个任务ID
    
    // 状态管理
    bool m_isRunning;                                  ///< 是否运行中
    bool m_isPaused;                                   ///< 是否暂停
    QAtomicInt m_activeTasks;                         ///< 活跃任务数
    
    // 性能统计
    mutable PerformanceStats m_stats;                 ///< 性能统计
    QTimer *m_statsTimer;                             ///< 统计更新定时器
    QList<qint64> m_processingTimes;                  ///< 处理时间历史
    
    // CPU信息
    int m_physicalCores;                              ///< 物理核心数
    int m_logicalCores;                               ///< 逻辑核心数
    bool m_hasHyperThreading;                         ///< 是否支持超线程
    QList<int> m_coreAffinityMap;                     ///< 核心亲和性映射
    
    // 负载均衡
    QList<int> m_threadLoads;                         ///< 线程负载统计
    QList<qint64> m_threadTimes;                      ///< 线程处理时间
};

// #include "multithreaded_processor.moc" 