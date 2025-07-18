// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EPSON_DRIVER_COMPLETE_H
#define EPSON_DRIVER_COMPLETE_H

#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerDriver.h"
#include "Scanner/DScannerTypes.h"
#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QLoggingCategory>
#include <QMap>
#include <QVariantMap>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <libusb-1.0/libusb.h>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(epsonDriver)

/**
 * @brief Epson扫描仪完整驱动类
 * 
 * 支持Epson各个系列的扫描仪，包括Perfection系列、Expression系列、WorkForce系列等，
 * 实现了Epson的ESC/I协议和网络通信协议。
 */
class DSCANNER_EXPORT EpsonDriverComplete : public DScannerDriver
{
    Q_OBJECT

public:
    /**
     * @brief Epson设备系列枚举
     */
    enum class EpsonSeries {
        Unknown,
        Perfection,      // Perfection系列 (消费级平板扫描仪)
        Expression,      // Expression系列 (专业级扫描仪)
        WorkForce,       // WorkForce系列 (商用多功能设备)
        FastFoto,        // FastFoto系列 (照片扫描仪)
        DocumentScanner  // DS系列文档扫描仪
    };

    /**
     * @brief Epson通信协议枚举
     */
    enum class EpsonProtocol {
        USB_Direct,      // 直接USB通信
        ESC_I,          // Epson ESC/I协议
        IPP_eSCL,       // IPP/eSCL协议
        WSD,            // WS-Discovery协议
        Network_Direct   // 直接网络通信
    };

    /**
     * @brief 连接类型枚举
     */
    enum class ConnectionType {
        USB,             // USB连接
        Network,         // 网络连接
        Unknown          // 未知连接
    };

    /**
     * @brief Epson设备能力结构
     */
    struct EpsonCapabilities {
        QList<int> supportedResolutions;     // 支持的分辨率
        QList<ScanMode> supportedModes;      // 支持的扫描模式
        QSize maxScanSize;                   // 最大扫描尺寸
        bool hasADF;                         // 是否有自动文档进纸器
        bool hasDuplex;                      // 是否支持双面扫描
        bool hasTransparency;                // 是否支持透明度扫描
        bool hasFilmHolder;                  // 是否有胶片夹
        int maxBitDepth;                     // 最大位深度
        QStringList supportedFormats;       // 支持的图像格式
        QVariantMap epsonFeatures;          // Epson特有功能
    };

public:
    explicit EpsonDriverComplete(QObject *parent = nullptr);
    ~EpsonDriverComplete();

    // 基本设备操作
    bool initialize() override;
    void cleanup() override;
    bool isDeviceSupported(const DeviceInfo &deviceInfo) override;
    bool connectDevice(const DeviceInfo &deviceInfo) override;
    void disconnectDevice() override;
    bool isConnected() const override;

    // 设备信息和能力
    DeviceInfo getCurrentDeviceInfo() const override;
    QVariantMap getDeviceCapabilities() const override;
    QStringList getSupportedOptions() const override;
    QVariant getOptionValue(const QString &option) const override;
    bool setOptionValue(const QString &option, const QVariant &value) override;

    // 扫描操作
    bool startScan(const ScanParameters &params) override;
    void cancelScan() override;
    QByteArray readScanData() override;
    bool isScanComplete() const override;
    int getScanProgress() const override;

    // Epson特有方法
    /**
     * @brief 检测Epson设备系列
     * @param deviceInfo 设备信息
     * @return Epson设备系列
     */
    EpsonSeries detectEpsonSeries(const DeviceInfo &deviceInfo) const;

    /**
     * @brief 获取Epson设备能力
     * @return Epson设备能力结构
     */
    EpsonCapabilities getEpsonCapabilities() const;

    /**
     * @brief 执行Epson设备校准
     * @return 是否成功
     */
    bool performEpsonCalibration();

    /**
     * @brief 获取Epson设备状态
     * @return 设备状态信息
     */
    QVariantMap getEpsonDeviceStatus() const;

    /**
     * @brief 执行Epson自动色彩还原
     * @param enable 是否启用
     * @return 是否成功
     */
    bool setEpsonAutoColorRestoration(bool enable);

    /**
     * @brief 设置Epson除尘功能
     * @param level 除尘级别 (0-3)
     * @return 是否成功
     */
    bool setEpsonDustRemoval(int level);

    /**
     * @brief 设置Epson数字ICE技术
     * @param enable 是否启用
     * @return 是否成功
     */
    bool setEpsonDigitalICE(bool enable);

Q_SIGNALS:
    /**
     * @brief Epson设备状态变化信号
     * @param status 状态信息
     */
    void epsonStatusChanged(const QVariantMap &status);

    /**
     * @brief Epson校准完成信号
     * @param success 是否成功
     */
    void epsonCalibrationCompleted(bool success);

    /**
     * @brief Epson特殊功能状态信号
     * @param feature 功能名称
     * @param status 状态信息
     */
    void epsonFeatureStatus(const QString &feature, const QVariantMap &status);

private Q_SLOTS:
    void onEpsonStatusTimer();
    void onEpsonDataReady();

private:
    /**
     * @brief 初始化Epson设备支持
     */
    void initializeEpsonSupport();

    /**
     * @brief 加载Epson设备数据库
     */
    void loadEpsonDeviceDatabase();

    /**
     * @brief 识别Epson设备型号
     * @param deviceInfo 设备信息
     * @return 是否成功识别
     */
    bool identifyEpsonDevice(const DeviceInfo &deviceInfo);

    /**
     * @brief 建立Epson USB连接
     * @return 是否成功
     */
    bool establishEpsonUSBConnection();

    /**
     * @brief 建立Epson网络连接
     * @return 是否成功
     */
    bool establishEpsonNetworkConnection();

    /**
     * @brief 执行Epson设备初始化
     * @return 是否成功
     */
    bool performEpsonDeviceInitialization();

    /**
     * @brief 读取Epson设备能力
     * @return 是否成功
     */
    bool readEpsonDeviceCapabilities();

    /**
     * @brief 设置扫描分辨率
     * @param resolution 分辨率
     * @return 是否成功
     */
    bool setResolution(int resolution);

    /**
     * @brief 设置扫描模式
     * @param mode 扫描模式
     * @return 是否成功
     */
    bool setScanMode(ScanMode mode);

    /**
     * @brief 设置扫描区域
     * @param area 扫描区域
     * @return 是否成功
     */
    bool setScanArea(const QRect &area);

    /**
     * @brief 设置亮度
     * @param brightness 亮度值
     * @return 是否成功
     */
    bool setBrightness(int brightness);

    /**
     * @brief 设置对比度
     * @param contrast 对比度值
     * @return 是否成功
     */
    bool setContrast(int contrast);

    /**
     * @brief 设置伽马值
     * @param gamma 伽马值
     * @return 是否成功
     */
    bool setGamma(double gamma);

    /**
     * @brief 设置自动色彩恢复
     * @param enabled 是否启用
     * @return 是否成功
     */
    bool setAutoColorRestoration(bool enabled);

    /**
     * @brief 设置除尘功能
     * @param enabled 是否启用
     * @return 是否成功
     */
    bool setDustRemoval(bool enabled);

    /**
     * @brief 设置数字ICE
     * @param enabled 是否启用
     * @return 是否成功
     */
    bool setDigitalICE(bool enabled);

    /**
     * @brief 配置Epson扫描参数
     * @param params 扫描参数
     * @return 是否成功
     */
    bool configureEpsonScanParameters(const ScanParameters &params);

    /**
     * @brief 启动Epson扫描过程
     * @return 是否成功
     */
    bool startEpsonScanProcess();

    /**
     * @brief 读取Epson扫描数据
     * @param buffer 数据缓冲区
     * @param size 缓冲区大小
     * @return 实际读取的字节数
     */
    int readEpsonScanData(unsigned char *buffer, int size);

    /**
     * @brief 处理Epson ESC/I协议通信
     * @param command 命令
     * @param data 数据
     * @return 响应数据
     */
    QByteArray processEpsonESCI(const QString &command, const QByteArray &data = QByteArray());

    /**
     * @brief 处理Epson网络协议通信
     * @param request 请求数据
     * @return 响应数据
     */
    QByteArray processEpsonNetworkProtocol(const QByteArray &request);

    /**
     * @brief 解析Epson设备响应
     * @param response 响应数据
     * @return 解析结果
     */
    QVariantMap parseEpsonResponse(const QByteArray &response);

    /**
     * @brief 构建ESC/I命令
     * @param command 命令字符串
     * @param parameters 参数
     * @return 完整的ESC/I命令
     */
    QByteArray buildESCICommand(const QString &command, const QVariantMap &parameters = QVariantMap());

    /**
     * @brief 错误处理
     * @param errorCode 错误代码
     * @param context 错误上下文
     */
    void handleEpsonError(const QString &errorCode, const QString &context);

    /**
     * @brief 更新Epson设备状态
     */
    void updateEpsonDeviceStatus();

    /**
     * @brief 解析Epson状态代码
     * @param statusCode 状态代码
     * @return 状态信息
     */
    QVariantMap parseEpsonStatusCode(const QString &statusCode);

    // Connection methods
    bool connectUSBDirect();
    bool connectESCI();
    bool connectIPP();
    bool connectWSD();
    bool connectNetworkDirect();
    
    void disconnectUSBDirect();
    void disconnectESCI();
    void disconnectIPP();
    void disconnectWSD();
    void disconnectNetworkDirect();
    
    // Device monitoring
    void checkDeviceStatus();
    void onNetworkReplyFinished(QNetworkReply *reply);
    
    // ESCI protocol functions
    bool sendESCICommand(const QByteArray &command);
    QByteArray readESCIResponse();
    void readDeviceCapabilities();
    void parseCapabilities(const QByteArray &data);
    bool applyEpsonScanParameters(const ScanParameters &params);
    void monitorScanProgress();
    
    // Data reading functions
    QByteArray readUSBScanData();
    QByteArray readESCIScanData();
    QByteArray readIPPScanData();
    QByteArray readNetworkScanData();
    
    // Network discovery functions
    void queryAdvancedFeatures();
    QByteArray buildIPPQueryMessage();
    
    // Device series detection
    EpsonSeries detectDeviceSeries(const DeviceInfo &deviceInfo);
    EpsonProtocol determineConnectionProtocol(const DeviceInfo &deviceInfo);

private:
    // 设备连接信息
    DeviceInfo m_currentDevice;
    EpsonSeries m_deviceSeries;
    EpsonProtocol m_protocol;
    bool m_isConnected;

    // USB连接
    libusb_context *m_usbContext;
    libusb_device_handle *m_usbHandle;
    int m_usbInterface;
    unsigned char m_bulkInEndpoint;
    unsigned char m_bulkOutEndpoint;

    // 网络连接
    QString m_networkHost;
    int m_networkPort;
    QTcpSocket *m_networkSocket;

    // 设备能力和状态
    EpsonCapabilities m_capabilities;
    EpsonCapabilities m_deviceCapabilities; // 添加缺失的成员变量
    QVariantMap m_deviceStatus;
    QMap<QString, QVariant> m_currentOptions;

    // 扫描状态
    bool m_isScanning;
    bool m_scanInProgress; // 添加缺失的成员变量
    int m_scanProgress;
    QByteArray m_scanBuffer;
    mutable QMutex m_scanMutex;
    mutable QMutex m_mutex; // 添加缺失的成员变量

    // 错误处理
    QString m_lastError; // 添加缺失的成员变量
    ConnectionType m_connectionType; // 添加连接类型

    // Epson特有状态
    bool m_autoColorRestoration;
    int m_dustRemovalLevel;
    bool m_digitalICE;

    // 定时器和监控
    QTimer *m_statusTimer;
    QTimer *m_dataTimer;

    // Epson设备数据库
    QMap<QString, EpsonCapabilities> m_deviceDatabase;
    QMap<quint16, EpsonSeries> m_seriesDatabase;

    // ESC/I命令常量
    static const QString ESCI_INITIALIZE;
    static const QString ESCI_GET_STATUS;
    static const QString ESCI_SET_PARAMS;
    static const QString ESCI_START_SCAN;
    static const QString ESCI_READ_DATA;
    static const QString ESCI_STOP_SCAN;
    static const QString ESCI_CALIBRATE;
    static const QString ESCI_GET_CAPS;
    static const QString ESCI_AUTO_COLOR;
    static const QString ESCI_DUST_REMOVAL;
    static const QString ESCI_DIGITAL_ICE;

    // 网络协议常量
    static const int EPSON_NETWORK_PORT = 1865;
    static const int ESCI_TIMEOUT = 5000;
};

DSCANNER_END_NAMESPACE

Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::EpsonDriverComplete::EpsonSeries)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::EpsonDriverComplete::EpsonProtocol)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::EpsonDriverComplete::EpsonCapabilities)

#endif // EPSON_DRIVER_COMPLETE_H 