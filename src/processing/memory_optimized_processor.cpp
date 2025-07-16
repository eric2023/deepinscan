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

#include "memory_optimized_processor.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <QtConcurrent>
#include <QMutexLocker>
#include <QPainter>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdlib>

// Linux系统内存信息
#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#include <fstream>
#include <string>
#endif

// Windows系统内存信息
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

// macOS系统内存信息
#ifdef Q_OS_MACOS
#include <mach/mach.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

// ==================== MemoryPool 实现 ====================

MemoryPool::MemoryPool(size_t initialSize)
    : m_poolMemory(nullptr)
    , m_poolSize(initialSize)
    , m_nextOffset(0)
{
    qDebug() << "MemoryPool: 初始化内存池，大小:" << (initialSize / 1024 / 1024) << "MB";
    
    // 分配对齐的内存池
    m_poolMemory = std::aligned_alloc(64, m_poolSize);  // 64字节对齐
    if (!m_poolMemory) {
        qWarning() << "MemoryPool: 初始内存池分配失败";
        m_poolSize = 0;
        return;
    }
    
    // 清零内存
    std::memset(m_poolMemory, 0, m_poolSize);
    
    // 初始化统计信息
    m_stats.poolSize = m_poolSize;
    
    qDebug() << "MemoryPool: 初始化成功，内存地址:" << m_poolMemory;
}

MemoryPool::~MemoryPool()
{
    if (m_poolMemory) {
        std::free(m_poolMemory);
        m_poolMemory = nullptr;
    }
    
    qDebug() << "MemoryPool: 析构完成，最终统计:";
    qDebug() << "  总分配:" << (m_stats.totalAllocated / 1024 / 1024) << "MB";
    qDebug() << "  总释放:" << (m_stats.totalFreed / 1024 / 1024) << "MB";
    qDebug() << "  分配次数:" << m_stats.allocationCount;
    qDebug() << "  碎片化率:" << m_stats.fragmentationRatio;
}

void* MemoryPool::allocate(size_t size, size_t alignment)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_poolMemory || size == 0) {
        return nullptr;
    }
    
    // 首先尝试找到现有的空闲块
    MemoryBlock* freeBlock = findFreeBlock(size, alignment);
    if (freeBlock) {
        freeBlock->inUse = true;
        m_stats.currentUsage += freeBlock->size;
        m_stats.allocationCount++;
        
        qDebug() << "MemoryPool: 重用现有块，大小:" << size << "字节，对齐:" << alignment;
        return freeBlock->ptr;
    }
    
    // 分配新块
    void* allocatedPtr = allocateAligned(size, alignment);
    if (allocatedPtr) {
        MemoryBlock block(allocatedPtr, size, true, alignment);
        m_blocks.push_back(block);
        
        m_stats.totalAllocated += size;
        m_stats.currentUsage += size;
        m_stats.allocationCount++;
        
        qDebug() << "MemoryPool: 分配新块，大小:" << size << "字节，地址:" << allocatedPtr;
    }
    
    return allocatedPtr;
}

void MemoryPool::deallocate(void* ptr)
{
    if (!ptr) return;
    
    QMutexLocker locker(&m_mutex);
    
    // 查找对应的内存块
    auto it = std::find_if(m_blocks.begin(), m_blocks.end(),
                          [ptr](const MemoryBlock& block) {
                              return block.ptr == ptr;
                          });
    
    if (it != m_blocks.end()) {
        it->inUse = false;
        m_stats.currentUsage -= it->size;
        m_stats.totalFreed += it->size;
        m_stats.deallocationCount++;
        
        qDebug() << "MemoryPool: 释放块，大小:" << it->size << "字节";
        
        // 定期合并相邻的空闲块
        if (m_stats.deallocationCount % 10 == 0) {
            mergeAdjacentFreeBlocks();
        }
    } else {
        qWarning() << "MemoryPool: 尝试释放未知指针:" << ptr;
    }
}

MemoryPool::Statistics MemoryPool::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    // 计算碎片化率
    size_t totalFreeSize = 0;
    int freeBlockCount = 0;
    
    for (const auto& block : m_blocks) {
        if (!block.inUse) {
            totalFreeSize += block.size;
            freeBlockCount++;
        }
    }
    
    if (totalFreeSize > 0 && freeBlockCount > 1) {
        // 碎片化率 = 空闲块数量 / 总空闲大小的理想块数量
        m_stats.fragmentationRatio = static_cast<double>(freeBlockCount) / 
                                    std::max(1.0, static_cast<double>(totalFreeSize) / (64 * 1024));
    } else {
        m_stats.fragmentationRatio = 0.0;
    }
    
    return m_stats;
}

void MemoryPool::compact()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MemoryPool: 开始内存压缩";
    
    mergeAdjacentFreeBlocks();
    
    // 移除空闲的小块（可选的激进压缩）
    auto it = std::remove_if(m_blocks.begin(), m_blocks.end(),
                            [](const MemoryBlock& block) {
                                return !block.inUse && block.size < 1024;  // 移除小于1KB的空闲块
                            });
    m_blocks.erase(it, m_blocks.end());
    
    qDebug() << "MemoryPool: 压缩完成，当前块数量:" << m_blocks.size();
}

void MemoryPool::reset()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MemoryPool: 重置内存池";
    
    m_blocks.clear();
    m_nextOffset = 0;
    
    // 重置统计信息（保留历史统计）
    size_t totalAllocated = m_stats.totalAllocated;
    size_t allocationCount = m_stats.allocationCount;
    
    m_stats = Statistics();
    m_stats.poolSize = m_poolSize;
    m_stats.totalAllocated = totalAllocated;
    m_stats.allocationCount = allocationCount;
}

void* MemoryPool::allocateAligned(size_t size, size_t alignment)
{
    // 计算对齐后的偏移
    size_t alignedOffset = (m_nextOffset + alignment - 1) & ~(alignment - 1);
    
    if (alignedOffset + size > m_poolSize) {
        // 内存池空间不足，尝试扩展
        if (!expandPool(alignedOffset + size)) {
            qWarning() << "MemoryPool: 内存池空间不足，分配失败";
            return nullptr;
        }
    }
    
    void* ptr = static_cast<char*>(m_poolMemory) + alignedOffset;
    m_nextOffset = alignedOffset + size;
    
    return ptr;
}

MemoryPool::MemoryBlock* MemoryPool::findFreeBlock(size_t size, size_t alignment)
{
    for (auto& block : m_blocks) {
        if (!block.inUse && block.size >= size && block.alignment >= alignment) {
            return &block;
        }
    }
    return nullptr;
}

void MemoryPool::mergeAdjacentFreeBlocks()
{
    if (m_blocks.size() < 2) return;
    
    // 按地址排序
    std::sort(m_blocks.begin(), m_blocks.end(),
              [](const MemoryBlock& a, const MemoryBlock& b) {
                  return a.ptr < b.ptr;
              });
    
    // 合并相邻的空闲块
    auto it = m_blocks.begin();
    while (it != m_blocks.end() && std::next(it) != m_blocks.end()) {
        auto next = std::next(it);
        
        if (!it->inUse && !next->inUse) {
            char* currentEnd = static_cast<char*>(it->ptr) + it->size;
            if (currentEnd == next->ptr) {
                // 合并块
                it->size += next->size;
                m_blocks.erase(next);
                continue;
            }
        }
        ++it;
    }
    
    qDebug() << "MemoryPool: 合并后块数量:" << m_blocks.size();
}

bool MemoryPool::expandPool(size_t minSize)
{
    size_t newSize = std::max(m_poolSize * 2, minSize);
    
    qDebug() << "MemoryPool: 扩展内存池从" << (m_poolSize / 1024 / 1024) 
             << "MB到" << (newSize / 1024 / 1024) << "MB";
    
    void* newMemory = std::aligned_alloc(64, newSize);
    if (!newMemory) {
        qWarning() << "MemoryPool: 内存池扩展失败";
        return false;
    }
    
    // 复制现有数据
    if (m_poolMemory && m_nextOffset > 0) {
        std::memcpy(newMemory, m_poolMemory, m_nextOffset);
    }
    
    // 更新指针引用
    ptrdiff_t offset = static_cast<char*>(newMemory) - static_cast<char*>(m_poolMemory);
    for (auto& block : m_blocks) {
        block.ptr = static_cast<char*>(block.ptr) + offset;
    }
    
    // 释放旧内存
    if (m_poolMemory) {
        std::free(m_poolMemory);
    }
    
    m_poolMemory = newMemory;
    m_poolSize = newSize;
    m_stats.poolSize = newSize;
    
    // 清零新分配的部分
    char* newPart = static_cast<char*>(m_poolMemory) + m_nextOffset;
    std::memset(newPart, 0, newSize - m_nextOffset);
    
    return true;
}

// ==================== TileProcessor 实现 ====================

TileProcessor::TileProcessor(const QSize &maxTileSize, int overlap)
    : m_maxTileSize(maxTileSize)
    , m_overlap(overlap)
{
    qDebug() << "TileProcessor: 初始化，最大分块尺寸:" << maxTileSize 
             << "重叠像素:" << overlap;
}

QList<TileProcessor::TileInfo> TileProcessor::calculateTiles(const QSize &imageSize) const
{
    QList<TileInfo> tiles;
    
    if (imageSize.isEmpty()) {
        return tiles;
    }
    
    qDebug() << "TileProcessor: 计算分块方案，图像尺寸:" << imageSize;
    
    int tileIndex = 0;
    
    // 计算水平和垂直方向的分块数量
    int tilesX = (imageSize.width() + m_maxTileSize.width() - 1) / m_maxTileSize.width();
    int tilesY = (imageSize.height() + m_maxTileSize.height() - 1) / m_maxTileSize.height();
    
    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            TileInfo tileInfo;
            tileInfo.tileIndex = tileIndex++;
            tileInfo.originalSize = imageSize;
            tileInfo.overlap = m_overlap;
            
            // 计算分块区域（包含重叠）
            int x = tx * m_maxTileSize.width() - (tx > 0 ? m_overlap : 0);
            int y = ty * m_maxTileSize.height() - (ty > 0 ? m_overlap : 0);
            
            int width = m_maxTileSize.width();
            int height = m_maxTileSize.height();
            
            // 添加右侧重叠
            if (tx < tilesX - 1) {
                width += m_overlap;
            }
            
            // 添加下方重叠
            if (ty < tilesY - 1) {
                height += m_overlap;
            }
            
            // 确保不超出图像边界
            width = std::min(width, imageSize.width() - x);
            height = std::min(height, imageSize.height() - y);
            
            tileInfo.region = QRect(x, y, width, height);
            tiles.append(tileInfo);
        }
    }
    
    qDebug() << "TileProcessor: 生成了" << tiles.size() << "个分块";
    return tiles;
}

QImage TileProcessor::extractTile(const QImage &image, const TileInfo &tileInfo) const
{
    if (image.isNull() || !tileInfo.region.isValid()) {
        return QImage();
    }
    
    // 确保区域在图像范围内
    QRect safeRegion = tileInfo.region.intersected(QRect(0, 0, image.width(), image.height()));
    
    if (safeRegion.isEmpty()) {
        return QImage();
    }
    
    QImage tile = image.copy(safeRegion);
    
    qDebug() << "TileProcessor: 提取分块" << tileInfo.tileIndex 
             << "区域:" << safeRegion << "尺寸:" << tile.size();
    
    return tile;
}

QImage TileProcessor::mergeTiles(const QList<QImage> &tiles, const QList<TileInfo> &tileInfos, const QSize &outputSize) const
{
    if (tiles.size() != tileInfos.size() || outputSize.isEmpty()) {
        qWarning() << "TileProcessor: 合并参数无效";
        return QImage();
    }
    
    qDebug() << "TileProcessor: 开始合并" << tiles.size() << "个分块，输出尺寸:" << outputSize;
    
    QImage result(outputSize, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    
    QPainter painter(&result);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    for (int i = 0; i < tiles.size(); ++i) {
        const QImage &tile = tiles[i];
        const TileInfo &tileInfo = tileInfos[i];
        
        if (tile.isNull()) {
            continue;
        }
        
        QRect targetRect = tileInfo.region;
        
        // 处理重叠区域的融合
        if (m_overlap > 0) {
            // 创建带透明度渐变的分块
            QImage blendedTile = tile.copy();
            blendOverlapRegion(result, blendedTile, targetRect, m_overlap);
        } else {
            // 直接绘制
            painter.drawImage(targetRect.topLeft(), tile);
        }
    }
    
    painter.end();
    
    qDebug() << "TileProcessor: 分块合并完成";
    return result;
}

void TileProcessor::blendOverlapRegion(QImage &target, const QImage &source, const QRect &region, int overlap) const
{
    if (overlap <= 0 || source.isNull() || target.isNull()) {
        // 无重叠，直接复制
        QPainter painter(&target);
        painter.drawImage(region.topLeft(), source);
        painter.end();
        return;
    }
    
    QPainter painter(&target);
    
    // 为重叠区域创建渐变蒙版
    int sourceWidth = source.width();
    int sourceHeight = source.height();
    
    // 创建alpha蒙版
    QImage alphaMask(sourceWidth, sourceHeight, QImage::Format_ARGB32);
    alphaMask.fill(qRgba(255, 255, 255, 255));
    
    // 左边缘渐变
    if (region.x() > 0) {
        for (int x = 0; x < std::min(overlap, sourceWidth); ++x) {
            int alpha = (x * 255) / overlap;
            for (int y = 0; y < sourceHeight; ++y) {
                alphaMask.setPixel(x, y, qRgba(255, 255, 255, alpha));
            }
        }
    }
    
    // 上边缘渐变
    if (region.y() > 0) {
        for (int y = 0; y < std::min(overlap, sourceHeight); ++y) {
            int alpha = (y * 255) / overlap;
            for (int x = 0; x < sourceWidth; ++x) {
                QRgb existing = alphaMask.pixel(x, y);
                int newAlpha = std::min(qAlpha(existing), alpha);
                alphaMask.setPixel(x, y, qRgba(255, 255, 255, newAlpha));
            }
        }
    }
    
    // 应用蒙版绘制
    QImage maskedSource = source.copy();
    QPainter maskPainter(&maskedSource);
    maskPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    maskPainter.drawImage(0, 0, alphaMask);
    maskPainter.end();
    
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(region.topLeft(), maskedSource);
    painter.end();
}

// ==================== MemoryOptimizedProcessor 实现 ====================

MemoryOptimizedProcessor::MemoryOptimizedProcessor(QObject *parent)
    : QObject(parent)
    , m_memoryMonitorTimer(new QTimer(this))
    , m_cacheCleanupTimer(new QTimer(this))
    , m_memoryThresholdBytes(static_cast<qint64>(m_config.memoryThreshold * 1024 * 1024 * 1024))
    , m_lastMemoryCheck(0)
    , m_isProcessing(false)
    , m_currentProgress(0)
    , m_cacheHits(0)
    , m_cacheMisses(0)
    , m_totalAllocations(0)
    , m_totalDeallocations(0)
    , m_currentMemoryUsage(0)
{
    qDebug() << "MemoryOptimizedProcessor: 初始化";
    
    // 创建内存池
    m_memoryPool = std::make_unique<MemoryPool>(m_config.poolInitialSizeMB * 1024 * 1024);
    
    // 创建分块处理器
    m_tileProcessor = std::make_unique<TileProcessor>(m_config.maxTileSize, m_config.tileOverlap);
    
    // 设置缓存大小
    m_imageCache.setMaxCost(m_config.maxCacheSize * 1024 * 1024); // 转换为字节
    m_tileCache.setMaxCost(m_config.maxCacheSize * 1024 * 1024 / 4); // 块缓存占用较少
    
    // 设置内存监控定时器
    m_memoryMonitorTimer->setSingleShot(false);
    m_memoryMonitorTimer->setInterval(5000); // 5秒检查一次
    connect(m_memoryMonitorTimer, &QTimer::timeout, this, &MemoryOptimizedProcessor::performMemoryMonitoring);
    m_memoryMonitorTimer->start();
    
    // 设置缓存清理定时器
    m_cacheCleanupTimer->setSingleShot(false);
    m_cacheCleanupTimer->setInterval(30000); // 30秒清理一次
    connect(m_cacheCleanupTimer, &QTimer::timeout, this, &MemoryOptimizedProcessor::cleanupExpiredCache);
    m_cacheCleanupTimer->start();
    
    qDebug() << "内存优化处理器初始化完成，内存阈值:" << (m_memoryThresholdBytes / 1024 / 1024) << "MB";
}

MemoryOptimizedProcessor::~MemoryOptimizedProcessor()
{
    qDebug() << "MemoryOptimizedProcessor::~MemoryOptimizedProcessor: 清理内存优化处理器";
    
    // 停止定时器
    m_memoryMonitorTimer->stop();
    m_cacheCleanupTimer->stop();
    
    // 清理缓存和内存池
    clearCache();
    
    // 输出最终统计信息
    qDebug() << "内存统计 - 总分配:" << m_totalAllocations << "总释放:" << m_totalDeallocations 
             << "缓存命中率:" << (m_cacheHits * 100.0 / qMax(1LL, m_cacheHits + m_cacheMisses)) << "%";
}

void MemoryOptimizedProcessor::setProcessingConfig(const ProcessingConfig &config)
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MemoryOptimizedProcessor::setProcessingConfig: 更新处理配置";
    
    m_config = config;
    
    // 更新内存阈值
    m_memoryThresholdBytes = static_cast<qint64>(config.memoryThreshold * 1024 * 1024 * 1024);
    
    // 更新缓存大小
    m_imageCache.setMaxCost(config.maxCacheSize * 1024 * 1024);
    m_tileCache.setMaxCost(config.maxCacheSize * 1024 * 1024 / 4);
    
    // 如果禁用内存池，清理现有池
    if (!config.enableMemoryPool) {
        m_imagePool.clear();
        m_poolBySize.clear();
    }
    
    qDebug() << "处理配置更新完成";
}

MemoryOptimizedProcessor::ProcessingConfig MemoryOptimizedProcessor::getProcessingConfig() const
{
    QMutexLocker locker(&m_mutex);
    return m_config;
}

MemoryOptimizedProcessor::MemoryStats MemoryOptimizedProcessor::getMemoryStats() const
{
    QMutexLocker locker(&m_mutex);
    
    // 更新实时统计信息
    m_memoryStats.currentUsage = getCurrentMemoryUsage();
    m_memoryStats.activeTiles = m_tileCache.size();
    m_memoryStats.cachedTiles = m_imageCache.size();
    m_memoryStats.hitRatio = m_cacheHits * 100.0 / qMax(1LL, m_cacheHits + m_cacheMisses);
    
    return m_memoryStats;
}

QImage MemoryOptimizedProcessor::enhanceImageOptimized(const QImage &image, const QVariantMap &enhanceParams)
{
    qDebug() << "MemoryOptimizedProcessor::enhanceImageOptimized: 开始内存优化的图像增强";
    
    if (image.isNull()) {
        qWarning() << "输入图像为空";
        return QImage();
    }
    
    QElapsedTimer timer;
    timer.start();
    
    m_isProcessing = true;
    m_currentOperation = "图像增强";
    emit progressUpdated(0, m_currentOperation);
    
    QImage result;
    
    try {
        // 检查是否需要分块处理
        if (shouldUseTiledProcessing(image)) {
            qDebug() << "使用分块处理模式";
            result = enhanceImageTiled(image, enhanceParams);
        } else {
            qDebug() << "使用直接处理模式";
            result = enhanceImageDirect(image, enhanceParams);
        }
        
        emit progressUpdated(100, "图像增强完成");
        
    } catch (const std::exception &e) {
        qCritical() << "图像增强过程中发生异常:" << e.what();
        emit processingError(QString("图像增强失败: %1").arg(e.what()));
        result = QImage();
    }
    
    m_isProcessing = false;
    qDebug() << "内存优化图像增强完成，用时:" << timer.elapsed() << "毫秒";
    
    return result;
}

QImage MemoryOptimizedProcessor::scaleImageOptimized(const QImage &image, const QSize &newSize, 
                                                    Qt::TransformationMode interpolation)
{
    qDebug() << "MemoryOptimizedProcessor::scaleImageOptimized: 开始内存优化的图像缩放";
    qDebug() << "原始尺寸:" << image.size() << "目标尺寸:" << newSize;
    
    if (image.isNull() || newSize.isEmpty()) {
        qWarning() << "无效的输入参数";
        return QImage();
    }
    
    QElapsedTimer timer;
    timer.start();
    
    m_isProcessing = true;
    m_currentOperation = "图像缩放";
    emit progressUpdated(0, m_currentOperation);
    
    QImage result;
    
    try {
        // 生成缓存键
        QString cacheKey = generateCacheKey("scale", {
            {"width", newSize.width()},
            {"height", newSize.height()},
            {"interpolation", static_cast<int>(interpolation)},
            {"imageSize", QString("%1x%2").arg(image.width()).arg(image.height())}
        });
        
        // 检查缓存
        if (QImage *cached = m_imageCache.object(cacheKey)) {
            qDebug() << "从缓存获取缩放结果";
            m_cacheHits++;
            emit progressUpdated(100, "从缓存获取完成");
            return *cached;
        }
        
        m_cacheMisses++;
        
        // 检查是否需要分块处理
        qint64 originalMemory = estimateImageMemoryUsage(image.size(), image.format());
        qint64 targetMemory = estimateImageMemoryUsage(newSize, image.format());
        qint64 totalMemory = originalMemory + targetMemory;
        
        if (totalMemory > m_memoryThresholdBytes / 2) {
            qDebug() << "使用分块缩放模式，预估内存:" << (totalMemory / 1024 / 1024) << "MB";
            result = scaleImageTiled(image, newSize, interpolation);
        } else {
            qDebug() << "使用直接缩放模式";
            result = scaleImageDirect(image, newSize, interpolation);
        }
        
        // 将结果添加到缓存
        if (!result.isNull()) {
            qint64 resultSize = estimateImageMemoryUsage(result.size(), result.format());
            m_imageCache.insert(cacheKey, new QImage(result), resultSize);
            updateMemoryStats(resultSize, 0);
        }
        
        emit progressUpdated(100, "图像缩放完成");
        
    } catch (const std::exception &e) {
        qCritical() << "图像缩放过程中发生异常:" << e.what();
        emit processingError(QString("图像缩放失败: %1").arg(e.what()));
        result = QImage();
    }
    
    m_isProcessing = false;
    qDebug() << "内存优化图像缩放完成，用时:" << timer.elapsed() << "毫秒";
    
    return result;
}

QList<MemoryOptimizedProcessor::ImageTile> MemoryOptimizedProcessor::splitImageIntoTiles(
    const QImage &image, const QSize &tileSize, int overlap)
{
    qDebug() << "MemoryOptimizedProcessor::splitImageIntoTiles: 分割图像为块";
    qDebug() << "图像尺寸:" << image.size() << "块尺寸:" << tileSize << "重叠:" << overlap;
    
    QList<ImageTile> tiles;
    
    if (image.isNull() || tileSize.isEmpty()) {
        qWarning() << "无效的输入参数";
        return tiles;
    }
    
    const int imageWidth = image.width();
    const int imageHeight = image.height();
    const int tileWidth = tileSize.width();
    const int tileHeight = tileSize.height();
    
    int tileId = 0;
    
    // 计算需要的块数量
    int tilesX = qCeil(static_cast<double>(imageWidth) / (tileWidth - overlap));
    int tilesY = qCeil(static_cast<double>(imageHeight) / (tileHeight - overlap));
    
    qDebug() << "计算得到块数量:" << tilesX << "x" << tilesY << "=" << (tilesX * tilesY);
    
    for (int tileY = 0; tileY < tilesY; ++tileY) {
        for (int tileX = 0; tileX < tilesX; ++tileX) {
            // 计算块的位置和尺寸
            int x = tileX * (tileWidth - overlap);
            int y = tileY * (tileHeight - overlap);
            
            int w = qMin(tileWidth, imageWidth - x);
            int h = qMin(tileHeight, imageHeight - y);
            
            if (w <= 0 || h <= 0) {
                continue;
            }
            
            QRect region(x, y, w, h);
            
            // 创建图像块
            ImageTile tile(region, tileId++);
            tile.data = image.copy(region);
            
            if (!tile.data.isNull()) {
                tiles.append(tile);
                updateMemoryStats(estimateImageMemoryUsage(tile.data.size(), tile.data.format()), 0);
            }
        }
    }
    
    qDebug() << "图像分割完成，实际创建块数量:" << tiles.size();
    return tiles;
}

QImage MemoryOptimizedProcessor::combineTilesIntoImage(const QList<ImageTile> &tiles, 
                                                      const QSize &originalSize, int overlap)
{
    qDebug() << "MemoryOptimizedProcessor::combineTilesIntoImage: 组合图像块";
    qDebug() << "块数量:" << tiles.size() << "目标尺寸:" << originalSize << "重叠:" << overlap;
    
    if (tiles.isEmpty() || originalSize.isEmpty()) {
        qWarning() << "无效的输入参数";
        return QImage();
    }
    
    // 从内存池获取或创建结果图像
    QImage result = getImageFromPool(originalSize, QImage::Format_ARGB32);
    if (result.isNull()) {
        result = QImage(originalSize, QImage::Format_ARGB32);
        updateMemoryStats(estimateImageMemoryUsage(originalSize, QImage::Format_ARGB32), 0);
    }
    result.fill(Qt::transparent);
    
    QPainter painter(&result);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // 组合所有块
    for (const ImageTile &tile : tiles) {
        if (tile.data.isNull() || !tile.processed) {
            continue;
        }
        
        QRect targetRect = tile.region;
        
        // 处理重叠区域的混合
        if (overlap > 0) {
            // 简化的重叠处理 - 直接覆盖
            painter.drawImage(targetRect, tile.data);
        } else {
            painter.drawImage(targetRect, tile.data);
        }
    }
    
    painter.end();
    
    qDebug() << "图像块组合完成";
    return result;
}

MemoryOptimizedProcessor::ImageTile MemoryOptimizedProcessor::processTile(
    const ImageTile &tile, std::function<QImage(const QImage&)> processor)
{
    if (tile.data.isNull() || !processor) {
        qWarning() << "无效的图像块或处理函数";
        return tile;
    }
    
    ImageTile processedTile = tile;
    
    try {
        // 应用处理函数
        QImage processedImage = processor(tile.data);
        
        if (!processedImage.isNull()) {
            processedTile.data = processedImage;
            processedTile.processed = true;
            
            // 更新内存统计
            qint64 newSize = estimateImageMemoryUsage(processedImage.size(), processedImage.format());
            qint64 oldSize = estimateImageMemoryUsage(tile.data.size(), tile.data.format());
            updateMemoryStats(newSize, oldSize);
        }
        
    } catch (const std::exception &e) {
        qCritical() << "处理图像块时发生异常:" << e.what();
        processedTile.processed = false;
    }
    
    return processedTile;
}

void MemoryOptimizedProcessor::clearCache()
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MemoryOptimizedProcessor::clearCache: 清理缓存";
    
    // 清理图像缓存
    qint64 freedMemory = 0;
    for (auto it = m_imageCache.begin(); it != m_imageCache.end(); ++it) {
        if (*it) {
            freedMemory += estimateImageMemoryUsage((*it)->size(), (*it)->format());
        }
    }
    m_imageCache.clear();
    
    // 清理块缓存
    m_tileCache.clear();
    
    // 清理内存池
    if (m_config.enableMemoryPool) {
        for (const QImage &poolImage : m_imagePool) {
            freedMemory += estimateImageMemoryUsage(poolImage.size(), poolImage.format());
        }
        m_imagePool.clear();
        m_poolBySize.clear();
    }
    
    updateMemoryStats(0, freedMemory);
    qDebug() << "缓存清理完成，释放内存:" << (freedMemory / 1024 / 1024) << "MB";
}

bool MemoryOptimizedProcessor::isMemoryPressureHigh() const
{
    qint64 currentUsage = getCurrentMemoryUsage();
    qint64 availableMemory = getAvailableSystemMemory();
    
    // 如果当前使用量超过阈值或可用内存不足
    bool highPressure = (currentUsage > m_memoryThresholdBytes) || 
                       (availableMemory < m_memoryThresholdBytes / 4);
    
    if (highPressure) {
        qDebug() << "检测到高内存压力 - 当前使用:" << (currentUsage / 1024 / 1024) 
                 << "MB 可用:" << (availableMemory / 1024 / 1024) << "MB";
    }
    
    return highPressure;
}

void MemoryOptimizedProcessor::optimizeMemoryUsage()
{
    QMutexLocker locker(&m_mutex);
    qDebug() << "MemoryOptimizedProcessor::optimizeMemoryUsage: 优化内存使用";
    
    qint64 initialUsage = getCurrentMemoryUsage();
    
    // 1. 清理过期缓存
    cleanupExpiredCache();
    
    // 2. 如果内存压力仍然很高，减少缓存大小
    if (isMemoryPressureHigh()) {
        int newCacheSize = m_config.maxCacheSize / 2;
        m_imageCache.setMaxCost(newCacheSize * 1024 * 1024);
        m_tileCache.setMaxCost(newCacheSize * 1024 * 1024 / 4);
        qDebug() << "降低缓存大小到:" << newCacheSize << "MB";
    }
    
    // 3. 清理内存池中较大的图像
    if (m_config.enableMemoryPool) {
        QSize largeThreshold(2048, 2048);
        auto it = m_poolBySize.begin();
        while (it != m_poolBySize.end()) {
            if (it.key().width() > largeThreshold.width() || 
                it.key().height() > largeThreshold.height()) {
                it = m_poolBySize.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // 4. 强制垃圾回收
    forceGarbageCollection();
    
    qint64 finalUsage = getCurrentMemoryUsage();
    qint64 freedMemory = initialUsage - finalUsage;
    
    qDebug() << "内存优化完成，释放内存:" << (freedMemory / 1024 / 1024) << "MB";
    
    if (freedMemory > 0) {
        updateMemoryStats(0, freedMemory);
    }
}

// 私有方法实现
QSize MemoryOptimizedProcessor::calculateOptimalTileSize(const QSize &imageSize, qint64 availableMemory) const
{
    // 目标是每个块使用不超过可用内存的1/8
    qint64 targetTileMemory = availableMemory / 8;
    
    // 估算单个像素的内存占用（ARGB32格式）
    int bytesPerPixel = 4;
    qint64 maxPixels = targetTileMemory / bytesPerPixel;
    
    // 计算正方形块的边长
    int sideLength = static_cast<int>(std::sqrt(maxPixels));
    
    // 限制在配置的最大块尺寸内
    sideLength = qMin(sideLength, qMax(m_config.maxTileSize.width(), m_config.maxTileSize.height()));
    
    // 确保最小尺寸
    sideLength = qMax(sideLength, 256);
    
    QSize optimalSize(sideLength, sideLength);
    
    // 如果图像本身较小，直接使用图像尺寸
    if (imageSize.width() <= optimalSize.width() && imageSize.height() <= optimalSize.height()) {
        optimalSize = imageSize;
    }
    
    qDebug() << "计算最优块尺寸:" << optimalSize << "对于图像:" << imageSize 
             << "可用内存:" << (availableMemory / 1024 / 1024) << "MB";
    
    return optimalSize;
}

qint64 MemoryOptimizedProcessor::estimateImageMemoryUsage(const QSize &size, QImage::Format format) const
{
    if (size.isEmpty()) {
        return 0;
    }
    
    int bytesPerPixel;
    switch (format) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        bytesPerPixel = 4;
        break;
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
    case QImage::Format_RGB444:
        bytesPerPixel = 2;
        break;
    case QImage::Format_Grayscale8:
    case QImage::Format_Indexed8:
        bytesPerPixel = 1;
        break;
    default:
        bytesPerPixel = 4; // 默认按最大估算
        break;
    }
    
    return static_cast<qint64>(size.width()) * size.height() * bytesPerPixel;
}

qint64 MemoryOptimizedProcessor::getAvailableSystemMemory() const
{
    qint64 availableMemory = 0;
    
#ifdef Q_OS_LINUX
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        availableMemory = si.freeram * si.mem_unit;
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        availableMemory = statex.ullAvailPhys;
    }
#elif defined(Q_OS_MACOS)
    vm_statistics_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(natural_t);
    if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) == KERN_SUCCESS) {
        availableMemory = vm_stat.free_count * vm_page_size;
    }
#endif
    
    // 如果无法获取系统信息，使用保守估算
    if (availableMemory == 0) {
        availableMemory = 1024 * 1024 * 1024; // 1GB
    }
    
    return availableMemory;
}

bool MemoryOptimizedProcessor::shouldUseTiledProcessing(const QImage &image) const
{
    qint64 imageMemory = estimateImageMemoryUsage(image.size(), image.format());
    qint64 threshold = m_memoryThresholdBytes / 4; // 使用阈值的1/4作为分块处理临界点
    
    bool shouldTile = imageMemory > threshold || isMemoryPressureHigh();
    
    qDebug() << "图像内存占用:" << (imageMemory / 1024 / 1024) << "MB"
             << "阈值:" << (threshold / 1024 / 1024) << "MB"
             << "是否分块:" << shouldTile;
    
    return shouldTile;
}

void MemoryOptimizedProcessor::performMemoryMonitoring()
{
    if (m_isProcessing) {
        return; // 处理过程中不进行监控，避免干扰
    }
    
    qint64 currentUsage = getCurrentMemoryUsage();
    qint64 availableMemory = getAvailableSystemMemory();
    
    // 更新峰值使用量
    if (currentUsage > m_memoryStats.peakUsage) {
        m_memoryStats.peakUsage = currentUsage;
    }
    
    // 检查内存压力
    if (currentUsage > m_memoryThresholdBytes) {
        emit memoryPressureWarning(currentUsage, m_memoryThresholdBytes);
        
        // 自动触发内存优化
        optimizeMemoryUsage();
    }
    
    m_lastMemoryCheck = QDateTime::currentMSecsSinceEpoch();
}

qint64 MemoryOptimizedProcessor::getCurrentMemoryUsage() const
{
    qint64 usage = 0;
    
    // 估算缓存使用的内存
    usage += m_imageCache.totalCost();
    usage += m_tileCache.totalCost();
    
    // 估算内存池使用的内存
    if (m_config.enableMemoryPool) {
        for (const QImage &poolImage : m_imagePool) {
            usage += estimateImageMemoryUsage(poolImage.size(), poolImage.format());
        }
    }
    
    return usage;
}

// 槽函数实现
void MemoryOptimizedProcessor::cleanupExpiredCache()
{
    QMutexLocker locker(&m_mutex);
    
    // QCache会自动管理过期项，这里主要是触发清理
    qint64 beforeCost = m_imageCache.totalCost() + m_tileCache.totalCost();
    
    // 强制清理一些缓存项以释放内存
    if (isMemoryPressureHigh()) {
        m_imageCache.setMaxCost(m_imageCache.maxCost() * 0.8); // 临时减少80%
        m_tileCache.setMaxCost(m_tileCache.maxCost() * 0.8);
        
        // 恢复原始大小
        QTimer::singleShot(60000, this, [this]() {
            m_imageCache.setMaxCost(m_config.maxCacheSize * 1024 * 1024);
            m_tileCache.setMaxCost(m_config.maxCacheSize * 1024 * 1024 / 4);
        });
    }
    
    qint64 afterCost = m_imageCache.totalCost() + m_tileCache.totalCost();
    qint64 freedMemory = beforeCost - afterCost;
    
    if (freedMemory > 0) {
        updateMemoryStats(0, freedMemory);
        qDebug() << "缓存清理释放内存:" << (freedMemory / 1024 / 1024) << "MB";
    }
} 