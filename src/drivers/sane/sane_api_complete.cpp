// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

// 预定义SANE版本码宏
#define SANE_VERSION_CODE(major, minor, patch) \
    (((major) << 24) | ((minor) << 16) | (patch))

#include "dscannersane_p.h"
#include "sane_api_complete.h"
#include "Scanner/DScannerDevice.h"
#include "Scanner/DScannerTypes.h"

#include <QLoggingCategory>
#include <QMutexLocker>
#include <QThread>
#include <QLibrary>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>
#include <QDateTime>

#include <cstring>
#include <algorithm>
#include <memory>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerSANEComplete, "deepinscan.sane.complete")

// SANE状态码映射
static const QMap<SANEStatus, int> g_saneStatusMap = {
    {SANEStatus::Good, 0},                    // SANE_STATUS_GOOD
    {SANEStatus::Unsupported, 1},             // SANE_STATUS_UNSUPPORTED  
    {SANEStatus::Cancelled, 2},               // SANE_STATUS_CANCELLED
    {SANEStatus::DeviceBusy, 3},              // SANE_STATUS_DEVICE_BUSY
    {SANEStatus::Invalid, 4},                 // SANE_STATUS_INVAL
    {SANEStatus::EOF_, 5},              // SANE_STATUS_EOF
    {SANEStatus::Jammed, 6},                  // SANE_STATUS_JAMMED
    {SANEStatus::NoDocs, 7},                  // SANE_STATUS_NO_DOCS
    {SANEStatus::CoverOpen, 8},               // SANE_STATUS_COVER_OPEN
    {SANEStatus::IOError, 9},                 // SANE_STATUS_IO_ERROR
    {SANEStatus::NoMem, 10},                  // SANE_STATUS_NO_MEM
    {SANEStatus::AccessDenied, 11}            // SANE_STATUS_ACCESS_DENIED
};

// SANE选项类型映射
static const QMap<SANEValueType, int> g_saneValueTypeMap = {
    {SANEValueType::Bool, 0},                 // SANE_TYPE_BOOL
    {SANEValueType::Int, 1},                  // SANE_TYPE_INT
    {SANEValueType::Fixed, 2},                // SANE_TYPE_FIXED
    {SANEValueType::String, 3},               // SANE_TYPE_STRING
    {SANEValueType::Button, 4},               // SANE_TYPE_BUTTON
    {SANEValueType::Group, 5}                 // SANE_TYPE_GROUP
};

// 全局SANE管理器实例
static SANEAPIManager *g_saneManager = nullptr;
static QMutex g_saneManagerMutex;

// SANEAPIManager实现
SANEAPIManager::SANEAPIManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_versionCode(0)
    , m_authCallback(nullptr)
    , m_deviceList(nullptr)
    , m_deviceCount(0)
    , m_openDevices()
    , m_deviceMutex()
    , m_optionCache()
    , m_optionCacheTimer(new QTimer(this))
{
    qCDebug(dscannerSANEComplete) << "SANEAPIManager created";
    
    // 设置选项缓存定时器
    m_optionCacheTimer->setInterval(30000); // 30秒更新缓存
    m_optionCacheTimer->setSingleShot(false);
    connect(m_optionCacheTimer, &QTimer::timeout, this, &SANEAPIManager::updateOptionCache);
    
    // 加载内置设备数据库
    loadBuiltinDeviceDatabase();
}

SANEAPIManager::~SANEAPIManager()
{
    if (m_initialized) {
        sane_exit_impl();
    }
    cleanupDeviceList();
    qCDebug(dscannerSANEComplete) << "SANEAPIManager destroyed";
}

SANEAPIManager* SANEAPIManager::instance()
{
    QMutexLocker locker(&g_saneManagerMutex);
    if (!g_saneManager) {
        g_saneManager = new SANEAPIManager();
    }
    return g_saneManager;
}

void SANEAPIManager::destroyInstance()
{
    QMutexLocker locker(&g_saneManagerMutex);
    if (g_saneManager) {
        delete g_saneManager;
        g_saneManager = nullptr;
    }
}

// SANE API核心实现
int SANEAPIManager::sane_init_impl(int *version_code, void (*auth_callback)(const char*, char*, char*))
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (m_initialized) {
        qCDebug(dscannerSANEComplete) << "SANE already initialized";
        if (version_code) {
            *version_code = m_versionCode;
        }
        return g_saneStatusMap[SANEStatus::Good];
    }
    
    qCInfo(dscannerSANEComplete) << "Initializing SANE subsystem";
    
    // 设置版本码 (模拟SANE 1.0.32)
    m_versionCode = SANE_VERSION_CODE(1, 0, 32);
    if (version_code) {
        *version_code = m_versionCode;
    }
    
    // 设置认证回调
    m_authCallback = auth_callback;
    
    // 初始化设备发现
    if (!initializeDeviceDiscovery()) {
        qCWarning(dscannerSANEComplete) << "Failed to initialize device discovery";
        return g_saneStatusMap[SANEStatus::IOError];
    }
    
    // 启动定时器
    m_optionCacheTimer->start();
    
    m_initialized = true;
    qCInfo(dscannerSANEComplete) << "SANE initialized successfully, version:" 
                                 << QString("0x%1").arg(m_versionCode, 0, 16);
    
    return g_saneStatusMap[SANEStatus::Good];
}

void SANEAPIManager::sane_exit_impl()
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!m_initialized) {
        return;
    }
    
    qCInfo(dscannerSANEComplete) << "Shutting down SANE subsystem";
    
    // 停止定时器
    m_optionCacheTimer->stop();
    
    // 关闭所有打开的设备
    closeAllDevices();
    
    // 清理设备列表
    cleanupDeviceList();
    
    // 清理资源
    m_authCallback = nullptr;
    m_versionCode = 0;
    m_initialized = false;
    
    qCInfo(dscannerSANEComplete) << "SANE shutdown completed";
}

int SANEAPIManager::sane_get_devices_impl(const void ***device_list, int local_only)
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!m_initialized) {
        qCWarning(dscannerSANEComplete) << "SANE not initialized";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    qCDebug(dscannerSANEComplete) << "Getting SANE devices, local_only:" << local_only;
    
    // 清理之前的设备列表
    cleanupDeviceList();
    
    // 发现设备
    QList<SANEDeviceInfo> discoveredDevices = discoverDevices(local_only != 0);
    
    if (discoveredDevices.isEmpty()) {
        qCDebug(dscannerSANEComplete) << "No devices discovered";
        *device_list = nullptr;
        return g_saneStatusMap[SANEStatus::Good];
    }
    
    // 创建SANE设备列表
    m_deviceCount = discoveredDevices.size();
    m_deviceList = new void*[m_deviceCount + 1];
    
    for (int i = 0; i < m_deviceCount; ++i) {
        SANEDeviceInfo *deviceInfo = new SANEDeviceInfo(discoveredDevices[i]);
        m_deviceList[i] = deviceInfo;
    }
    m_deviceList[m_deviceCount] = nullptr; // 结束标志
    
    *device_list = const_cast<const void**>(m_deviceList);
    
    qCInfo(dscannerSANEComplete) << "Discovered" << m_deviceCount << "SANE devices";
    return g_saneStatusMap[SANEStatus::Good];
}

int SANEAPIManager::sane_open_impl(const char *devicename, void **handle)
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!m_initialized) {
        qCWarning(dscannerSANEComplete) << "SANE not initialized";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    if (!devicename || !handle) {
        qCWarning(dscannerSANEComplete) << "Invalid parameters for sane_open";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    QString deviceName = QString::fromUtf8(devicename);
    qCDebug(dscannerSANEComplete) << "Opening SANE device:" << deviceName;
    
    // 检查设备是否已经打开
    if (m_openDevices.contains(deviceName)) {
        qCWarning(dscannerSANEComplete) << "Device already open:" << deviceName;
        return g_saneStatusMap[SANEStatus::DeviceBusy];
    }
    
    // 查找设备信息
    SANEDeviceInfo deviceInfo = findDeviceInfo(deviceName);
    if (deviceInfo.name.isEmpty()) {
        qCWarning(dscannerSANEComplete) << "Device not found:" << deviceName;
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    // 创建设备句柄
    SANEDeviceHandle *deviceHandle = new SANEDeviceHandle();
    deviceHandle->deviceInfo = deviceInfo;
    deviceHandle->isOpen = true;
    deviceHandle->scanInProgress = false;
    deviceHandle->lastError = SANEStatus::Good;
    
    // 初始化设备选项
    if (!initializeDeviceOptions(deviceHandle)) {
        delete deviceHandle;
        qCWarning(dscannerSANEComplete) << "Failed to initialize device options:" << deviceName;
        return g_saneStatusMap[SANEStatus::IOError];
    }
    
    // 尝试打开实际设备
    if (!openPhysicalDevice(deviceHandle)) {
        delete deviceHandle;
        qCWarning(dscannerSANEComplete) << "Failed to open physical device:" << deviceName;
        return g_saneStatusMap[SANEStatus::IOError];
    }
    
    // 添加到打开设备列表
    m_openDevices[deviceName] = deviceHandle;
    *handle = deviceHandle;
    
    qCInfo(dscannerSANEComplete) << "Device opened successfully:" << deviceName;
    return g_saneStatusMap[SANEStatus::Good];
}

void SANEAPIManager::sane_close_impl(void *handle)
{
    QMutexLocker locker(&m_deviceMutex);
    
    if (!handle) {
        qCWarning(dscannerSANEComplete) << "Invalid handle for sane_close";
        return;
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    QString deviceName = deviceHandle->deviceInfo.name;
    
    qCDebug(dscannerSANEComplete) << "Closing SANE device:" << deviceName;
    
    // 停止扫描（如果正在进行）
    if (deviceHandle->scanInProgress) {
        sane_cancel_impl(handle);
    }
    
    // 关闭物理设备
    closePhysicalDevice(deviceHandle);
    
    // 从打开设备列表中移除
    m_openDevices.remove(deviceName);
    
    // 清理设备句柄
    delete deviceHandle;
    
    qCInfo(dscannerSANEComplete) << "Device closed:" << deviceName;
}

const void* SANEAPIManager::sane_get_option_descriptor_impl(void *handle, int option)
{
    if (!handle) {
        qCWarning(dscannerSANEComplete) << "Invalid handle for sane_get_option_descriptor";
        return nullptr;
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    
    if (option < 0 || option >= deviceHandle->options.size()) {
        qCWarning(dscannerSANEComplete) << "Invalid option index:" << option;
        return nullptr;
    }
    
    qCDebug(dscannerSANEComplete) << "Getting option descriptor for option:" << option;
    
    return &deviceHandle->options[option].descriptor;
}

int SANEAPIManager::sane_control_option_impl(void *handle, int option, int action, void *value, int *info)
{
    if (!handle) {
        qCWarning(dscannerSANEComplete) << "Invalid handle for sane_control_option";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    
    if (option < 0 || option >= deviceHandle->options.size()) {
        qCWarning(dscannerSANEComplete) << "Invalid option index:" << option;
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    SANEAction saneAction = static_cast<SANEAction>(action);
    SANEOption &optionData = deviceHandle->options[option];
    
    qCDebug(dscannerSANEComplete) << "Controlling option:" << option << "action:" << action;
    
    switch (saneAction) {
    case SANEAction::GetValue:
        return getOptionValue(deviceHandle, option, value, info);
        
    case SANEAction::SetValue:
        return setOptionValue(deviceHandle, option, value, info);
        
    case SANEAction::SetAuto:
        return setOptionAuto(deviceHandle, option, info);
        
    default:
        qCWarning(dscannerSANEComplete) << "Unsupported action:" << action;
        return g_saneStatusMap[SANEStatus::Unsupported];
    }
}

int SANEAPIManager::sane_get_parameters_impl(void *handle, SANEParameters *params)
{
    if (!handle || !params) {
        qCWarning(dscannerSANEComplete) << "Invalid parameters for sane_get_parameters";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    
    qCDebug(dscannerSANEComplete) << "Getting scan parameters";
    
    // 构建扫描参数
    *params = buildScanParameters(deviceHandle);
    
    qCDebug(dscannerSANEComplete) << "Scan parameters:"
                                  << "format:" << static_cast<int>(params->format)
                                  << "lastFrame:" << params->lastFrame
                                  << "bytesPerLine:" << params->bytesPerLine
                                  << "pixelsPerLine:" << params->pixelsPerLine
                                  << "lines:" << params->lines
                                  << "depth:" << params->depth;
    
    return g_saneStatusMap[SANEStatus::Good];
}

int SANEAPIManager::sane_start_impl(void *handle)
{
    if (!handle) {
        qCWarning(dscannerSANEComplete) << "Invalid handle for sane_start";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    
    if (deviceHandle->scanInProgress) {
        qCWarning(dscannerSANEComplete) << "Scan already in progress";
        return g_saneStatusMap[SANEStatus::DeviceBusy];
    }
    
    qCInfo(dscannerSANEComplete) << "Starting scan";
    
    // 准备扫描参数
    if (!prepareScanParameters(deviceHandle)) {
        qCWarning(dscannerSANEComplete) << "Failed to prepare scan parameters";
        return g_saneStatusMap[SANEStatus::IOError];
    }
    
    // 开始扫描
    if (!startPhysicalScan(deviceHandle)) {
        qCWarning(dscannerSANEComplete) << "Failed to start physical scan";
        return g_saneStatusMap[SANEStatus::IOError];
    }
    
    deviceHandle->scanInProgress = true;
    deviceHandle->scanPosition = 0;
    
    qCInfo(dscannerSANEComplete) << "Scan started successfully";
    return g_saneStatusMap[SANEStatus::Good];
}

int SANEAPIManager::sane_read_impl(void *handle, unsigned char *data, int max_length, int *length)
{
    if (!handle || !data || !length) {
        qCWarning(dscannerSANEComplete) << "Invalid parameters for sane_read";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    
    if (!deviceHandle->scanInProgress) {
        qCWarning(dscannerSANEComplete) << "No scan in progress";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    // 读取扫描数据
    int bytesRead = readScanData(deviceHandle, data, max_length);
    
    if (bytesRead < 0) {
        qCWarning(dscannerSANEComplete) << "Error reading scan data";
        return g_saneStatusMap[SANEStatus::IOError];
    }
    
    if (bytesRead == 0) {
        // 扫描完成
        deviceHandle->scanInProgress = false;
        qCInfo(dscannerSANEComplete) << "Scan completed";
        return g_saneStatusMap[SANEStatus::EOF_];
    }
    
    *length = bytesRead;
    deviceHandle->scanPosition += bytesRead;
    
    qCDebug(dscannerSANEComplete) << "Read" << bytesRead << "bytes, total:" << deviceHandle->scanPosition;
    return g_saneStatusMap[SANEStatus::Good];
}

void SANEAPIManager::sane_cancel_impl(void *handle)
{
    if (!handle) {
        qCWarning(dscannerSANEComplete) << "Invalid handle for sane_cancel";
        return;
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    
    if (!deviceHandle->scanInProgress) {
        qCDebug(dscannerSANEComplete) << "No scan to cancel";
        return;
    }
    
    qCInfo(dscannerSANEComplete) << "Cancelling scan";
    
    // 取消物理扫描
    cancelPhysicalScan(deviceHandle);
    
    deviceHandle->scanInProgress = false;
    deviceHandle->scanPosition = 0;
    
    qCInfo(dscannerSANEComplete) << "Scan cancelled";
}

int SANEAPIManager::sane_set_io_mode_impl(void *handle, int non_blocking)
{
    if (!handle) {
        qCWarning(dscannerSANEComplete) << "Invalid handle for sane_set_io_mode";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    SANEDeviceHandle *deviceHandle = static_cast<SANEDeviceHandle*>(handle);
    deviceHandle->nonBlockingMode = (non_blocking != 0);
    
    qCDebug(dscannerSANEComplete) << "Set I/O mode, non_blocking:" << deviceHandle->nonBlockingMode;
    return g_saneStatusMap[SANEStatus::Good];
}

int SANEAPIManager::sane_get_select_fd_impl(void *handle, int *fd)
{
    if (!handle || !fd) {
        qCWarning(dscannerSANEComplete) << "Invalid parameters for sane_get_select_fd";
        return g_saneStatusMap[SANEStatus::Invalid];
    }
    
    // 对于我们的实现，不支持select()模式
    qCDebug(dscannerSANEComplete) << "Select FD not supported";
    return g_saneStatusMap[SANEStatus::Unsupported];
}

// 辅助函数实现
bool SANEAPIManager::initializeDeviceDiscovery()
{
    qCDebug(dscannerSANEComplete) << "Initializing device discovery";
    
    // 初始化USB发现
    if (!initializeUSBDiscovery()) {
        qCWarning(dscannerSANEComplete) << "Failed to initialize USB discovery";
        return false;
    }
    
    // 初始化网络发现
    if (!initializeNetworkDiscovery()) {
        qCWarning(dscannerSANEComplete) << "Failed to initialize network discovery";
        // 网络发现失败不是致命错误
    }
    
    return true;
}

void SANEAPIManager::cleanupDeviceList()
{
    if (m_deviceList) {
        for (int i = 0; i < m_deviceCount; ++i) {
            delete static_cast<SANEDeviceInfo*>(m_deviceList[i]);
        }
        delete[] m_deviceList;
        m_deviceList = nullptr;
        m_deviceCount = 0;
    }
}

void SANEAPIManager::closeAllDevices()
{
    for (auto it = m_openDevices.begin(); it != m_openDevices.end(); ++it) {
        SANEDeviceHandle *handle = it.value();
        if (handle->scanInProgress) {
            cancelPhysicalScan(handle);
        }
        closePhysicalDevice(handle);
        delete handle;
    }
    m_openDevices.clear();
}

void SANEAPIManager::updateOptionCache()
{
    // 更新选项缓存，用于性能优化
    qCDebug(dscannerSANEComplete) << "Updating option cache";
    
    for (auto it = m_openDevices.begin(); it != m_openDevices.end(); ++it) {
        SANEDeviceHandle *handle = it.value();
        refreshDeviceOptions(handle);
    }
}

void SANEAPIManager::loadBuiltinDeviceDatabase()
{
    qCDebug(dscannerSANEComplete) << "Loading builtin device database";
    
    // 加载内置设备数据库
    QString databasePath = QStandardPaths::locate(QStandardPaths::AppDataLocation, "device_database.json");
    if (databasePath.isEmpty()) {
        // 使用默认内置数据库
        loadDefaultDeviceDatabase();
    } else {
        // 加载外部数据库文件
        loadExternalDeviceDatabase(databasePath);
    }
    
    qCInfo(dscannerSANEComplete) << "Loaded" << m_deviceDatabase.size() << "device entries";
}

// #include "sane_api_complete.moc" 