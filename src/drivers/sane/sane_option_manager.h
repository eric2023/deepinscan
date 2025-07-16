#ifndef SANE_OPTION_MANAGER_H
#define SANE_OPTION_MANAGER_H

#include <QObject>
#include <QHash>
#include <QVariant>
#include <QStringList>
#include <QDateTime>
#include <QMutex>
#include <sane/sane.h>

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief SANE选项管理器
 * 
 * 基于先进技术的选项管理架构，提供高效的选项缓存和访问机制
 * 主要特性：
 * - 智能选项缓存系统
 * - 快速名称到索引映射
 * - 类型安全的值转换
 * - 线程安全的访问控制
 */
class SANEOptionManager : public QObject
{
    Q_OBJECT

public:
    explicit SANEOptionManager(SANE_Handle handle, QObject *parent = nullptr);
    ~SANEOptionManager();

    /**
     * @brief 根据名称获取SANE参数值
     * @param name 参数名称
     * @return 参数值，失败返回无效QVariant
     */
    QVariant getSANEParameter(const QString &name) const;

    /**
     * @brief 获取所有可用的SANE参数名称列表
     * @return 参数名称列表
     */
    QStringList getSANEParameterNames() const;

    /**
     * @brief 根据名称查找SANE选项索引
     * @param name 选项名称
     * @return 选项索引，未找到返回-1
     */
    int findSANEOption(const QString &name) const;

    /**
     * @brief 根据索引获取SANE选项名称
     * @param option 选项索引
     * @return 选项名称，无效索引返回空字符串
     */
    QString getSANEOptionName(int option) const;

    /**
     * @brief 更新选项缓存
     * 重新扫描设备选项并更新内部缓存
     */
    void updateOptionCache();

    /**
     * @brief 检查选项是否存在
     * @param name 选项名称
     * @return 存在返回true
     */
    bool hasOption(const QString &name) const;

    /**
     * @brief 获取选项描述符
     * @param name 选项名称
     * @return 选项描述符指针，不存在返回nullptr
     */
    const SANE_Option_Descriptor* getOptionDescriptor(const QString &name) const;

signals:
    /**
     * @brief 选项缓存已更新信号
     */
    void optionCacheUpdated();

    /**
     * @brief 选项值变化信号
     * @param name 选项名称
     * @param value 新值
     */
    void optionValueChanged(const QString &name, const QVariant &value);

private slots:
    void onOptionValueChanged();

private:
    struct OptionCacheEntry {
        SANE_Option_Descriptor descriptor;
        QVariant value;
        QDateTime lastAccess;
        bool isValid;
        
        OptionCacheEntry() : isValid(false) {}
    };

    /**
     * @brief 选项缓存结构
     */
    struct OptionCache {
        QHash<QString, int> nameToIndex;              ///< 名称到索引映射
        QHash<int, OptionCacheEntry> entries;         ///< 选项条目缓存
        QStringList optionNames;                      ///< 所有选项名称列表
        QDateTime lastUpdate;                         ///< 最后更新时间
        bool isValid;                                 ///< 缓存是否有效
        
        OptionCache() : isValid(false) {}
    };

    /**
     * @brief 初始化选项缓存
     */
    void initializeCache();

    /**
     * @brief 从SANE设备读取选项值
     * @param option 选项索引
     * @return 选项值
     */
    QVariant readSANEOptionValue(int option) const;

    /**
     * @brief 将SANE值转换为QVariant
     * @param descriptor 选项描述符
     * @param data SANE数据指针
     * @return 转换后的QVariant
     */
    QVariant convertSANEValue(const SANE_Option_Descriptor *descriptor, 
                              const void *data) const;

    /**
     * @brief 检查缓存是否需要更新
     * @return 需要更新返回true
     */
    bool needsCacheUpdate() const;

    /**
     * @brief 清理过期的缓存条目
     */
    void cleanupExpiredEntries();

    SANE_Handle m_handle;                   ///< SANE设备句柄
    mutable OptionCache m_cache;            ///< 选项缓存
    mutable QMutex m_cacheMutex;            ///< 缓存访问互斥锁
    static const int CACHE_TIMEOUT_MS;      ///< 缓存超时时间(毫秒)
    static const int MAX_CACHE_ENTRIES;     ///< 最大缓存条目数
};

DSCANNER_END_NAMESPACE

#endif // SANE_OPTION_MANAGER_H 