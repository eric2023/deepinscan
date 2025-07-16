// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CANON_DRIVER_COMPLETE_H
#define CANON_DRIVER_COMPLETE_H

#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerDriver.h"
#include "Scanner/DScannerTypes.h"
#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QLoggingCategory>
#include <QMap>
#include <QVariantMap>
#include <libusb-1.0/libusb.h>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(canonDriver)

/**
 * @brief Canon扫描仪完整驱动类
 * 
 * 支持Canon各个系列的扫描仪，包括CanoScan系列、imageFORMULA系列等，
 * 实现了Canon私有的BJNP协议和USB通信协议。
 */
class DSCANNER_EXPORT CanonDriverComplete : public DScannerDriver
{
    Q_OBJECT

public:
    /**
     * @brief Canon设备系列枚举
     */
    enum class CanonSeries {
        Unknown,
        CanoScan,        // CanoScan系列 (消费级)
        ImageFormula,    // imageFORMULA系列 (商用)
        PixmaMF,        // PIXMA MF系列 (多功能一体机)
        PixmaTS,        // PIXMA TS系列 (打印扫描一体机)
        ScanFrontDR     // ScanFront DR系列 (高端商用)
    };

    /**
     * @brief Canon通信协议枚举
     */
    enum class CanonProtocol {
        USB_Direct,     // 直接USB通信
        BJNP,          // Canon BJNP网络协议
        IPP_eSCL,      // IPP/eSCL协议
        WSD            // WS-Discovery协议
    };

    /**
     * @brief Canon设备能力结构
     */
    struct CanonCapabilities {
        QList<int> supportedResolutions;     // 支持的分辨率
        QList<ScanMode> supportedModes;      // 支持的扫描模式
        QSize maxScanSize;                   // 最大扫描尺寸
        bool hasADF;                         // 是否有自动文档进纸器
        bool hasDuplex;                      // 是否支持双面扫描
        bool hasTransparency;                // 是否支持透明度扫描
        int maxBitDepth;                     // 最大位深度
        QStringList supportedFormats;       // 支持的图像格式
        QVariantMap advancedFeatures;       // 高级功能
    };

public:
    explicit CanonDriverComplete(QObject *parent = nullptr);
    ~CanonDriverComplete();

    // 基本设备操作
    bool initialize() override;
    void cleanup() override;
    bool isDeviceSupported(const DScannerDeviceInfo &deviceInfo) override;
    bool connectDevice(const DScannerDeviceInfo &deviceInfo) override;
    void disconnectDevice() override;
    bool isConnected() const override;

    // 设备信息和能力
    DScannerDeviceInfo getCurrentDeviceInfo() const override;
    QVariantMap getDeviceCapabilities() const override;
    QStringList getSupportedOptions() const override;
    QVariant getOptionValue(const QString &option) const override;
    bool setOptionValue(const QString &option, const QVariant &value) override;

    // 扫描操作
    bool startScan(const DScannerScanParameters &params) override;
    void cancelScan() override;
    QByteArray readScanData() override;
    bool isScanComplete() const override;
    int getScanProgress() const override;

    // Canon特有方法
    /**
     * @brief 检测Canon设备系列
     * @param deviceInfo 设备信息
     * @return Canon设备系列
     */
    CanonSeries detectCanonSeries(const DScannerDeviceInfo &deviceInfo) const;

    /**
     * @brief 获取Canon设备能力
     * @return Canon设备能力结构
     */
    CanonCapabilities getCanonCapabilities() const;

    /**
     * @brief 执行Canon设备校准
     * @return 是否成功
     */
    bool performCanonCalibration();

    /**
     * @brief 获取Canon设备状态
     * @return 设备状态信息
     */
    QVariantMap getCanonDeviceStatus() const;

    /**
     * @brief 设置Canon高级选项
     * @param options 选项映射
     * @return 是否成功
     */
    bool setCanonAdvancedOptions(const QVariantMap &options);

Q_SIGNALS:
    /**
     * @brief Canon设备状态变化信号
     * @param status 状态信息
     */
    void canonStatusChanged(const QVariantMap &status);

    /**
     * @brief Canon校准完成信号
     * @param success 是否成功
     */
    void canonCalibrationCompleted(bool success);

private Q_SLOTS:
    void onCanonStatusTimer();
    void onCanonDataReady();

private:
    /**
     * @brief 初始化Canon设备支持
     */
    void initializeCanonSupport();

    /**
     * @brief 加载Canon设备数据库
     */
    void loadCanonDeviceDatabase();

    /**
     * @brief 识别Canon设备型号
     * @param deviceInfo 设备信息
     * @return 是否成功识别
     */
    bool identifyCanonDevice(const DScannerDeviceInfo &deviceInfo);

    /**
     * @brief 建立Canon USB连接
     * @return 是否成功
     */
    bool establishCanonUSBConnection();

    /**
     * @brief 建立Canon网络连接
     * @return 是否成功
     */
    bool establishCanonNetworkConnection();

    /**
     * @brief 执行Canon设备初始化
     * @return 是否成功
     */
    bool performCanonDeviceInitialization();

    /**
     * @brief 读取Canon设备能力
     * @return 是否成功
     */
    bool readCanonDeviceCapabilities();

    /**
     * @brief 配置Canon扫描参数
     * @param params 扫描参数
     * @return 是否成功
     */
    bool configureCanonScanParameters(const DScannerScanParameters &params);

    /**
     * @brief 启动Canon扫描过程
     * @return 是否成功
     */
    bool startCanonScanProcess();

    /**
     * @brief 读取Canon扫描数据
     * @param buffer 数据缓冲区
     * @param size 缓冲区大小
     * @return 实际读取的字节数
     */
    int readCanonScanData(unsigned char *buffer, int size);

    /**
     * @brief 处理Canon协议通信
     * @param command 命令
     * @param data 数据
     * @return 响应数据
     */
    QByteArray processCanonProtocol(quint8 command, const QByteArray &data = QByteArray());

    /**
     * @brief Canon BJNP协议通信
     * @param request BJNP请求
     * @return BJNP响应
     */
    QByteArray canonBJNPCommunication(const QByteArray &request);

    /**
     * @brief 解析Canon设备响应
     * @param response 响应数据
     * @return 解析结果
     */
    QVariantMap parseCanonResponse(const QByteArray &response);

    /**
     * @brief 错误处理
     * @param errorCode 错误代码
     * @param context 错误上下文
     */
    void handleCanonError(int errorCode, const QString &context);

    /**
     * @brief 更新Canon设备状态
     */
    void updateCanonDeviceStatus();

private:
    // 设备连接信息
    DScannerDeviceInfo m_currentDevice;
    CanonSeries m_deviceSeries;
    CanonProtocol m_protocol;
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
    CanonCapabilities m_capabilities;
    QVariantMap m_deviceStatus;
    QMap<QString, QVariant> m_currentOptions;

    // 扫描状态
    bool m_isScanning;
    int m_scanProgress;
    QByteArray m_scanBuffer;
    mutable QMutex m_scanMutex;

    // 定时器和监控
    QTimer *m_statusTimer;
    QTimer *m_dataTimer;

    // Canon设备数据库
    QMap<QString, CanonCapabilities> m_deviceDatabase;
    QMap<quint16, CanonSeries> m_seriesDatabase;

    // 协议常量
    static const quint8 CANON_CMD_INITIALIZE = 0x01;
    static const quint8 CANON_CMD_GET_STATUS = 0x02;
    static const quint8 CANON_CMD_SET_PARAMS = 0x03;
    static const quint8 CANON_CMD_START_SCAN = 0x04;
    static const quint8 CANON_CMD_READ_DATA = 0x05;
    static const quint8 CANON_CMD_STOP_SCAN = 0x06;
    static const quint8 CANON_CMD_CALIBRATE = 0x07;
    static const quint8 CANON_CMD_GET_CAPS = 0x08;

    // BJNP协议常量
    static const quint16 BJNP_PORT = 9600;
    static const quint8 BJNP_VERSION = 0x01;
    static const quint8 BJNP_TYPE_SCAN = 0x32;
};

DSCANNER_END_NAMESPACE

Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::CanonDriverComplete::CanonSeries)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::CanonDriverComplete::CanonProtocol)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::CanonDriverComplete::CanonCapabilities)

#endif // CANON_DRIVER_COMPLETE_H 