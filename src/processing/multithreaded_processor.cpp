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

#include "multithreaded_processor.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>
#include <QThread>
#include <QMutexLocker>
#include <QFutureInterface>
#include <QtConcurrent>
#include <algorithm>
#include <random>

// 系统CPU信息检测
#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sched.h>
#include <fstream>
#include <string>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <processthreadsapi.h>
#endif

#ifdef Q_OS_MACOS
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif

MultithreadedProcessor::MultithreadedProcessor(QObject *parent)
    : QObject(parent)
    , m_threadPool(new QThreadPool(this))
    , m_nextTaskId(1)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_activeTasks(0)
    , m_statsTimer(new QTimer(this))
    , m_physicalCores(1)
    , m_logicalCores(1)
    , m_hasHyperThreading(false)
{
    qDebug() << "MultithreadedProcessor::MultithreadedProcessor: 初始化多线程处理器";
    
    // 检测CPU信息
    detectCPUInfo();
    
    // 配置线程池
    m_config.maxThreadCount = qMin(m_logicalCores, 16); // 限制最大线程数
    m_config.idealThreadCount = m_physicalCores;
    
    m_threadPool->setMaxThreadCount(m_config.maxThreadCount);
    
    // 初始化线程负载统计
    m_threadLoads.resize(m_config.maxThreadCount);
    m_threadTimes.resize(m_config.maxThreadCount);
    std::fill(m_threadLoads.begin(), m_threadLoads.end(), 0);
    std::fill(m_threadTimes.begin(), m_threadTimes.end(), 0);
    
    // 设置性能统计定时器
    m_statsTimer->setSingleShot(false);
    m_statsTimer->setInterval(1000); // 每秒更新一次
    connect(m_statsTimer, &QTimer::timeout, this, &MultithreadedProcessor::updatePerformanceStats);
    
    qDebug() << "多线程处理器初始化完成 - 物理核心:" << m_physicalCores 
             << "逻辑核心:" << m_logicalCores << "最大线程数:" << m_config.maxThreadCount;
}

MultithreadedProcessor::~MultithreadedProcessor()
{
    qDebug() << "MultithreadedProcessor::~MultithreadedProcessor: 清理多线程处理器";
    
    // 停止线程池
    stopThreadPool(true);
    
    // 清理工作线程
    for (ThreadWorker *worker : m_workers) {
        worker->deleteLater();
    }
    
    for (QThread *thread : m_threads) {
        thread->quit();
        thread->wait(3000);
        thread->deleteLater();
    }
    
    qDebug() << "多线程处理器清理完成";
}

void MultithreadedProcessor::setThreadPoolConfig(const ThreadPoolConfig &config)
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MultithreadedProcessor::setThreadPoolConfig: 更新线程池配置";
    
    m_config = config;
    
    // 更新线程池设置
    m_threadPool->setMaxThreadCount(config.maxThreadCount);
    
    // 重新初始化线程负载统计
    m_threadLoads.resize(config.maxThreadCount);
    m_threadTimes.resize(config.maxThreadCount);
    std::fill(m_threadLoads.begin(), m_threadLoads.end(), 0);
    std::fill(m_threadTimes.begin(), m_threadTimes.end(), 0);
    
    qDebug() << "线程池配置更新完成，最大线程数:" << config.maxThreadCount;
}

MultithreadedProcessor::ThreadPoolConfig MultithreadedProcessor::getThreadPoolConfig() const
{
    QMutexLocker locker(&m_mutex);
    return m_config;
}

MultithreadedProcessor::PerformanceStats MultithreadedProcessor::getPerformanceStats() const
{
    QMutexLocker locker(&m_mutex);
    
    // 更新实时统计
    m_stats.activeThreads = m_threadPool->activeThreadCount();
    m_stats.queuedTasks = m_taskQueue.size();
    
    // 计算平均处理时间
    if (!m_processingTimes.isEmpty()) {
        qint64 total = 0;
        for (qint64 time : m_processingTimes) {
            total += time;
        }
        m_stats.averageProcessingTime = static_cast<double>(total) / m_processingTimes.size();
    }
    
    // 计算吞吐量（最近一秒的任务完成数）
    static qint64 lastCompletedCount = 0;
    qint64 currentCompleted = m_stats.completedTasks;
    m_stats.throughput = currentCompleted - lastCompletedCount;
    lastCompletedCount = currentCompleted;
    
    return m_stats;
}

QFuture<MultithreadedProcessor::ProcessingResult> MultithreadedProcessor::enhanceImageAsync(
    const QImage &image, const QVariantMap &params, TaskPriority priority)
{
    qDebug() << "MultithreadedProcessor::enhanceImageAsync: 提交异步图像增强任务";
    
    if (image.isNull()) {
        qWarning() << "输入图像为空";
        ProcessingResult errorResult;
        errorResult.success = false;
        errorResult.errorMessage = "输入图像为空";
        return QtConcurrent::run([errorResult]() { return errorResult; });
    }
    
    auto processor = createProcessor(Enhancement, params);
    return processImageAsync(image, processor, priority);
}

QFuture<MultithreadedProcessor::ProcessingResult> MultithreadedProcessor::scaleImageAsync(
    const QImage &image, const QSize &newSize, Qt::TransformationMode interpolation, TaskPriority priority)
{
    qDebug() << "MultithreadedProcessor::scaleImageAsync: 提交异步图像缩放任务";
    qDebug() << "原始尺寸:" << image.size() << "目标尺寸:" << newSize;
    
    if (image.isNull() || newSize.isEmpty()) {
        qWarning() << "无效的输入参数";
        ProcessingResult errorResult;
        errorResult.success = false;
        errorResult.errorMessage = "无效的输入参数";
        return QtConcurrent::run([errorResult]() { return errorResult; });
    }
    
    QVariantMap params;
    params["newSize"] = newSize;
    params["interpolation"] = static_cast<int>(interpolation);
    
    auto processor = createProcessor(Scaling, params);
    return processImageAsync(image, processor, priority);
}

QFuture<MultithreadedProcessor::ProcessingResult> MultithreadedProcessor::filterImageAsync(
    const QImage &image, const QString &filterType, const QVariantMap &params, TaskPriority priority)
{
    qDebug() << "MultithreadedProcessor::filterImageAsync: 提交异步图像滤波任务，类型:" << filterType;
    
    if (image.isNull() || filterType.isEmpty()) {
        qWarning() << "无效的输入参数";
        ProcessingResult errorResult;
        errorResult.success = false;
        errorResult.errorMessage = "无效的输入参数";
        return QtConcurrent::run([errorResult]() { return errorResult; });
    }
    
    QVariantMap filterParams = params;
    filterParams["filterType"] = filterType;
    
    auto processor = createProcessor(Filtering, filterParams);
    return processImageAsync(image, processor, priority);
}

QFuture<MultithreadedProcessor::ProcessingResult> MultithreadedProcessor::processImageAsync(
    const QImage &image, std::function<QImage(const QImage&)> processor, TaskPriority priority)
{
    qDebug() << "MultithreadedProcessor::processImageAsync: 提交异步自定义处理任务";
    
    if (image.isNull() || !processor) {
        qWarning() << "无效的输入参数";
        ProcessingResult errorResult;
        errorResult.success = false;
        errorResult.errorMessage = "无效的输入参数";
        return QtConcurrent::run([errorResult]() { return errorResult; });
    }
    
    // 创建 Future 接口
    QFutureInterface<ProcessingResult> *interface = new QFutureInterface<ProcessingResult>;
    interface->reportStarted();
    
    // 创建处理任务
    int taskId = m_nextTaskId.fetchAndAddOrdered(1);
    ProcessingTask *task = new ProcessingTask(this, taskId, Custom, priority, image, processor);
    
    // 保存 Future 接口引用
    {
        QMutexLocker locker(&m_mutex);
        m_futures[taskId] = interface;
    }
    
    // 提交任务
    submitTask(task);
    
    return interface->future();
}

QList<QFuture<MultithreadedProcessor::ProcessingResult>> MultithreadedProcessor::processBatchAsync(
    const QList<QImage> &images, std::function<QImage(const QImage&)> processor, TaskPriority priority)
{
    qDebug() << "MultithreadedProcessor::processBatchAsync: 提交批量异步处理任务，数量:" << images.size();
    
    QList<QFuture<ProcessingResult>> futures;
    
    if (images.isEmpty() || !processor) {
        qWarning() << "无效的输入参数";
        return futures;
    }
    
    // 为每个图像创建独立的异步任务
    for (const QImage &image : images) {
        if (!image.isNull()) {
            futures.append(processImageAsync(image, processor, priority));
        }
    }
    
    qDebug() << "批量任务提交完成，任务数量:" << futures.size();
    return futures;
}

QFuture<MultithreadedProcessor::ProcessingResult> MultithreadedProcessor::processPipelineAsync(
    const QImage &image, const QList<std::function<QImage(const QImage&)>> &processors, TaskPriority priority)
{
    qDebug() << "MultithreadedProcessor::processPipelineAsync: 提交管道式处理任务，步骤数:" << processors.size();
    
    if (image.isNull() || processors.isEmpty()) {
        qWarning() << "无效的输入参数";
        ProcessingResult errorResult;
        errorResult.success = false;
        errorResult.errorMessage = "无效的输入参数";
        return QtConcurrent::run([errorResult]() { return errorResult; });
    }
    
    // 创建管道处理器
    auto pipelineProcessor = [processors](const QImage &inputImage) -> QImage {
        QImage currentImage = inputImage;
        
        for (const auto &processor : processors) {
            if (processor && !currentImage.isNull()) {
                currentImage = processor(currentImage);
            } else {
                break;
            }
        }
        
        return currentImage;
    };
    
    return processImageAsync(image, pipelineProcessor, priority);
}

MultithreadedProcessor::ProcessingResult MultithreadedProcessor::enhanceImageSync(const QImage &image, const QVariantMap &params)
{
    qDebug() << "MultithreadedProcessor::enhanceImageSync: 同步图像增强";
    
    QFuture<ProcessingResult> future = enhanceImageAsync(image, params, Normal);
    future.waitForFinished();
    return future.result();
}

MultithreadedProcessor::ProcessingResult MultithreadedProcessor::scaleImageSync(const QImage &image, const QSize &newSize, Qt::TransformationMode interpolation)
{
    qDebug() << "MultithreadedProcessor::scaleImageSync: 同步图像缩放";
    
    QFuture<ProcessingResult> future = scaleImageAsync(image, newSize, interpolation, Normal);
    future.waitForFinished();
    return future.result();
}

void MultithreadedProcessor::startThreadPool()
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MultithreadedProcessor::startThreadPool: 启动线程池";
    
    if (m_isRunning) {
        qDebug() << "线程池已在运行";
        return;
    }
    
    m_isRunning = true;
    m_isPaused = false;
    
    // 启动统计定时器
    m_statsTimer->start();
    
    // 创建工作线程
    for (int i = 0; i < m_config.maxThreadCount; ++i) {
        ThreadWorker *worker = new ThreadWorker(i, this);
        QThread *thread = new QThread();
        
        worker->moveToThread(thread);
        
        // 设置线程亲和性
        if (m_config.enableThreadAffinity && i < m_coreAffinityMap.size()) {
            worker->setThreadAffinity(m_coreAffinityMap[i]);
        }
        
        connect(thread, &QThread::started, worker, &ThreadWorker::processTask);
        
        m_workers.append(worker);
        m_threads.append(thread);
        
        thread->start();
    }
    
    qDebug() << "线程池启动完成，工作线程数:" << m_workers.size();
}

void MultithreadedProcessor::stopThreadPool(bool waitForCompletion)
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MultithreadedProcessor::stopThreadPool: 停止线程池，等待完成:" << waitForCompletion;
    
    if (!m_isRunning) {
        qDebug() << "线程池未运行";
        return;
    }
    
    m_isRunning = false;
    
    // 停止统计定时器
    m_statsTimer->stop();
    
    if (waitForCompletion) {
        // 等待所有任务完成
        m_threadPool->waitForDone();
        
        // 等待队列中的任务完成
        while (!m_taskQueue.isEmpty() && m_activeTasks.load() > 0) {
            m_condition.wait(&m_mutex, 100);
        }
    } else {
        // 清空任务队列
        clearQueue();
    }
    
    // 停止工作线程
    for (QThread *thread : m_threads) {
        thread->quit();
        if (!thread->wait(3000)) {
            qWarning() << "线程停止超时，强制终止";
            thread->terminate();
        }
    }
    
    qDebug() << "线程池停止完成";
}

void MultithreadedProcessor::pauseProcessing()
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MultithreadedProcessor::pauseProcessing: 暂停处理";
    
    m_isPaused = true;
    
    qDebug() << "处理已暂停";
}

void MultithreadedProcessor::resumeProcessing()
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MultithreadedProcessor::resumeProcessing: 恢复处理";
    
    m_isPaused = false;
    m_condition.wakeAll();
    
    qDebug() << "处理已恢复";
}

void MultithreadedProcessor::clearQueue()
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MultithreadedProcessor::clearQueue: 清空任务队列";
    
    int clearedTasks = m_taskQueue.size();
    
    // 清理队列中的任务
    while (!m_taskQueue.isEmpty()) {
        ProcessingTask *task = m_taskQueue.dequeue();
        
        // 通知任务失败
        int taskId = task->getTaskId();
        if (m_futures.contains(taskId)) {
            QFutureInterface<ProcessingResult> *interface = m_futures[taskId];
            ProcessingResult result;
            result.success = false;
            result.errorMessage = "任务被取消";
            interface->reportResult(result);
            interface->reportFinished();
            delete interface;
            m_futures.remove(taskId);
        }
        
        delete task;
    }
    
    qDebug() << "清空任务队列完成，清理任务数:" << clearedTasks;
}

bool MultithreadedProcessor::waitForAll(int timeout)
{
    qDebug() << "MultithreadedProcessor::waitForAll: 等待所有任务完成，超时:" << timeout;
    
    QElapsedTimer timer;
    timer.start();
    
    while (m_activeTasks.load() > 0 || !m_taskQueue.isEmpty()) {
        if (timeout > 0 && timer.elapsed() > timeout) {
            qDebug() << "等待超时";
            return false;
        }
        
        QThread::msleep(10);
    }
    
    qDebug() << "所有任务完成，用时:" << timer.elapsed() << "毫秒";
    return true;
}

// ProcessingTask 实现
MultithreadedProcessor::ProcessingTask::ProcessingTask(MultithreadedProcessor *processor, int taskId, 
                                                      TaskType type, TaskPriority priority,
                                                      const QImage &image, std::function<QImage(const QImage&)> func)
    : m_processor(processor)
    , m_taskId(taskId)
    , m_type(type)
    , m_priority(priority)
    , m_image(image)
    , m_function(func)
{
    setAutoDelete(true);
}

void MultithreadedProcessor::ProcessingTask::run()
{
    qDebug() << "ProcessingTask::run: 执行任务" << m_taskId << "类型:" << m_type;
    
    QElapsedTimer timer;
    timer.start();
    
    ProcessingResult result;
    result.threadId = static_cast<int>(reinterpret_cast<quintptr>(QThread::currentThreadId()) % 1000);
    
    try {
        // 处理图像
        QImage processedImage = m_function(m_image);
        
        if (!processedImage.isNull()) {
            result.result = processedImage;
            result.success = true;
            result.processingTime = timer.elapsed();
        } else {
            result.success = false;
            result.errorMessage = "图像处理返回空结果";
        }
        
    } catch (const std::exception &e) {
        qCritical() << "任务处理异常:" << e.what();
        result.success = false;
        result.errorMessage = QString("处理异常: %1").arg(e.what());
    } catch (...) {
        qCritical() << "任务处理发生未知异常";
        result.success = false;
        result.errorMessage = "发生未知异常";
    }
    
    result.processingTime = timer.elapsed();
    
    // 通知处理器任务完成
    m_processor->handleTaskCompleted(m_taskId, result);
    
    qDebug() << "任务" << m_taskId << "执行完成，用时:" << result.processingTime << "毫秒，成功:" << result.success;
}

// ThreadWorker 实现
MultithreadedProcessor::ThreadWorker::ThreadWorker(int workerId, MultithreadedProcessor *processor)
    : m_workerId(workerId)
    , m_processor(processor)
    , m_assignedCore(-1)
{
    qDebug() << "ThreadWorker::ThreadWorker: 创建工作线程" << workerId;
}

void MultithreadedProcessor::ThreadWorker::setThreadAffinity(int cpuCore)
{
    m_assignedCore = cpuCore;
    
#ifdef Q_OS_LINUX
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuCore, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0) {
        qDebug() << "工作线程" << m_workerId << "设置CPU亲和性到核心" << cpuCore;
    } else {
        qWarning() << "设置CPU亲和性失败";
    }
#elif defined(Q_OS_WIN)
    DWORD_PTR mask = 1ULL << cpuCore;
    if (SetThreadAffinityMask(GetCurrentThread(), mask)) {
        qDebug() << "工作线程" << m_workerId << "设置CPU亲和性到核心" << cpuCore;
    } else {
        qWarning() << "设置CPU亲和性失败";
    }
#endif
}

void MultithreadedProcessor::ThreadWorker::processTask()
{
    qDebug() << "ThreadWorker::processTask: 工作线程" << m_workerId << "开始处理任务";
    
    while (m_processor->m_isRunning) {
        ProcessingTask *task = m_processor->getNextTask();
        
        if (task) {
            QElapsedTimer timer;
            timer.start();
            
            // 执行任务
            task->run();
            
            // 更新线程统计
            QMutexLocker locker(&m_processor->m_mutex);
            if (m_workerId < m_processor->m_threadLoads.size()) {
                m_processor->m_threadLoads[m_workerId]++;
                m_processor->m_threadTimes[m_workerId] += timer.elapsed();
            }
            
            m_processor->m_activeTasks.fetchAndSubOrdered(1);
            
        } else {
            // 没有任务，短暂休眠
            QThread::msleep(1);
        }
        
        // 检查暂停状态
        if (m_processor->m_isPaused) {
            QMutexLocker locker(&m_processor->m_mutex);
            m_processor->m_condition.wait(&m_processor->m_mutex);
        }
    }
    
    qDebug() << "工作线程" << m_workerId << "停止处理";
}

// 私有方法实现
int MultithreadedProcessor::submitTask(ProcessingTask *task)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isRunning) {
        qWarning() << "线程池未运行，无法提交任务";
        delete task;
        return -1;
    }
    
    int taskId = task->getTaskId();
    
    // 检查队列容量
    if (m_taskQueue.size() >= m_config.queueCapacity) {
        qWarning() << "任务队列已满，丢弃任务" << taskId;
        delete task;
        return -1;
    }
    
    // 添加到队列
    m_taskQueue.enqueue(task);
    m_activeTasks.fetchAndAddOrdered(1);
    
    // 唤醒等待的线程
    m_condition.wakeOne();
    
    qDebug() << "提交任务" << taskId << "到队列，当前队列大小:" << m_taskQueue.size();
    return taskId;
}

MultithreadedProcessor::ProcessingTask* MultithreadedProcessor::getNextTask()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_taskQueue.isEmpty()) {
        return nullptr;
    }
    
    // 简单的FIFO调度，可以扩展为优先级调度
    ProcessingTask *task = m_taskQueue.dequeue();
    
    return task;
}

std::function<QImage(const QImage&)> MultithreadedProcessor::createProcessor(TaskType type, const QVariantMap &params)
{
    switch (type) {
    case Enhancement:
        return [params](const QImage &image) -> QImage {
            // 简化的图像增强实现
            QImage result = image;
            double brightness = params.value("brightness", 1.0).toDouble();
            double contrast = params.value("contrast", 1.0).toDouble();
            
            // 这里应该调用实际的增强算法
            qDebug() << "应用图像增强，亮度:" << brightness << "对比度:" << contrast;
            
            return result;
        };
        
    case Scaling:
        return [params](const QImage &image) -> QImage {
            QSize newSize = params.value("newSize").toSize();
            Qt::TransformationMode mode = static_cast<Qt::TransformationMode>(
                params.value("interpolation", Qt::SmoothTransformation).toInt());
            
            if (newSize.isValid()) {
                return image.scaled(newSize, Qt::KeepAspectRatio, mode);
            }
            return image;
        };
        
    case Filtering:
        return [params](const QImage &image) -> QImage {
            QString filterType = params.value("filterType").toString();
            
            // 简化的滤波实现
            qDebug() << "应用滤波，类型:" << filterType;
            
            return image; // 这里应该实现具体的滤波算法
        };
        
    default:
        return [](const QImage &image) -> QImage {
            return image; // 默认直接返回
        };
    }
}

void MultithreadedProcessor::detectCPUInfo()
{
    qDebug() << "MultithreadedProcessor::detectCPUInfo: 检测CPU信息";
    
    // 获取逻辑核心数
    m_logicalCores = QThread::idealThreadCount();
    
#ifdef Q_OS_LINUX
    // 在Linux上获取物理核心数
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    int physicalCores = 0;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("cpu cores") != std::string::npos) {
            size_t pos = line.find(": ");
            if (pos != std::string::npos) {
                physicalCores = std::stoi(line.substr(pos + 2));
                break;
            }
        }
    }
    
    if (physicalCores > 0) {
        m_physicalCores = physicalCores;
    }
    
#elif defined(Q_OS_WIN)
    // Windows获取物理核心数
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_logicalCores = sysInfo.dwNumberOfProcessors;
    
    // 获取物理核心数需要更复杂的API调用
    m_physicalCores = m_logicalCores / 2; // 简化假设有超线程
    
#elif defined(Q_OS_MACOS)
    // macOS获取物理核心数
    size_t size = sizeof(m_physicalCores);
    sysctlbyname("hw.physicalcpu", &m_physicalCores, &size, NULL, 0);
    
    size = sizeof(m_logicalCores);
    sysctlbyname("hw.logicalcpu", &m_logicalCores, &size, NULL, 0);
#endif
    
    // 检测超线程
    m_hasHyperThreading = (m_logicalCores > m_physicalCores);
    
    // 设置核心亲和性映射
    for (int i = 0; i < m_logicalCores; ++i) {
        m_coreAffinityMap.append(i);
    }
    
    qDebug() << "CPU信息检测完成 - 物理核心:" << m_physicalCores 
             << "逻辑核心:" << m_logicalCores << "支持超线程:" << m_hasHyperThreading;
}

// 槽函数实现
void MultithreadedProcessor::updatePerformanceStats()
{
    QMutexLocker locker(&m_mutex);
    
    // 更新基本统计
    m_stats.activeThreads = m_threadPool->activeThreadCount();
    m_stats.queuedTasks = m_taskQueue.size();
    
    // 计算CPU利用率（简化计算）
    if (!m_threadTimes.isEmpty()) {
        qint64 totalTime = 0;
        for (qint64 time : m_threadTimes) {
            totalTime += time;
        }
        
        // 简化的CPU利用率计算
        m_stats.cpuUtilization = qMin(100.0, (totalTime * 100.0) / (m_logicalCores * 1000));
    }
    
    // 发送统计更新信号
    emit performanceStatsUpdated(m_stats);
}

void MultithreadedProcessor::handleTaskCompleted(int taskId, const ProcessingResult &result)
{
    {
        QMutexLocker locker(&m_mutex);
        
        // 更新统计
        if (result.success) {
            m_stats.completedTasks++;
            m_processingTimes.append(result.processingTime);
            
            // 保持处理时间历史在合理范围内
            if (m_processingTimes.size() > 1000) {
                m_processingTimes.removeFirst();
            }
        } else {
            m_stats.failedTasks++;
        }
        
        // 处理Future接口
        if (m_futures.contains(taskId)) {
            QFutureInterface<ProcessingResult> *interface = m_futures[taskId];
            interface->reportResult(result);
            interface->reportFinished();
            delete interface;
            m_futures.remove(taskId);
        }
    }
    
    // 发送信号
    if (result.success) {
        emit taskCompleted(taskId, result);
    } else {
        emit taskFailed(taskId, result.errorMessage);
    }
    
    // 更新进度
    emit progressUpdated(m_stats.completedTasks, m_stats.completedTasks + m_stats.failedTasks + m_taskQueue.size());
} 