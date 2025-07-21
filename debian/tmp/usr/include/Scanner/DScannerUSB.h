// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERUSB_H
#define DSCANNERUSB_H

#include "DScannerGlobal.h"
#include "DScannerTypes.h"

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief USB传输类型枚举
 */
enum class USBTransferType {
    Control = 0,            ///< 控制传输
    Isochronous,            ///< 同步传输
    Bulk,                   ///< 批量传输
    Interrupt               ///< 中断传输
};

/**
 * @brief USB端点信息
 */
struct USBEndpoint {
    quint8 address;         ///< 端点地址
    USBTransferType type;   ///< 传输类型
    quint16 maxPacketSize;  ///< 最大包大小
    quint8 interval;        ///< 轮询间隔
    bool isInput;           ///< 是否为输入端点
    
    USBEndpoint()
        : address(0)
        , type(USBTransferType::Bulk)
        , maxPacketSize(0)
        , interval(0)
        , isInput(false)
    {}
};

/**
 * @brief USB接口信息
 */
struct USBInterface {
    quint8 interfaceNumber;     ///< 接口号
    quint8 alternateSetting;    ///< 备用设置
    quint8 interfaceClass;      ///< 接口类
    quint8 interfaceSubClass;   ///< 接口子类
    quint8 interfaceProtocol;   ///< 接口协议
    QString description;        ///< 接口描述
    QList<USBEndpoint> endpoints; ///< 端点列表
    
    USBInterface()
        : interfaceNumber(0)
        , alternateSetting(0)
        , interfaceClass(0)
        , interfaceSubClass(0)
        , interfaceProtocol(0)
    {}
};

/**
 * @brief USB配置信息
 */
struct USBConfiguration {
    quint8 configurationValue;     ///< 配置值
    QString description;            ///< 配置描述
    quint8 attributes;              ///< 配置属性
    quint8 maxPower;                ///< 最大功率
    QList<USBInterface> interfaces; ///< 接口列表
    
    USBConfiguration()
        : configurationValue(0)
        , attributes(0)
        , maxPower(0)
    {}
};

/**
 * @brief 扩展的USB设备信息
 */
struct USBDeviceDescriptor {
    // 基本设备信息
    quint16 vendorId;               ///< 厂商ID
    quint16 productId;              ///< 产品ID
    quint16 deviceVersion;          ///< 设备版本
    QString manufacturer;           ///< 厂商名称
    QString product;                ///< 产品名称
    QString serialNumber;           ///< 序列号
    
    // USB描述符信息
    quint8 deviceClass;             ///< 设备类
    quint8 deviceSubClass;          ///< 设备子类
    quint8 deviceProtocol;          ///< 设备协议
    quint8 maxPacketSize0;          ///< 端点0最大包大小
    
    // 物理连接信息
    quint8 busNumber;               ///< 总线号
    quint8 deviceAddress;           ///< 设备地址
    QString devicePath;             ///< 设备路径
    
    // 配置信息
    QList<USBConfiguration> configurations; ///< 配置列表
    
    USBDeviceDescriptor()
        : vendorId(0)
        , productId(0)
        , deviceVersion(0)
        , deviceClass(0)
        , deviceSubClass(0)
        , deviceProtocol(0)
        , maxPacketSize0(0)
        , busNumber(0)
        , deviceAddress(0)
    {}
};

/**
 * @brief USB通信接口
 * 
 * 提供对USB设备的底层通信支持，基于libusb实现
 */
class DSCANNER_EXPORT DScannerUSB : public QObject
{
    Q_OBJECT

public:
    explicit DScannerUSB(QObject *parent = nullptr);
    virtual ~DScannerUSB();

    // USB子系统管理
    /**
     * @brief 初始化USB子系统
     * @return 是否成功
     */
    bool initialize();

    /**
     * @brief 清理USB子系统
     */
    void shutdown();

    /**
     * @brief 检查USB子系统是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const;

    // 设备发现和管理
    /**
     * @brief 发现USB设备
     * @param vendorId 厂商ID过滤，0表示不过滤
     * @param productId 产品ID过滤，0表示不过滤
     * @return 设备列表
     */
    QList<USBDeviceDescriptor> discoverDevices(quint16 vendorId = 0, quint16 productId = 0);

    /**
     * @brief 打开USB设备
     * @param vendorId 厂商ID
     * @param productId 产品ID
     * @param serialNumber 序列号，空表示打开第一个匹配的设备
     * @return 是否成功
     */
    bool openDevice(quint16 vendorId, quint16 productId, const QString &serialNumber = QString());

    /**
     * @brief 通过设备路径打开设备
     * @param devicePath 设备路径
     * @return 是否成功
     */
    bool openDeviceByPath(const QString &devicePath);

    /**
     * @brief 关闭当前设备
     */
    void closeDevice();

    /**
     * @brief 检查设备是否已打开
     * @return 是否已打开
     */
    bool isDeviceOpen() const;

    // 设备配置
    /**
     * @brief 设置设备配置
     * @param configuration 配置值
     * @return 是否成功
     */
    bool setConfiguration(quint8 configuration);

    /**
     * @brief 声明接口
     * @param interfaceNumber 接口号
     * @return 是否成功
     */
    bool claimInterface(quint8 interfaceNumber);

    /**
     * @brief 释放接口
     * @param interfaceNumber 接口号
     * @return 是否成功
     */
    bool releaseInterface(quint8 interfaceNumber);

    /**
     * @brief 设置备用设置
     * @param interfaceNumber 接口号
     * @param alternateSetting 备用设置
     * @return 是否成功
     */
    bool setInterfaceAltSetting(quint8 interfaceNumber, quint8 alternateSetting);

    // 数据传输
    /**
     * @brief 控制传输
     * @param requestType 请求类型
     * @param request 请求
     * @param value 值
     * @param index 索引
     * @param data 数据缓冲区
     * @param timeout 超时时间（毫秒）
     * @return 传输的字节数，失败返回-1
     */
    int controlTransfer(quint8 requestType, quint8 request, quint16 value, quint16 index,
                        QByteArray &data, int timeout = 1000);

    /**
     * @brief 批量传输（输出）
     * @param endpoint 端点地址
     * @param data 要发送的数据
     * @param timeout 超时时间（毫秒）
     * @return 传输的字节数，失败返回-1
     */
    int bulkTransferOut(quint8 endpoint, const QByteArray &data, int timeout = 1000);

    /**
     * @brief 批量传输（输入）
     * @param endpoint 端点地址
     * @param maxLength 最大接收长度
     * @param timeout 超时时间（毫秒）
     * @return 接收到的数据
     */
    QByteArray bulkTransferIn(quint8 endpoint, int maxLength, int timeout = 1000);

    /**
     * @brief 中断传输（输出）
     * @param endpoint 端点地址
     * @param data 要发送的数据
     * @param timeout 超时时间（毫秒）
     * @return 传输的字节数，失败返回-1
     */
    int interruptTransferOut(quint8 endpoint, const QByteArray &data, int timeout = 1000);

    /**
     * @brief 中断传输（输入）
     * @param endpoint 端点地址
     * @param maxLength 最大接收长度
     * @param timeout 超时时间（毫秒）
     * @return 接收到的数据
     */
    QByteArray interruptTransferIn(quint8 endpoint, int maxLength, int timeout = 1000);

    // 设备信息查询
    /**
     * @brief 获取当前设备描述符
     * @return 设备描述符
     */
    USBDeviceDescriptor getCurrentDeviceDescriptor() const;

    /**
     * @brief 获取设备字符串描述符
     * @param index 描述符索引
     * @return 字符串描述符
     */
    QString getStringDescriptor(quint8 index) const;

    /**
     * @brief 重置设备
     * @return 是否成功
     */
    bool resetDevice();

    /**
     * @brief 清除端点停滞状态
     * @param endpoint 端点地址
     * @return 是否成功
     */
    bool clearHalt(quint8 endpoint);

    // 错误处理
    /**
     * @brief 获取最后的错误信息
     * @return 错误信息
     */
    QString lastError() const;

    /**
     * @brief 获取USB错误代码
     * @return 错误代码
     */
    int lastErrorCode() const;

    // 静态工具方法
    /**
     * @brief 获取USB传输类型名称
     * @param type 传输类型
     * @return 类型名称
     */
    static QString transferTypeName(USBTransferType type);

    /**
     * @brief 检查是否为扫描仪设备
     * @param descriptor 设备描述符
     * @return 是否为扫描仪
     */
    static bool isScannerDevice(const USBDeviceDescriptor &descriptor);

    /**
     * @brief 格式化设备路径
     * @param busNumber 总线号
     * @param deviceAddress 设备地址
     * @return 设备路径
     */
    static QString formatDevicePath(quint8 busNumber, quint8 deviceAddress);

signals:
    /**
     * @brief 设备连接信号
     * @param descriptor 设备描述符
     */
    void deviceConnected(const USBDeviceDescriptor &descriptor);

    /**
     * @brief 设备断开信号
     * @param devicePath 设备路径
     */
    void deviceDisconnected(const QString &devicePath);

    /**
     * @brief 数据接收信号
     * @param endpoint 端点地址
     * @param data 接收到的数据
     */
    void dataReceived(quint8 endpoint, const QByteArray &data);

    /**
     * @brief 传输完成信号
     * @param endpoint 端点地址
     * @param bytesTransferred 传输字节数
     */
    void transferCompleted(quint8 endpoint, int bytesTransferred);

    /**
     * @brief 错误发生信号
     * @param errorCode 错误代码
     * @param errorMessage 错误信息
     */
    void errorOccurred(int errorCode, const QString &errorMessage);

private slots:
    void processUSBEvents();
    void checkDeviceStatus();

private:
    class DScannerUSBPrivate;
    DScannerUSBPrivate *d_ptr;
    Q_DECLARE_PRIVATE(DScannerUSB)
    Q_DISABLE_COPY(DScannerUSB)
};

DSCANNER_END_NAMESPACE

Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::USBTransferType)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::USBEndpoint)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::USBInterface)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::USBConfiguration)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::USBDeviceDescriptor)

#endif // DSCANNERUSB_H 