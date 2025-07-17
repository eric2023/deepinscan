// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "canon_driver_complete.h"
#include "Scanner/DScannerException.h"
#include <QDebug>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <cstring>

DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(canonDriver, "deepinscan.drivers.canon")

// Canon驱动完整实现

CanonDriverComplete::CanonDriverComplete(QObject *parent)
    : DScannerDriver(parent)
    , m_deviceSeries(CanonSeries::Unknown)
    , m_protocol(CanonProtocol::USB_Direct)
    , m_isConnected(false)
    , m_usbContext(nullptr)
    , m_usbHandle(nullptr)
    , m_usbInterface(0)
    , m_bulkInEndpoint(0x81)
    , m_bulkOutEndpoint(0x02)
    , m_networkPort(BJNP_PORT)
    , m_networkSocket(nullptr)
    , m_isScanning(false)
    , m_scanProgress(0)
    , m_statusTimer(new QTimer(this))
    , m_dataTimer(new QTimer(this))
{
    qCDebug(canonDriver) << "初始化Canon完整驱动";
    
    // 初始化Canon支持
    initializeCanonSupport();
    
    // 加载设备数据库
    loadCanonDeviceDatabase();
    
    // 设置定时器
    m_statusTimer->setInterval(1000); // 1秒状态更新
    m_dataTimer->setInterval(100);     // 100ms数据检查
    
    connect(m_statusTimer, &QTimer::timeout, this, &CanonDriverComplete::onCanonStatusTimer);
    connect(m_dataTimer, &QTimer::timeout, this, &CanonDriverComplete::onCanonDataReady);
    
    qCDebug(canonDriver) << "Canon完整驱动初始化完成";
}

CanonDriverComplete::~CanonDriverComplete()
{
    qCDebug(canonDriver) << "销毁Canon完整驱动";
    cleanup();
}

bool CanonDriverComplete::initialize()
{
    qCDebug(canonDriver) << "初始化Canon驱动";
    
    // 初始化USB上下文
    int result = libusb_init(&m_usbContext);
    if (result != LIBUSB_SUCCESS) {
        qCWarning(canonDriver) << "无法初始化libusb:" << libusb_error_name(result);
        return false;
    }
    
    // 设置USB调试级别
    libusb_set_option(m_usbContext, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
    
    qCDebug(canonDriver) << "Canon驱动初始化成功";
    return true;
}

void CanonDriverComplete::cleanup()
{
    qCDebug(canonDriver) << "清理Canon驱动";
    
    if (m_isConnected) {
        disconnectDevice();
    }
    
    if (m_usbContext) {
        libusb_exit(m_usbContext);
        m_usbContext = nullptr;
    }
    
    qCDebug(canonDriver) << "Canon驱动清理完成";
}

bool CanonDriverComplete::isDeviceSupported(const DScannerDeviceInfo &deviceInfo)
{
    qCDebug(canonDriver) << "检查Canon设备支持:" << deviceInfo.name;
    
    // 检查厂商名称
    if (deviceInfo.manufacturer.toLower().contains("canon")) {
        return true;
    }
    
    // 检查设备名称
    QString deviceName = deviceInfo.name.toLower();
    if (deviceName.contains("canoscan") || 
        deviceName.contains("imageformula") ||
        deviceName.contains("pixma") ||
        deviceName.contains("scanfront")) {
        return true;
    }
    
    // 检查USB供应商ID (Canon: 0x04a9)
    if (deviceInfo.properties.contains("vendorId")) {
        quint16 vendorId = deviceInfo.properties.value("vendorId").toUInt();
        if (vendorId == 0x04a9) {
            return true;
        }
    }
    
    return false;
}

bool CanonDriverComplete::connectDevice(const DScannerDeviceInfo &deviceInfo)
{
    qCDebug(canonDriver) << "连接Canon设备:" << deviceInfo.name;
    
    if (m_isConnected) {
        qCWarning(canonDriver) << "设备已连接";
        return false;
    }
    
    m_currentDevice = deviceInfo;
    
    // 识别Canon设备
    if (!identifyCanonDevice(deviceInfo)) {
        qCWarning(canonDriver) << "无法识别Canon设备";
        return false;
    }
    
    // 根据连接类型建立连接
    bool connected = false;
    if (deviceInfo.connectionType == "USB") {
        m_protocol = CanonProtocol::USB_Direct;
        connected = establishCanonUSBConnection();
    } else if (deviceInfo.connectionType == "Network") {
        // 根据设备能力选择网络协议
        if (deviceInfo.properties.contains("bjnp")) {
            m_protocol = CanonProtocol::BJNP;
        } else if (deviceInfo.properties.contains("eSCL")) {
            m_protocol = CanonProtocol::IPP_eSCL;
        } else {
            m_protocol = CanonProtocol::WSD;
        }
        connected = establishCanonNetworkConnection();
    }
    
    if (!connected) {
        qCWarning(canonDriver) << "无法建立Canon设备连接";
        return false;
    }
    
    // 执行设备初始化
    if (!performCanonDeviceInitialization()) {
        qCWarning(canonDriver) << "Canon设备初始化失败";
        disconnectDevice();
        return false;
    }
    
    // 读取设备能力
    if (!readCanonDeviceCapabilities()) {
        qCWarning(canonDriver) << "无法读取Canon设备能力";
        // 不致命错误，继续连接
    }
    
    m_isConnected = true;
    
    // 启动状态监控
    m_statusTimer->start();
    
    qCDebug(canonDriver) << "Canon设备连接成功";
    return true;
}

void CanonDriverComplete::disconnectDevice()
{
    qCDebug(canonDriver) << "断开Canon设备连接";
    
    if (!m_isConnected) {
        return;
    }
    
    // 停止扫描
    if (m_isScanning) {
        cancelScan();
    }
    
    // 停止定时器
    m_statusTimer->stop();
    m_dataTimer->stop();
    
    // 断开USB连接
    if (m_usbHandle) {
        libusb_release_interface(m_usbHandle, m_usbInterface);
        libusb_close(m_usbHandle);
        m_usbHandle = nullptr;
    }
    
    // 断开网络连接
    if (m_networkSocket) {
        m_networkSocket->disconnectFromHost();
        m_networkSocket->deleteLater();
        m_networkSocket = nullptr;
    }
    
    m_isConnected = false;
    
    qCDebug(canonDriver) << "Canon设备连接已断开";
}

bool CanonDriverComplete::isConnected() const
{
    return m_isConnected;
}

DScannerDeviceInfo CanonDriverComplete::getCurrentDeviceInfo() const
{
    return m_currentDevice;
}

QVariantMap CanonDriverComplete::getDeviceCapabilities() const
{
    QVariantMap caps;
    
    const CanonCapabilities &canonCaps = m_capabilities;
    
    caps["resolutions"] = QVariantList();
    for (int res : canonCaps.supportedResolutions) {
        caps["resolutions"] = caps["resolutions"].toList() << res;
    }
    
    caps["scanModes"] = QVariantList();
    for (ScanMode mode : canonCaps.supportedModes) {
        caps["scanModes"] = caps["scanModes"].toList() << static_cast<int>(mode);
    }
    
    caps["maxScanSize"] = canonCaps.maxScanSize;
    caps["hasADF"] = canonCaps.hasADF;
    caps["hasDuplex"] = canonCaps.hasDuplex;
    caps["hasTransparency"] = canonCaps.hasTransparency;
    caps["maxBitDepth"] = canonCaps.maxBitDepth;
    caps["supportedFormats"] = canonCaps.supportedFormats;
    caps["advancedFeatures"] = canonCaps.advancedFeatures;
    
    caps["canonSeries"] = static_cast<int>(m_deviceSeries);
    caps["canonProtocol"] = static_cast<int>(m_protocol);
    
    return caps;
}

QStringList CanonDriverComplete::getSupportedOptions() const
{
    QStringList options;
    
    options << "resolution" << "mode" << "source" << "depth";
    options << "top" << "left" << "width" << "height";
    options << "brightness" << "contrast" << "gamma";
    
    // Canon特有选项
    options << "canon_quality" << "canon_speed" << "canon_correction";
    options << "canon_descreening" << "canon_threshold";
    
    if (m_capabilities.hasADF) {
        options << "adf_mode" << "duplex";
    }
    
    if (m_capabilities.hasTransparency) {
        options << "transparency_mode";
    }
    
    return options;
}

QVariant CanonDriverComplete::getOptionValue(const QString &option) const
{
    return m_currentOptions.value(option);
}

bool CanonDriverComplete::setOptionValue(const QString &option, const QVariant &value)
{
    qCDebug(canonDriver) << "设置Canon选项:" << option << "=" << value;
    
    m_currentOptions[option] = value;
    
    // 立即更新设备选项
    QVariantMap options;
    options[option] = value;
    
    return setCanonAdvancedOptions(options);
}

bool CanonDriverComplete::startScan(const DScannerScanParameters &params)
{
    qCDebug(canonDriver) << "开始Canon扫描";
    
    if (!m_isConnected) {
        qCWarning(canonDriver) << "设备未连接";
        return false;
    }
    
    if (m_isScanning) {
        qCWarning(canonDriver) << "扫描已在进行中";
        return false;
    }
    
    QMutexLocker locker(&m_scanMutex);
    
    // 配置扫描参数
    if (!configureCanonScanParameters(params)) {
        qCWarning(canonDriver) << "配置Canon扫描参数失败";
        return false;
    }
    
    // 清空扫描缓冲区
    m_scanBuffer.clear();
    m_scanProgress = 0;
    
    // 启动扫描过程
    if (!startCanonScanProcess()) {
        qCWarning(canonDriver) << "启动Canon扫描过程失败";
        return false;
    }
    
    m_isScanning = true;
    
    // 启动数据定时器
    m_dataTimer->start();
    
    emit scanStarted();
    
    qCDebug(canonDriver) << "Canon扫描启动成功";
    return true;
}

void CanonDriverComplete::cancelScan()
{
    qCDebug(canonDriver) << "取消Canon扫描";
    
    if (!m_isScanning) {
        return;
    }
    
    QMutexLocker locker(&m_scanMutex);
    
    // 发送停止扫描命令
    processCanonProtocol(CANON_CMD_STOP_SCAN);
    
    m_isScanning = false;
    m_scanProgress = 0;
    
    // 停止数据定时器
    m_dataTimer->stop();
    
    emit scanCancelled();
    
    qCDebug(canonDriver) << "Canon扫描已取消";
}

QByteArray CanonDriverComplete::readScanData()
{
    QMutexLocker locker(&m_scanMutex);
    
    if (!m_isScanning) {
        return QByteArray();
    }
    
    // 从缓冲区读取数据
    QByteArray data = m_scanBuffer;
    m_scanBuffer.clear();
    
    qCDebug(canonDriver) << "读取Canon扫描数据:" << data.size() << "字节";
    
    return data;
}

bool CanonDriverComplete::isScanComplete() const
{
    return !m_isScanning && m_scanProgress >= 100;
}

int CanonDriverComplete::getScanProgress() const
{
    return m_scanProgress;
}

CanonDriverComplete::CanonSeries CanonDriverComplete::detectCanonSeries(const DScannerDeviceInfo &deviceInfo) const
{
    QString deviceName = deviceInfo.name.toLower();
    
    if (deviceName.contains("canoscan")) {
        return CanonSeries::CanoScan;
    } else if (deviceName.contains("imageformula")) {
        return CanonSeries::ImageFormula;
    } else if (deviceName.contains("pixma mf")) {
        return CanonSeries::PixmaMF;
    } else if (deviceName.contains("pixma ts")) {
        return CanonSeries::PixmaTS;
    } else if (deviceName.contains("scanfront")) {
        return CanonSeries::ScanFrontDR;
    }
    
    // 根据产品ID检测
    if (deviceInfo.properties.contains("productId")) {
        quint16 productId = deviceInfo.properties.value("productId").toUInt();
        return m_seriesDatabase.value(productId, CanonSeries::Unknown);
    }
    
    return CanonSeries::Unknown;
}

CanonDriverComplete::CanonCapabilities CanonDriverComplete::getCanonCapabilities() const
{
    return m_capabilities;
}

bool CanonDriverComplete::performCanonCalibration()
{
    qCDebug(canonDriver) << "执行Canon设备校准";
    
    if (!m_isConnected) {
        qCWarning(canonDriver) << "设备未连接";
        return false;
    }
    
    // 发送校准命令
    QByteArray response = processCanonProtocol(CANON_CMD_CALIBRATE);
    
    bool success = !response.isEmpty() && response[0] == 0x00; // 成功标志
    
    emit canonCalibrationCompleted(success);
    
    qCDebug(canonDriver) << "Canon设备校准" << (success ? "成功" : "失败");
    return success;
}

QVariantMap CanonDriverComplete::getCanonDeviceStatus() const
{
    return m_deviceStatus;
}

bool CanonDriverComplete::setCanonAdvancedOptions(const QVariantMap &options)
{
    qCDebug(canonDriver) << "设置Canon高级选项:" << options;
    
    if (!m_isConnected) {
        qCWarning(canonDriver) << "设备未连接";
        return false;
    }
    
    // 构建选项设置命令
    QByteArray optionData;
    QDataStream stream(&optionData, QIODevice::WriteOnly);
    
    for (auto it = options.begin(); it != options.end(); ++it) {
        QString key = it.key();
        QVariant value = it.value();
        
        // 编码选项键值对
        QByteArray keyBytes = key.toUtf8();
        stream << quint16(keyBytes.size());
        stream.writeRawData(keyBytes.constData(), keyBytes.size());
        
        QByteArray valueBytes = value.toByteArray();
        stream << quint16(valueBytes.size());
        stream.writeRawData(valueBytes.constData(), valueBytes.size());
    }
    
    // 发送设置参数命令
    QByteArray response = processCanonProtocol(CANON_CMD_SET_PARAMS, optionData);
    
    bool success = !response.isEmpty() && response[0] == 0x00;
    
    qCDebug(canonDriver) << "Canon高级选项设置" << (success ? "成功" : "失败");
    return success;
}

void CanonDriverComplete::initializeCanonSupport()
{
    qCDebug(canonDriver) << "初始化Canon设备支持";
    
    // 初始化系列数据库
    m_seriesDatabase[0x2206] = CanonSeries::CanoScan;     // CanoScan LiDE 25
    m_seriesDatabase[0x2207] = CanonSeries::CanoScan;     // CanoScan LiDE 60
    m_seriesDatabase[0x220d] = CanonSeries::CanoScan;     // CanoScan LiDE 100
    m_seriesDatabase[0x220e] = CanonSeries::CanoScan;     // CanoScan LiDE 200
    m_seriesDatabase[0x2220] = CanonSeries::CanoScan;     // CanoScan LiDE 220
    
    m_seriesDatabase[0x1909] = CanonSeries::ImageFormula; // imageFORMULA DR-2080C
    m_seriesDatabase[0x190a] = CanonSeries::ImageFormula; // imageFORMULA DR-2050C
    m_seriesDatabase[0x190f] = CanonSeries::ImageFormula; // imageFORMULA DR-6030C
    
    m_seriesDatabase[0x178a] = CanonSeries::PixmaMF;      // PIXMA MF3010
    m_seriesDatabase[0x178b] = CanonSeries::PixmaMF;      // PIXMA MF4410
    m_seriesDatabase[0x178c] = CanonSeries::PixmaMF;      // PIXMA MF4430
    
    m_seriesDatabase[0x179a] = CanonSeries::PixmaTS;      // PIXMA TS3100
    m_seriesDatabase[0x179b] = CanonSeries::PixmaTS;      // PIXMA TS5000
    m_seriesDatabase[0x179c] = CanonSeries::PixmaTS;      // PIXMA TS6000
    
    qCDebug(canonDriver) << "Canon设备支持初始化完成，支持" 
                         << m_seriesDatabase.size() << "个设备型号";
}

void CanonDriverComplete::loadCanonDeviceDatabase()
{
    qCDebug(canonDriver) << "加载Canon设备数据库";
    
    // 为不同系列创建默认能力
    CanonCapabilities canoScanCaps;
    canoScanCaps.supportedResolutions = {75, 150, 300, 600, 1200, 2400, 4800};
    canoScanCaps.supportedModes = {ScanMode::Color, ScanMode::Gray, ScanMode::Lineart};
    canoScanCaps.maxScanSize = QSize(8500, 11700); // A4
    canoScanCaps.hasADF = false;
    canoScanCaps.hasDuplex = false;
    canoScanCaps.hasTransparency = false;
    canoScanCaps.maxBitDepth = 16;
    canoScanCaps.supportedFormats = {"JPEG", "PNG", "TIFF", "PDF"};
    
    CanonCapabilities imageFormulaCaps;
    imageFormulaCaps.supportedResolutions = {150, 200, 300, 400, 600};
    imageFormulaCaps.supportedModes = {ScanMode::Color, ScanMode::Gray, ScanMode::Lineart};
    imageFormulaCaps.maxScanSize = QSize(8500, 14000); // Legal
    imageFormulaCaps.hasADF = true;
    imageFormulaCaps.hasDuplex = true;
    imageFormulaCaps.hasTransparency = false;
    imageFormulaCaps.maxBitDepth = 24;
    imageFormulaCaps.supportedFormats = {"JPEG", "PNG", "TIFF", "PDF"};
    
    CanonCapabilities pixmaCaps;
    pixmaCaps.supportedResolutions = {75, 150, 300, 600, 1200};
    pixmaCaps.supportedModes = {ScanMode::Color, ScanMode::Gray};
    pixmaCaps.maxScanSize = QSize(8500, 11700); // A4
    pixmaCaps.hasADF = true;
    pixmaCaps.hasDuplex = false;
    pixmaCaps.hasTransparency = false;
    pixmaCaps.maxBitDepth = 24;
    pixmaCaps.supportedFormats = {"JPEG", "PNG", "PDF"};
    
    // 存储到数据库
    m_deviceDatabase["CanoScan"] = canoScanCaps;
    m_deviceDatabase["ImageFormula"] = imageFormulaCaps;
    m_deviceDatabase["PixmaMF"] = pixmaCaps;
    m_deviceDatabase["PixmaTS"] = pixmaCaps;
    m_deviceDatabase["ScanFrontDR"] = imageFormulaCaps;
    
    qCDebug(canonDriver) << "Canon设备数据库加载完成";
}

bool CanonDriverComplete::identifyCanonDevice(const DScannerDeviceInfo &deviceInfo)
{
    qCDebug(canonDriver) << "识别Canon设备:" << deviceInfo.name;
    
    m_deviceSeries = detectCanonSeries(deviceInfo);
    
    if (m_deviceSeries == CanonSeries::Unknown) {
        qCWarning(canonDriver) << "无法识别Canon设备系列";
        return false;
    }
    
    // 根据系列加载设备能力
    QString seriesName;
    switch (m_deviceSeries) {
        case CanonSeries::CanoScan:
            seriesName = "CanoScan";
            break;
        case CanonSeries::ImageFormula:
            seriesName = "ImageFormula";
            break;
        case CanonSeries::PixmaMF:
            seriesName = "PixmaMF";
            break;
        case CanonSeries::PixmaTS:
            seriesName = "PixmaTS";
            break;
        case CanonSeries::ScanFrontDR:
            seriesName = "ScanFrontDR";
            break;
        default:
            seriesName = "Unknown";
            break;
    }
    
    if (m_deviceDatabase.contains(seriesName)) {
        m_capabilities = m_deviceDatabase[seriesName];
    }
    
    qCDebug(canonDriver) << "Canon设备识别完成，系列:" << seriesName;
    return true;
}

bool CanonDriverComplete::establishCanonUSBConnection()
{
    qCDebug(canonDriver) << "建立Canon USB连接";
    
    if (!m_usbContext) {
        qCWarning(canonDriver) << "USB上下文未初始化";
        return false;
    }
    
    // 获取USB设备列表
    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(m_usbContext, &deviceList);
    
    if (deviceCount < 0) {
        qCWarning(canonDriver) << "无法获取USB设备列表";
        return false;
    }
    
    // 查找Canon设备
    bool deviceFound = false;
    for (ssize_t i = 0; i < deviceCount; ++i) {
        libusb_device *device = deviceList[i];
        libusb_device_descriptor desc;
        
        if (libusb_get_device_descriptor(device, &desc) == LIBUSB_SUCCESS) {
            if (desc.idVendor == 0x04a9) { // Canon供应商ID
                qCDebug(canonDriver) << "找到Canon USB设备:" 
                                     << QString::number(desc.idProduct, 16);
                
                // 尝试打开设备
                int result = libusb_open(device, &m_usbHandle);
                if (result == LIBUSB_SUCCESS) {
                    deviceFound = true;
                    break;
                }
            }
        }
    }
    
    libusb_free_device_list(deviceList, 1);
    
    if (!deviceFound) {
        qCWarning(canonDriver) << "未找到Canon USB设备";
        return false;
    }
    
    // 声明接口
    int result = libusb_claim_interface(m_usbHandle, m_usbInterface);
    if (result != LIBUSB_SUCCESS) {
        qCWarning(canonDriver) << "无法声明USB接口:" << libusb_error_name(result);
        libusb_close(m_usbHandle);
        m_usbHandle = nullptr;
        return false;
    }
    
    qCDebug(canonDriver) << "Canon USB连接建立成功";
    return true;
}

bool CanonDriverComplete::establishCanonNetworkConnection()
{
    qCDebug(canonDriver) << "建立Canon网络连接";
    
    if (m_currentDevice.properties.contains("addresses")) {
        QStringList addresses = m_currentDevice.properties.value("addresses").toStringList();
        if (!addresses.isEmpty()) {
            m_networkHost = addresses.first();
        }
    }
    
    if (m_networkHost.isEmpty()) {
        qCWarning(canonDriver) << "无有效的网络地址";
        return false;
    }
    
    m_networkSocket = new QTcpSocket(this);
    
    // 连接到Canon设备
    m_networkSocket->connectToHost(m_networkHost, m_networkPort);
    
    if (!m_networkSocket->waitForConnected(5000)) {
        qCWarning(canonDriver) << "无法连接到Canon网络设备:" 
                               << m_networkSocket->errorString();
        return false;
    }
    
    qCDebug(canonDriver) << "Canon网络连接建立成功，地址:" 
                         << m_networkHost << ":" << m_networkPort;
    return true;
}

bool CanonDriverComplete::performCanonDeviceInitialization()
{
    qCDebug(canonDriver) << "执行Canon设备初始化";
    
    // 发送初始化命令
    QByteArray response = processCanonProtocol(CANON_CMD_INITIALIZE);
    
    if (response.isEmpty()) {
        qCWarning(canonDriver) << "Canon设备初始化失败 - 无响应";
        return false;
    }
    
    if (response[0] != 0x00) {
        qCWarning(canonDriver) << "Canon设备初始化失败 - 错误代码:" 
                               << QString::number(response[0], 16);
        return false;
    }
    
    qCDebug(canonDriver) << "Canon设备初始化成功";
    return true;
}

bool CanonDriverComplete::readCanonDeviceCapabilities()
{
    qCDebug(canonDriver) << "读取Canon设备能力";
    
    // 发送获取能力命令
    QByteArray response = processCanonProtocol(CANON_CMD_GET_CAPS);
    
    if (response.isEmpty()) {
        qCWarning(canonDriver) << "无法读取Canon设备能力";
        return false;
    }
    
    // 解析能力数据
    QVariantMap capsData = parseCanonResponse(response);
    
    if (capsData.contains("resolutions")) {
        m_capabilities.supportedResolutions.clear();
        QVariantList resList = capsData.value("resolutions").toList();
        for (const QVariant &res : resList) {
            m_capabilities.supportedResolutions.append(res.toInt());
        }
    }
    
    if (capsData.contains("maxSize")) {
        m_capabilities.maxScanSize = capsData.value("maxSize").toSize();
    }
    
    if (capsData.contains("hasADF")) {
        m_capabilities.hasADF = capsData.value("hasADF").toBool();
    }
    
    qCDebug(canonDriver) << "Canon设备能力读取完成";
    return true;
}

bool CanonDriverComplete::configureCanonScanParameters(const DScannerScanParameters &params)
{
    qCDebug(canonDriver) << "配置Canon扫描参数";
    
    // 构建参数数据
    QByteArray paramData;
    QDataStream stream(&paramData, QIODevice::WriteOnly);
    
    stream << quint16(params.resolution);
    stream << quint8(static_cast<int>(params.mode));
    stream << quint16(params.scanArea.x());
    stream << quint16(params.scanArea.y());
    stream << quint16(params.scanArea.width());
    stream << quint16(params.scanArea.height());
    
    // 发送设置参数命令
    QByteArray response = processCanonProtocol(CANON_CMD_SET_PARAMS, paramData);
    
    bool success = !response.isEmpty() && response[0] == 0x00;
    
    qCDebug(canonDriver) << "Canon扫描参数配置" << (success ? "成功" : "失败");
    return success;
}

bool CanonDriverComplete::startCanonScanProcess()
{
    qCDebug(canonDriver) << "启动Canon扫描过程";
    
    // 发送开始扫描命令
    QByteArray response = processCanonProtocol(CANON_CMD_START_SCAN);
    
    bool success = !response.isEmpty() && response[0] == 0x00;
    
    qCDebug(canonDriver) << "Canon扫描过程启动" << (success ? "成功" : "失败");
    return success;
}

QByteArray CanonDriverComplete::processCanonProtocol(quint8 command, const QByteArray &data)
{
    qCDebug(canonDriver) << "处理Canon协议通信，命令:" << QString::number(command, 16);
    
    QByteArray response;
    
    if (m_protocol == CanonProtocol::USB_Direct && m_usbHandle) {
        // USB协议通信
        QByteArray packet;
        packet.append(command);
        packet.append(static_cast<char>(data.size() & 0xFF));
        packet.append(static_cast<char>((data.size() >> 8) & 0xFF));
        packet.append(data);
        
        int transferred;
        int result = libusb_bulk_transfer(m_usbHandle, m_bulkOutEndpoint,
                                         reinterpret_cast<unsigned char*>(packet.data()),
                                         packet.size(), &transferred, 5000);
        
        if (result == LIBUSB_SUCCESS) {
            // 读取响应
            unsigned char buffer[1024];
            result = libusb_bulk_transfer(m_usbHandle, m_bulkInEndpoint,
                                         buffer, sizeof(buffer), &transferred, 5000);
            
            if (result == LIBUSB_SUCCESS) {
                response = QByteArray(reinterpret_cast<char*>(buffer), transferred);
            }
        }
        
    } else if (m_networkSocket && m_networkSocket->isValid()) {
        // 网络协议通信
        if (m_protocol == CanonProtocol::BJNP) {
            response = canonBJNPCommunication(QByteArray().append(command).append(data));
        } else {
            // 其他网络协议的处理
            m_networkSocket->write(QByteArray().append(command).append(data));
            m_networkSocket->waitForBytesWritten(3000);
            
            if (m_networkSocket->waitForReadyRead(5000)) {
                response = m_networkSocket->readAll();
            }
        }
    }
    
    qCDebug(canonDriver) << "Canon协议通信完成，响应大小:" << response.size();
    return response;
}

QByteArray CanonDriverComplete::canonBJNPCommunication(const QByteArray &request)
{
    qCDebug(canonDriver) << "Canon BJNP协议通信";
    
    // 构建BJNP包
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    
    stream << quint8(BJNP_VERSION);     // BJNP版本
    stream << quint8(BJNP_TYPE_SCAN);   // 扫描类型
    stream << quint16(request.size());   // 数据长度
    stream.writeRawData(request.constData(), request.size());
    
    // 发送BJNP包
    if (m_networkSocket) {
        m_networkSocket->write(packet);
        m_networkSocket->waitForBytesWritten(3000);
        
        if (m_networkSocket->waitForReadyRead(5000)) {
            QByteArray response = m_networkSocket->readAll();
            
            // 解析BJNP响应
            if (response.size() >= 4) {
                quint16 dataLen = (response[2] << 8) | response[3];
                if (response.size() >= 4 + dataLen) {
                    return response.mid(4, dataLen);
                }
            }
        }
    }
    
    return QByteArray();
}

QVariantMap CanonDriverComplete::parseCanonResponse(const QByteArray &response)
{
    QVariantMap result;
    
    if (response.isEmpty()) {
        return result;
    }
    
    // 简化的响应解析
    QDataStream stream(response);
    
    quint8 status;
    stream >> status;
    result["status"] = status;
    
    if (status == 0x00 && response.size() > 1) {
        // 成功响应，解析数据
        while (!stream.atEnd()) {
            quint8 tag;
            quint16 length;
            stream >> tag >> length;
            
            QByteArray data(length, 0);
            stream.readRawData(data.data(), length);
            
            switch (tag) {
                case 0x01: // 分辨率列表
                    result["resolutions"] = data;
                    break;
                case 0x02: // 最大尺寸
                    result["maxSize"] = data;
                    break;
                case 0x03: // ADF支持
                    result["hasADF"] = data[0] != 0;
                    break;
                default:
                    result[QString("tag_%1").arg(tag)] = data;
                    break;
            }
        }
    }
    
    return result;
}

void CanonDriverComplete::handleCanonError(int errorCode, const QString &context)
{
    qCWarning(canonDriver) << "Canon错误" << context << "错误代码:" << errorCode;
    
    QString errorMessage;
    switch (errorCode) {
        case 0x01:
            errorMessage = "设备忙碌";
            break;
        case 0x02:
            errorMessage = "纸张卡住";
            break;
        case 0x03:
            errorMessage = "无纸张";
            break;
        case 0x04:
            errorMessage = "盖子打开";
            break;
        default:
            errorMessage = QString("未知错误: %1").arg(errorCode);
            break;
    }
    
    emit errorOccurred(errorMessage);
}

void CanonDriverComplete::updateCanonDeviceStatus()
{
    if (!m_isConnected) {
        return;
    }
    
    QByteArray response = processCanonProtocol(CANON_CMD_GET_STATUS);
    
    if (!response.isEmpty()) {
        QVariantMap status = parseCanonResponse(response);
        if (status != m_deviceStatus) {
            m_deviceStatus = status;
            emit canonStatusChanged(status);
        }
    }
}

void CanonDriverComplete::onCanonStatusTimer()
{
    updateCanonDeviceStatus();
}

void CanonDriverComplete::onCanonDataReady()
{
    if (!m_isScanning) {
        return;
    }
    
    // 读取扫描数据
    unsigned char buffer[8192];
    int bytesRead = readCanonScanData(buffer, sizeof(buffer));
    
    if (bytesRead > 0) {
        QMutexLocker locker(&m_scanMutex);
        m_scanBuffer.append(reinterpret_cast<char*>(buffer), bytesRead);
        
        // 更新进度（简化计算）
        m_scanProgress = qMin(100, m_scanProgress + 1);
        
        emit dataReady();
        
        if (m_scanProgress >= 100) {
            m_isScanning = false;
            m_dataTimer->stop();
            emit scanCompleted();
        }
    }
}

int CanonDriverComplete::readCanonScanData(unsigned char *buffer, int size)
{
    QByteArray response = processCanonProtocol(CANON_CMD_READ_DATA);
    
    if (response.isEmpty()) {
        return 0;
    }
    
    int dataSize = qMin(size, response.size() - 1); // 排除状态字节
    if (dataSize > 0) {
        memcpy(buffer, response.constData() + 1, dataSize);
    }
    
    return dataSize;
}

// #include "canon_driver_complete.moc" 