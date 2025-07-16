#include "sane_option_manager.h"
#include "DScannerGlobal.h"
#include <QDebug>
#include <QMutexLocker>
#include <QThread>

DSCANNER_BEGIN_NAMESPACE

// 静态常量定义
const int SANEOptionManager::CACHE_TIMEOUT_MS = 5000;  // 5秒缓存超时
const int SANEOptionManager::MAX_CACHE_ENTRIES = 256;  // 最大缓存条目数

SANEOptionManager::SANEOptionManager(SANE_Handle handle, QObject *parent)
    : QObject(parent)
    , m_handle(handle)
{
    qDebug() << "SANEOptionManager: 初始化选项管理器";
    
    // 初始化缓存
    initializeCache();
    
    qDebug() << "SANEOptionManager: 选项管理器初始化完成，发现选项数量:" << m_cache.optionNames.size();
}

SANEOptionManager::~SANEOptionManager()
{
    qDebug() << "SANEOptionManager: 销毁选项管理器";
}

QVariant SANEOptionManager::getSANEParameter(const QString &name) const
{
    QMutexLocker locker(&m_cacheMutex);
    
    qDebug() << "SANEOptionManager::getSANEParameter: 获取参数" << name;
    
    // 检查缓存是否需要更新
    if (needsCacheUpdate()) {
        const_cast<SANEOptionManager*>(this)->updateOptionCache();
    }
    
    // 查找选项索引
    int optionIndex = m_cache.nameToIndex.value(name, -1);
    if (optionIndex == -1) {
        qWarning() << "SANEOptionManager::getSANEParameter: 未找到选项" << name;
        return QVariant();
    }
    
    // 检查缓存中是否有有效值
    if (m_cache.entries.contains(optionIndex) && m_cache.entries[optionIndex].isValid) {
        const OptionCacheEntry &entry = m_cache.entries[optionIndex];
        
        // 更新访问时间
        const_cast<OptionCacheEntry&>(entry).lastAccess = QDateTime::currentDateTime();
        
        qDebug() << "SANEOptionManager::getSANEParameter: 从缓存返回值" << entry.value;
        return entry.value;
    }
    
    // 从设备读取值
    QVariant value = readSANEOptionValue(optionIndex);
    
    // 更新缓存
    if (value.isValid()) {
        OptionCacheEntry &entry = const_cast<OptionCache&>(m_cache).entries[optionIndex];
        entry.value = value;
        entry.lastAccess = QDateTime::currentDateTime();
        entry.isValid = true;
    }
    
    qDebug() << "SANEOptionManager::getSANEParameter: 返回值" << value;
    return value;
}

QStringList SANEOptionManager::getSANEParameterNames() const
{
    QMutexLocker locker(&m_cacheMutex);
    
    qDebug() << "SANEOptionManager::getSANEParameterNames: 返回选项名称列表";
    
    // 检查缓存是否需要更新
    if (needsCacheUpdate()) {
        const_cast<SANEOptionManager*>(this)->updateOptionCache();
    }
    
    return m_cache.optionNames;
}

int SANEOptionManager::findSANEOption(const QString &name) const
{
    QMutexLocker locker(&m_cacheMutex);
    
    qDebug() << "SANEOptionManager::findSANEOption: 查找选项索引" << name;
    
    // 检查缓存是否需要更新
    if (needsCacheUpdate()) {
        const_cast<SANEOptionManager*>(this)->updateOptionCache();
    }
    
    int index = m_cache.nameToIndex.value(name, -1);
    qDebug() << "SANEOptionManager::findSANEOption: 选项" << name << "索引为" << index;
    
    return index;
}

QString SANEOptionManager::getSANEOptionName(int option) const
{
    QMutexLocker locker(&m_cacheMutex);
    
    qDebug() << "SANEOptionManager::getSANEOptionName: 获取选项名称，索引" << option;
    
    if (!m_handle) {
        qWarning() << "SANEOptionManager::getSANEOptionName: SANE句柄无效";
        return QString();
    }
    
    // 获取选项描述符
    const SANE_Option_Descriptor *desc = sane_get_option_descriptor(m_handle, option);
    if (!desc || !desc->name) {
        qWarning() << "SANEOptionManager::getSANEOptionName: 无效的选项索引" << option;
        return QString();
    }
    
    QString name = QString::fromUtf8(desc->name);
    qDebug() << "SANEOptionManager::getSANEOptionName: 选项名称" << name;
    
    return name;
}

void SANEOptionManager::updateOptionCache()
{
    qDebug() << "SANEOptionManager::updateOptionCache: 更新选项缓存";
    
    if (!m_handle) {
        qWarning() << "SANEOptionManager::updateOptionCache: SANE句柄无效";
        return;
    }
    
    QMutexLocker locker(&m_cacheMutex);
    
    // 清空现有缓存
    m_cache.nameToIndex.clear();
    m_cache.entries.clear();
    m_cache.optionNames.clear();
    
    // 获取选项数量
    SANE_Int optionCount = 0;
    const SANE_Option_Descriptor *desc = sane_get_option_descriptor(m_handle, 0);
    if (desc && desc->type == SANE_TYPE_INT) {
        // 选项0通常包含选项总数
        SANE_Status status = sane_control_option(m_handle, 0, SANE_ACTION_GET_VALUE, 
                                                  &optionCount, nullptr);
        if (status != SANE_STATUS_GOOD) {
            qWarning() << "SANEOptionManager::updateOptionCache: 无法获取选项数量";
            return;
        }
    }
    
    qDebug() << "SANEOptionManager::updateOptionCache: 发现选项数量" << optionCount;
    
    // 遍历所有选项
    for (int i = 1; i < optionCount; ++i) {  // 跳过选项0
        desc = sane_get_option_descriptor(m_handle, i);
        if (!desc || !desc->name) {
            continue;
        }
        
        QString optionName = QString::fromUtf8(desc->name);
        
        // 跳过无效或隐藏的选项
        if (!(desc->cap & SANE_CAP_SOFT_DETECT)) {
            continue;
        }
        
        // 添加到缓存
        m_cache.nameToIndex[optionName] = i;
        m_cache.optionNames.append(optionName);
        
        // 初始化缓存条目
        OptionCacheEntry &entry = m_cache.entries[i];
        entry.descriptor = *desc;
        entry.isValid = false;
        entry.lastAccess = QDateTime::currentDateTime();
        
        qDebug() << "SANEOptionManager::updateOptionCache: 添加选项" << optionName << "索引" << i;
    }
    
    // 更新缓存时间戳
    m_cache.lastUpdate = QDateTime::currentDateTime();
    m_cache.isValid = true;
    
    qDebug() << "SANEOptionManager::updateOptionCache: 缓存更新完成，总选项数" << m_cache.optionNames.size();
    
    // 发送信号
    emit optionCacheUpdated();
}

bool SANEOptionManager::hasOption(const QString &name) const
{
    QMutexLocker locker(&m_cacheMutex);
    
    // 检查缓存是否需要更新
    if (needsCacheUpdate()) {
        const_cast<SANEOptionManager*>(this)->updateOptionCache();
    }
    
    return m_cache.nameToIndex.contains(name);
}

const SANE_Option_Descriptor* SANEOptionManager::getOptionDescriptor(const QString &name) const
{
    QMutexLocker locker(&m_cacheMutex);
    
    qDebug() << "SANEOptionManager::getOptionDescriptor: 获取选项描述符" << name;
    
    // 检查缓存是否需要更新
    if (needsCacheUpdate()) {
        const_cast<SANEOptionManager*>(this)->updateOptionCache();
    }
    
    int optionIndex = m_cache.nameToIndex.value(name, -1);
    if (optionIndex == -1) {
        qWarning() << "SANEOptionManager::getOptionDescriptor: 未找到选项" << name;
        return nullptr;
    }
    
    if (m_cache.entries.contains(optionIndex)) {
        return &m_cache.entries[optionIndex].descriptor;
    }
    
    return nullptr;
}

void SANEOptionManager::onOptionValueChanged()
{
    qDebug() << "SANEOptionManager::onOptionValueChanged: 选项值已变化，清理缓存";
    
    QMutexLocker locker(&m_cacheMutex);
    
    // 标记所有缓存值为无效
    for (auto &entry : m_cache.entries) {
        entry.isValid = false;
    }
}

void SANEOptionManager::initializeCache()
{
    qDebug() << "SANEOptionManager::initializeCache: 初始化缓存";
    
    m_cache.isValid = false;
    m_cache.lastUpdate = QDateTime();
    
    // 执行初始缓存更新
    updateOptionCache();
}

QVariant SANEOptionManager::readSANEOptionValue(int option) const
{
    qDebug() << "SANEOptionManager::readSANEOptionValue: 读取选项值，索引" << option;
    
    if (!m_handle) {
        qWarning() << "SANEOptionManager::readSANEOptionValue: SANE句柄无效";
        return QVariant();
    }
    
    // 获取选项描述符
    const SANE_Option_Descriptor *desc = sane_get_option_descriptor(m_handle, option);
    if (!desc) {
        qWarning() << "SANEOptionManager::readSANEOptionValue: 无效的选项索引" << option;
        return QVariant();
    }
    
    // 检查选项是否可读
    if (!(desc->cap & SANE_CAP_SOFT_DETECT)) {
        qWarning() << "SANEOptionManager::readSANEOptionValue: 选项不可读，索引" << option;
        return QVariant();
    }
    
    // 分配缓冲区
    QByteArray buffer(desc->size, 0);
    
    // 读取值
    SANE_Status status = sane_control_option(m_handle, option, SANE_ACTION_GET_VALUE, 
                                             buffer.data(), nullptr);
    if (status != SANE_STATUS_GOOD) {
        qWarning() << "SANEOptionManager::readSANEOptionValue: 读取选项值失败，状态" << status;
        return QVariant();
    }
    
    // 转换值
    QVariant result = convertSANEValue(desc, buffer.constData());
    qDebug() << "SANEOptionManager::readSANEOptionValue: 读取到值" << result;
    
    return result;
}

QVariant SANEOptionManager::convertSANEValue(const SANE_Option_Descriptor *descriptor, 
                                             const void *data) const
{
    if (!descriptor || !data) {
        return QVariant();
    }
    
    switch (descriptor->type) {
    case SANE_TYPE_BOOL:
        return QVariant(static_cast<bool>(*static_cast<const SANE_Bool*>(data)));
        
    case SANE_TYPE_INT:
        return QVariant(*static_cast<const SANE_Int*>(data));
        
    case SANE_TYPE_FIXED:
        return QVariant(SANE_UNFIX(*static_cast<const SANE_Fixed*>(data)));
        
    case SANE_TYPE_STRING:
        return QVariant(QString::fromUtf8(static_cast<const char*>(data)));
        
    case SANE_TYPE_BUTTON:
        // 按钮类型没有值
        return QVariant();
        
    case SANE_TYPE_GROUP:
        // 组类型没有值
        return QVariant();
        
    default:
        qWarning() << "SANEOptionManager::convertSANEValue: 未知的选项类型" << descriptor->type;
        return QVariant();
    }
}

bool SANEOptionManager::needsCacheUpdate() const
{
    if (!m_cache.isValid) {
        return true;
    }
    
    // 检查超时
    if (m_cache.lastUpdate.msecsTo(QDateTime::currentDateTime()) > CACHE_TIMEOUT_MS) {
        return true;
    }
    
    return false;
}

void SANEOptionManager::cleanupExpiredEntries()
{
    qDebug() << "SANEOptionManager::cleanupExpiredEntries: 清理过期缓存条目";
    
    QDateTime now = QDateTime::currentDateTime();
    QList<int> expiredEntries;
    
    // 查找过期条目
    for (auto it = m_cache.entries.begin(); it != m_cache.entries.end(); ++it) {
        if (it->lastAccess.msecsTo(now) > CACHE_TIMEOUT_MS * 2) {
            expiredEntries.append(it.key());
        }
    }
    
    // 删除过期条目
    for (int key : expiredEntries) {
        m_cache.entries.remove(key);
    }
    
    qDebug() << "SANEOptionManager::cleanupExpiredEntries: 清理了" << expiredEntries.size() << "个过期条目";
}

DSCANNER_END_NAMESPACE

#include "sane_option_manager.moc" 