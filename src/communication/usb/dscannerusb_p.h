// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERUSB_P_H
#define DSCANNERUSB_P_H

#include "Scanner/DScannerUSB.h"

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QThread>
#include <QHash>
#include <QQueue>

// 前向声明libusb结构体
struct libusb_context;
struct libusb_device;
struct libusb_device_handle;
struct libusb_device_descriptor;
struct libusb_config_descriptor;
struct libusb_interface_descriptor;
struct libusb_endpoint_descriptor;
struct libusb_transfer;

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief USB传输请求结构
 */
struct USBTransferRequest {
    quint8 endpoint;                ///< 端点地址
    QByteArray data;                ///< 传输数据
    int timeout;                    ///< 超时时间
    bool isInput;                   ///< 是否为输入传输
    USBTransferType type;           ///< 传输类型
    
    // 控制传输专用字段
    quint8 requestType;             ///< 请求类型
    quint8 request;                 ///< 请求
    quint16 value;                  ///< 值
    quint16 index;                  ///< 索引
    
    USBTransferRequest()
        : endpoint(0)
        , timeout(1000)
        , isInput(false)
        , type(USBTransferType::Bulk)
        , requestType(0)
        , request(0)
        , value(0)
        , index(0)
    {}
};

/**
 * @brief USB传输结果结构
 */
struct USBTransferResult {
    int status;                     ///< 传输状态
    QByteArray data;                ///< 传输数据
    int actualLength;               ///< 实际传输长度
    QString errorMessage;           ///< 错误信息
    
    USBTransferResult()
        : status(0)
        , actualLength(0)
    {}
};

/**
 * @brief USB设备监控线程
 */
class USBDeviceMonitor : public QThread
{
    Q_OBJECT

public:
    explicit USBDeviceMonitor(QObject *parent = nullptr);
    ~USBDeviceMonitor();

    void startMonitoring();
    void stopMonitoring();

signals:
    void deviceConnected(const USBDeviceDescriptor &descriptor);
    void deviceDisconnected(const QString &devicePath);

protected:
    void run() override;

private:
    bool monitoring;
    QMutex monitorMutex;
    QHash<QString, USBDeviceDescriptor> knownDevices;
    
    QList<USBDeviceDescriptor> scanUSBDevices();
    bool isScannerDevice(const USBDeviceDescriptor &descriptor);
};

/**
 * @brief DScannerUSB的私有实现类
 */
class DScannerUSBPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(DScannerUSB)

public:
    explicit DScannerUSBPrivate(DScannerUSB *q);
    virtual ~DScannerUSBPrivate();

    // libusb初始化和清理
    bool initializeLibUSB();
    void shutdownLibUSB();
    bool isLibUSBInitialized() const;

    // 设备发现和管理
    QList<USBDeviceDescriptor> discoverUSBDevices(quint16 vendorId, quint16 productId);
    bool openUSBDevice(quint16 vendorId, quint16 productId, const QString &serialNumber);
    bool openUSBDeviceByPath(const QString &devicePath);
    void closeUSBDevice();
    bool isUSBDeviceOpen() const;

    // 设备配置
    bool setUSBConfiguration(quint8 configuration);
    bool claimUSBInterface(quint8 interfaceNumber);
    bool releaseUSBInterface(quint8 interfaceNumber);
    bool setUSBInterfaceAltSetting(quint8 interfaceNumber, quint8 alternateSetting);

    // 数据传输
    USBTransferResult performControlTransfer(const USBTransferRequest &request);
    USBTransferResult performBulkTransfer(const USBTransferRequest &request);
    USBTransferResult performInterruptTransfer(const USBTransferRequest &request);

    // 设备信息查询
    USBDeviceDescriptor getCurrentUSBDeviceDescriptor() const;
    QString getUSBStringDescriptor(quint8 index) const;
    bool resetUSBDevice();
    bool clearUSBHalt(quint8 endpoint);

    // 错误处理
    QString getLastUSBError() const;
    int getLastUSBErrorCode() const;
    void setLastError(int errorCode, const QString &errorMessage);

    // libusb辅助函数
    USBDeviceDescriptor convertToDeviceDescriptor(libusb_device *device);
    USBConfiguration convertToConfiguration(const libusb_config_descriptor *config);
    USBInterface convertToInterface(const libusb_interface_descriptor *interface);
    USBEndpoint convertToEndpoint(const libusb_endpoint_descriptor *endpoint);

    // 异步传输支持
    void processAsyncTransfers();
    void handleTransferComplete(libusb_transfer *transfer);

public:
    DScannerUSB *q_ptr;

    // libusb上下文和状态
    libusb_context *usbContext;
    libusb_device_handle *deviceHandle;
    libusb_device *currentDevice;
    bool initialized;
    mutable QMutex usbMutex;

    // 错误信息
    int lastErrorCode;
    QString lastErrorMessage;

    // 设备信息
    USBDeviceDescriptor currentDescriptor;
    QList<quint8> claimedInterfaces;

    // 异步传输管理
    QQueue<libusb_transfer*> pendingTransfers;
    QTimer *eventTimer;
    QThread *eventThread;

    // 设备监控
    USBDeviceMonitor *deviceMonitor;

    // 常量定义
    static constexpr int USB_TIMEOUT_DEFAULT = 1000;
    static constexpr int USB_TIMEOUT_CONTROL = 5000;
    static constexpr int USB_MAX_PACKET_SIZE = 64 * 1024;

private slots:
    void processUSBEvents();
    void onDeviceConnected(const USBDeviceDescriptor &descriptor);
    void onDeviceDisconnected(const QString &devicePath);

private:
    // 禁用拷贝
    Q_DISABLE_COPY(DScannerUSBPrivate)
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERUSB_P_H 