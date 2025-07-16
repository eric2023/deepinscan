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
#include <QRect>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <memory>
#include <vector>
#include <deque>

/**
 * @brief 智能内存池管理器
 * 
 * 为图像处理提供高效的内存分配和回收机制，减少内存碎片化
 * 和频繁的内存分配开销。
 */
class MemoryPool
{
public:
    explicit MemoryPool(size_t initialSize = 64 * 1024 * 1024); // 64MB初始大小
    ~MemoryPool();
    
    /**
     * @brief 分配对齐的内存块
     * @param size 需要的内存大小
     * @param alignment 内存对齐要求（默认32字节用于AVX2）
     * @return 分配的内存指针，失败返回nullptr
     */
    void* allocate(size_t size, size_t alignment = 32);
    
    /**
     * @brief 释放内存块
     * @param ptr 要释放的内存指针
     */
    void deallocate(void* ptr);
    
    /**
     * @brief 获取内存池统计信息
     */
    struct Statistics {
        size_t totalAllocated = 0;     // 总分配字节数
        size_t totalFreed = 0;         // 总释放字节数
        size_t currentUsage = 0;       // 当前使用量
        size_t poolSize = 0;           // 内存池总大小
        size_t allocationCount = 0;    // 分配次数
        size_t deallocationCount = 0;  // 释放次数
        double fragmentationRatio = 0.0; // 碎片化率
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief 压缩内存池，减少碎片化
     */
    void compact();
    
    /**
     * @brief 重置内存池
     */
    void reset();

private:
    struct MemoryBlock {
        void* ptr;
        size_t size;
        bool inUse;
        size_t alignment;
        
        MemoryBlock(void* p, size_t s, bool used = false, size_t align = 1) 
            : ptr(p), size(s), inUse(used), alignment(align) {}
    };
    
    mutable QMutex m_mutex;
    std::vector<MemoryBlock> m_blocks;
    void* m_poolMemory;
    size_t m_poolSize;
    size_t m_nextOffset;
    
    Statistics m_stats;
    
    // 内存池增长策略
    bool expandPool(size_t minSize);
    void* allocateAligned(size_t size, size_t alignment);
    MemoryBlock* findFreeBlock(size_t size, size_t alignment);
    void mergeAdjacentFreeBlocks();
};

/**
 * @brief 大图像分块处理器
 * 
 * 将大图像分割成小块进行处理，降低内存使用峰值，
 * 支持多线程并行处理各个分块。
 */
class TileProcessor
{
public:
    struct TileInfo {
        QRect region;           // 分块在原图中的区域
        int tileIndex;          // 分块索引
        QSize originalSize;     // 原图尺寸
        int overlap;            // 重叠像素数（处理边界效应）
    };
    
    /**
     * @brief 构造函数
     * @param maxTileSize 单个分块的最大尺寸
     * @param overlap 分块间的重叠像素数
     */
    explicit TileProcessor(const QSize &maxTileSize = QSize(512, 512), int overlap = 16);
    
    /**
     * @brief 计算图像的分块方案
     * @param imageSize 原图尺寸
     * @return 分块信息列表
     */
    QList<TileInfo> calculateTiles(const QSize &imageSize) const;
    
    /**
     * @brief 从原图提取分块
     * @param image 原图
     * @param tileInfo 分块信息
     * @return 提取的分块图像
     */
    QImage extractTile(const QImage &image, const TileInfo &tileInfo) const;
    
    /**
     * @brief 将处理后的分块合并回原图
     * @param tiles 处理后的分块列表
     * @param tileInfos 对应的分块信息
     * @param outputSize 输出图像尺寸
     * @return 合并后的完整图像
     */
    QImage mergeTiles(const QList<QImage> &tiles, const QList<TileInfo> &tileInfos, const QSize &outputSize) const;
    
    /**
     * @brief 设置分块处理参数
     */
    void setMaxTileSize(const QSize &size) { m_maxTileSize = size; }
    void setOverlap(int overlap) { m_overlap = overlap; }
    
    QSize maxTileSize() const { return m_maxTileSize; }
    int overlap() const { return m_overlap; }

private:
    QSize m_maxTileSize;
    int m_overlap;
    
    // 处理重叠区域的边界融合
    void blendOverlapRegion(QImage &target, const QImage &source, const QRect &region, int overlap) const;
};

/**
 * @brief 内存优化的图像处理器
 * 
 * 集成内存池管理和分块处理，为大图像提供高效的内存管理方案。
 */
class DSCANNER_EXPORT MemoryOptimizedProcessor : public QObject
{
    Q_OBJECT
    
public:
    explicit MemoryOptimizedProcessor(QObject *parent = nullptr);
    ~MemoryOptimizedProcessor() override;
    
    /**
     * @brief 处理大图像（自动分块）
     * @param image 输入图像
     * @param processor 图像处理函数
     * @return 处理后的图像
     */
    template<typename ProcessorFunc>
    QImage processLargeImage(const QImage &image, ProcessorFunc processor);
    
    /**
     * @brief 设置内存使用限制
     * @param limitMB 内存限制（MB）
     */
    void setMemoryLimit(int limitMB);
    
    /**
     * @brief 获取当前内存使用情况
     * @return 内存使用统计
     */
    MemoryPool::Statistics getMemoryStatistics() const;
    
    /**
     * @brief 优化内存使用
     * @return true表示优化成功
     */
    bool optimizeMemoryUsage();
    
    /**
     * @brief 预分配图像处理所需的内存
     * @param imageSize 图像尺寸
     * @param bytesPerPixel 每像素字节数
     * @return true表示预分配成功
     */
    bool preallocateMemory(const QSize &imageSize, int bytesPerPixel = 4);
    
    /**
     * @brief 批量处理图像（内存优化）
     * @param images 输入图像列表
     * @param processor 处理函数
     * @return 处理后的图像列表
     */
    template<typename ProcessorFunc>
    QList<QImage> processBatch(const QList<QImage> &images, ProcessorFunc processor);
    
    /**
     * @brief 处理配置
     */
    struct Config {
        int memoryLimitMB = 512;           // 内存使用限制（MB）
        QSize maxTileSize = QSize(512, 512); // 最大分块尺寸
        int tileOverlap = 16;              // 分块重叠像素
        bool enableMemoryPool = true;      // 启用内存池
        bool enableTileProcessing = true;  // 启用分块处理
        int poolInitialSizeMB = 64;        // 内存池初始大小（MB）
        double fragmentationThreshold = 0.3; // 碎片化阈值
    };
    
    void setConfig(const Config &config);
    Config getConfig() const { return m_config; }

signals:
    void memoryUsageChanged(int usedMB, int limitMB);
    void processingProgress(int current, int total);
    void memoryOptimizationCompleted(bool success);

private slots:
    void onMemoryUsageHigh();
    void onMemoryOptimizationNeeded();

private:
    /**
     * @brief 检查是否需要分块处理
     * @param imageSize 图像尺寸
     * @return true表示需要分块处理
     */
    bool shouldUseTileProcessing(const QSize &imageSize) const;
    
    /**
     * @brief 估算图像处理的内存需求
     * @param imageSize 图像尺寸
     * @param bytesPerPixel 每像素字节数
     * @return 估算的内存需求（字节）
     */
    size_t estimateMemoryRequirement(const QSize &imageSize, int bytesPerPixel) const;
    
    /**
     * @brief 自动调整分块大小
     * @param imageSize 原图尺寸
     * @return 优化后的分块尺寸
     */
    QSize calculateOptimalTileSize(const QSize &imageSize) const;
    
    /**
     * @brief 监控内存使用情况
     */
    void monitorMemoryUsage();
    
    std::unique_ptr<MemoryPool> m_memoryPool;
    std::unique_ptr<TileProcessor> m_tileProcessor;
    Config m_config;
    
    QMutex m_configMutex;
    QAtomicInt m_currentMemoryUsage;  // 当前内存使用量（MB）
    QElapsedTimer m_monitorTimer;
    
    // 性能统计
    struct PerformanceStats {
        int totalProcessedImages = 0;
        qint64 totalProcessingTime = 0;
        int memoryOptimizationCount = 0;
        double averageMemoryUsage = 0.0;
    } m_stats;
};

// 模板方法实现
template<typename ProcessorFunc>
QImage MemoryOptimizedProcessor::processLargeImage(const QImage &image, ProcessorFunc processor)
{
    if (image.isNull()) {
        return QImage();
    }
    
    // 检查是否需要分块处理
    if (!shouldUseTileProcessing(image.size())) {
        // 直接处理小图像
        return processor(image);
    }
    
    qDebug() << "使用分块处理大图像，尺寸:" << image.size();
    
    // 计算分块方案
    QList<TileProcessor::TileInfo> tiles = m_tileProcessor->calculateTiles(image.size());
    QList<QImage> processedTiles;
    processedTiles.reserve(tiles.size());
    
    // 处理各个分块
    for (int i = 0; i < tiles.size(); ++i) {
        const auto &tileInfo = tiles[i];
        
        // 提取分块
        QImage tile = m_tileProcessor->extractTile(image, tileInfo);
        
        if (!tile.isNull()) {
            // 处理分块
            QImage processedTile = processor(tile);
            processedTiles.append(processedTile);
            
            // 发送进度信号
            emit processingProgress(i + 1, tiles.size());
        } else {
            processedTiles.append(QImage());
        }
    }
    
    // 合并分块
    QImage result = m_tileProcessor->mergeTiles(processedTiles, tiles, image.size());
    
    qDebug() << "分块处理完成，输出尺寸:" << result.size();
    return result;
}

template<typename ProcessorFunc>
QList<QImage> MemoryOptimizedProcessor::processBatch(const QList<QImage> &images, ProcessorFunc processor)
{
    QList<QImage> results;
    results.reserve(images.size());
    
    for (int i = 0; i < images.size(); ++i) {
        const QImage &image = images[i];
        
        if (!image.isNull()) {
            QImage result = processLargeImage(image, processor);
            results.append(result);
        } else {
            results.append(QImage());
        }
        
        // 定期优化内存使用
        if ((i + 1) % 5 == 0) {
            optimizeMemoryUsage();
        }
        
        emit processingProgress(i + 1, images.size());
    }
    
    return results;
} 