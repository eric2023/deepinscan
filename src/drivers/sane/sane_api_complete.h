// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SANE_API_COMPLETE_H
#define SANE_API_COMPLETE_H

#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerSANE.h"

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QMap>
#include <QList>
#include <QVariant>
#include <QByteArray>

DSCANNER_BEGIN_NAMESPACE

// 前向声明
class SANEDeviceHandle;
struct SANEDeviceInfo;
struct SANEOption;
struct SANEParameters;

/**
 * @brief SANE设备信息结构
 */
struct SANEDeviceInfo {
    QString name;           // 设备名称
    QString vendor;         // 厂商名称
    QString model;          // 型号名称
    QString type;           // 设备类型
    QString devicePath;     // 设备路径
    quint16 vendorId;       // 厂商ID
    quint16 productId;      // 产品ID
    ChipsetType chipset;    // 芯片组类型
    DriverType driverType;  // 驱动类型
    
    SANEDeviceInfo() : vendorId(0), productId(0), chipset(ChipsetType::Unknown), driverType(DriverType::Generic) {}
};

// SANEOptionDescriptor 已在 DScannerSANE.h 中定义

/**
 * @brief SANE选项数据
 */
struct SANEOption {
    SANEOptionDescriptor descriptor;
    QVariant value;         // 当前值
    QVariant defaultValue;  // 默认值
    bool isActive;          // 是否激活
    bool isSettable;        // 是否可设置
    bool isAdvanced;        // 是否高级选项
    
    SANEOption() : isActive(true), isSettable(true), isAdvanced(false) {}
};

/**
 * @brief SANE设备句柄
 */
class SANEDeviceHandle {
public:
    SANEDeviceInfo deviceInfo;
    QList<SANEOption> options;
    bool isOpen;
    bool scanInProgress;
    bool nonBlockingMode;
    SANEStatus lastError;
    qint64 scanPosition;
    QByteArray scanData;
    void *physicalHandle;   // 物理设备句柄
    
    SANEDeviceHandle() 
        : isOpen(false)
        , scanInProgress(false)
        , nonBlockingMode(false)
        , lastError(SANEStatus::Good)
        , scanPosition(0)
        , physicalHandle(nullptr)
    {}
    
    ~SANEDeviceHandle() {
        scanData.clear();
    }
};

// SANEParameters 已在 DScannerSANE.h 中定义

/**
 * @brief SANE API完整管理器
 * 
 * 这个类实现了完整的SANE API功能，基于先进的技术研究成果
 * 内置所有主要厂商的设备支持和驱动
 */
class SANEAPIManager : public QObject
{
    Q_OBJECT
    
public:
    // 单例模式
    static SANEAPIManager* instance();
    static void destroyInstance();
    
    // SANE API核心函数
    int sane_init_impl(int *version_code, void (*auth_callback)(const char*, char*, char*));
    void sane_exit_impl();
    int sane_get_devices_impl(const void ***device_list, int local_only);
    int sane_open_impl(const char *devicename, void **handle);
    void sane_close_impl(void *handle);
    const void* sane_get_option_descriptor_impl(void *handle, int option);
    int sane_control_option_impl(void *handle, int option, int action, void *value, int *info);
    int sane_get_parameters_impl(void *handle, SANEParameters *params);
    int sane_start_impl(void *handle);
    int sane_read_impl(void *handle, unsigned char *data, int max_length, int *length);
    void sane_cancel_impl(void *handle);
    int sane_set_io_mode_impl(void *handle, int non_blocking);
    int sane_get_select_fd_impl(void *handle, int *fd);
    
    // 扩展功能
    bool isInitialized() const { return m_initialized; }
    int getVersionCode() const { return m_versionCode; }
    QList<SANEDeviceInfo> getAllDevices(bool localOnly = true);
    
signals:
    void deviceConnected(const SANEDeviceInfo &deviceInfo);
    void deviceDisconnected(const QString &deviceName);
    void scanProgress(void *handle, int percentage);
    void errorOccurred(const QString &error);
    
private slots:
    void updateOptionCache();
    
private:
    explicit SANEAPIManager(QObject *parent = nullptr);
    ~SANEAPIManager();
    
    // 初始化和清理
    bool initializeDeviceDiscovery();
    bool initializeUSBDiscovery();
    bool initializeNetworkDiscovery();
    void cleanupDeviceList();
    void closeAllDevices();
    
    // 设备发现
    QList<SANEDeviceInfo> discoverDevices(bool localOnly);
    QList<SANEDeviceInfo> discoverUSBDevices();
    QList<SANEDeviceInfo> discoverNetworkDevices();
    SANEDeviceInfo findDeviceInfo(const QString &deviceName);
    
    // 设备操作
    bool openPhysicalDevice(SANEDeviceHandle *handle);
    void closePhysicalDevice(SANEDeviceHandle *handle);
    bool initializeDeviceOptions(SANEDeviceHandle *handle);
    void refreshDeviceOptions(SANEDeviceHandle *handle);
    
    // 选项管理
    int getOptionValue(SANEDeviceHandle *handle, int option, void *value, int *info);
    int setOptionValue(SANEDeviceHandle *handle, int option, void *value, int *info);
    int setOptionAuto(SANEDeviceHandle *handle, int option, int *info);
    
    // 扫描操作
    bool prepareScanParameters(SANEDeviceHandle *handle);
    bool startPhysicalScan(SANEDeviceHandle *handle);
    void cancelPhysicalScan(SANEDeviceHandle *handle);
    int readScanData(SANEDeviceHandle *handle, unsigned char *data, int maxLength);
    SANEParameters buildScanParameters(SANEDeviceHandle *handle);
    
    // 设备数据库
    void loadBuiltinDeviceDatabase();
    void loadDefaultDeviceDatabase();
    void loadExternalDeviceDatabase(const QString &path);
    
    // 厂商特定处理
    bool identifyGenesysDevice(SANEDeviceInfo &deviceInfo);
    bool identifyCanonDevice(SANEDeviceInfo &deviceInfo);
    bool identifyEpsonDevice(SANEDeviceInfo &deviceInfo);
    bool identifyHPDevice(SANEDeviceInfo &deviceInfo);
    
    // 成员变量
    bool m_initialized;
    int m_versionCode;
    void (*m_authCallback)(const char*, char*, char*);
    
    // 设备管理
    void **m_deviceList;
    int m_deviceCount;
    QMap<QString, SANEDeviceHandle*> m_openDevices;
    QMap<QString, SANEDeviceInfo> m_deviceDatabase;
    
    // 线程安全
    mutable QMutex m_deviceMutex;
    
    // 性能优化
    QMap<QString, QVariant> m_optionCache;
    QTimer *m_optionCacheTimer;
    
    // 静态实例
    friend class QScopedPointer<SANEAPIManager>;
};

DSCANNER_END_NAMESPACE

#endif // SANE_API_COMPLETE_H 