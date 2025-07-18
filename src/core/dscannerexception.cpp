// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerException.h"

DSCANNER_USE_NAMESPACE

DScannerException::DScannerException(ErrorCode code, const QString &message)
    : m_errorCode(code)
    , m_errorMessage(message.isEmpty() ? defaultErrorMessage(code) : message)
{
}

DScannerException::DScannerException(const QString &message)
    : m_errorCode(ErrorCode::Unknown)
    , m_errorMessage(message)
{
}

DScannerException::DScannerException(const DScannerException &other)
    : QException(other)
    , m_errorCode(other.m_errorCode)
    , m_errorMessage(other.m_errorMessage)
{
}

DScannerException& DScannerException::operator=(const DScannerException &other)
{
    if (this != &other) {
        QException::operator=(other);
        m_errorCode = other.m_errorCode;
        m_errorMessage = other.m_errorMessage;
        m_whatBuffer.clear();
    }
    return *this;
}

DScannerException::~DScannerException() = default;

DScannerException::ErrorCode DScannerException::errorCode() const
{
    return m_errorCode;
}

QString DScannerException::errorMessage() const
{
    return m_errorMessage;
}

const char* DScannerException::what() const noexcept
{
    if (m_whatBuffer.isEmpty()) {
        QString fullMessage = QStringLiteral("[%1] %2")
                             .arg(errorCodeName(m_errorCode))
                             .arg(m_errorMessage);
        m_whatBuffer = fullMessage.toUtf8();
    }
    return m_whatBuffer.constData();
}

DScannerException* DScannerException::clone() const
{
    return new DScannerException(*this);
}

void DScannerException::raise() const
{
    throw *this;
}

QString DScannerException::errorCodeName(ErrorCode code)
{
    switch (code) {
    case ErrorCode::Unknown:
        return QStringLiteral("Unknown");
    case ErrorCode::DeviceNotFound:
        return QStringLiteral("DeviceNotFound");
    case ErrorCode::DeviceNotAvailable:
        return QStringLiteral("DeviceNotAvailable");
    case ErrorCode::DeviceNotConnected:
        return QStringLiteral("DeviceNotConnected");
    case ErrorCode::DeviceAlreadyOpen:
        return QStringLiteral("DeviceAlreadyOpen");
    case ErrorCode::DeviceNotOpen:
        return QStringLiteral("DeviceNotOpen");
    case ErrorCode::DeviceNotReady:
        return QStringLiteral("DeviceNotReady");
    case ErrorCode::DeviceBusy:
        return QStringLiteral("DeviceBusy");
    case ErrorCode::DeviceError:
        return QStringLiteral("DeviceError");
    case ErrorCode::DriverNotFound:
        return QStringLiteral("DriverNotFound");
    case ErrorCode::DriverError:
        return QStringLiteral("DriverError");
    case ErrorCode::CommunicationError:
        return QStringLiteral("CommunicationError");
    case ErrorCode::InvalidParameter:
        return QStringLiteral("InvalidParameter");
    case ErrorCode::InvalidScanArea:
        return QStringLiteral("InvalidScanArea");
    case ErrorCode::InvalidResolution:
        return QStringLiteral("InvalidResolution");
    case ErrorCode::ScanInProgress:
        return QStringLiteral("ScanInProgress");
    case ErrorCode::ScanNotInProgress:
        return QStringLiteral("ScanNotInProgress");
    case ErrorCode::ScanCancelled:
        return QStringLiteral("ScanCancelled");
    case ErrorCode::ScanFailed:
        return QStringLiteral("ScanFailed");
    case ErrorCode::ImageProcessingError:
        return QStringLiteral("ImageProcessingError");
    case ErrorCode::FileIOError:
        return QStringLiteral("FileIOError");
    case ErrorCode::OutOfMemory:
        return QStringLiteral("OutOfMemory");
    case ErrorCode::Timeout:
        return QStringLiteral("Timeout");
    case ErrorCode::TimeoutError:
        return QStringLiteral("TimeoutError");
    case ErrorCode::PermissionDenied:
        return QStringLiteral("PermissionDenied");
    case ErrorCode::NetworkError:
        return QStringLiteral("NetworkError");
    case ErrorCode::ConfigurationError:
        return QStringLiteral("ConfigurationError");
    case ErrorCode::CalibrationError:
        return QStringLiteral("CalibrationError");
    case ErrorCode::LicenseError:
        return QStringLiteral("LicenseError");
    case ErrorCode::NotSupported:
        return QStringLiteral("NotSupported");
    case ErrorCode::NotImplemented:
        return QStringLiteral("NotImplemented");
    default:
        return QStringLiteral("UnknownErrorCode");
    }
}

QString DScannerException::getErrorDescription(ErrorCode code)
{
    switch (code) {
    case ErrorCode::Unknown:
        return QStringLiteral("未知错误");
    case ErrorCode::DeviceNotFound:
        return QStringLiteral("未找到扫描设备");
    case ErrorCode::DeviceNotAvailable:
        return QStringLiteral("设备不可用");
    case ErrorCode::DeviceNotConnected:
        return QStringLiteral("设备未连接");
    case ErrorCode::DeviceAlreadyOpen:
        return QStringLiteral("设备已经打开");
    case ErrorCode::DeviceNotOpen:
        return QStringLiteral("设备未打开");
    case ErrorCode::DeviceNotReady:
        return QStringLiteral("设备未就绪");
    case ErrorCode::DeviceBusy:
        return QStringLiteral("设备忙碌，正在被其他应用程序使用");
    case ErrorCode::DeviceError:
        return QStringLiteral("设备硬件错误");
    case ErrorCode::DriverNotFound:
        return QStringLiteral("未找到设备驱动程序");
    case ErrorCode::DriverError:
        return QStringLiteral("驱动程序错误");
    case ErrorCode::CommunicationError:
        return QStringLiteral("通信错误");
    case ErrorCode::InvalidParameter:
        return QStringLiteral("无效的参数");
    case ErrorCode::InvalidScanArea:
        return QStringLiteral("无效的扫描区域");
    case ErrorCode::InvalidResolution:
        return QStringLiteral("无效的分辨率");
    case ErrorCode::ScanInProgress:
        return QStringLiteral("扫描正在进行中");
    case ErrorCode::ScanNotInProgress:
        return QStringLiteral("扫描未在进行中");
    case ErrorCode::ScanCancelled:
        return QStringLiteral("扫描操作被取消");
    case ErrorCode::ScanFailed:
        return QStringLiteral("扫描失败");
    case ErrorCode::ImageProcessingError:
        return QStringLiteral("图像处理错误");
    case ErrorCode::FileIOError:
        return QStringLiteral("文件输入输出错误");
    case ErrorCode::OutOfMemory:
        return QStringLiteral("内存不足");
    case ErrorCode::Timeout:
        return QStringLiteral("操作超时");
    case ErrorCode::TimeoutError:
        return QStringLiteral("操作超时错误");
    case ErrorCode::PermissionDenied:
        return QStringLiteral("权限被拒绝");
    case ErrorCode::NetworkError:
        return QStringLiteral("网络连接错误");
    case ErrorCode::ConfigurationError:
        return QStringLiteral("配置错误");
    case ErrorCode::CalibrationError:
        return QStringLiteral("设备校准错误");
    case ErrorCode::LicenseError:
        return QStringLiteral("许可证错误");
    case ErrorCode::NotSupported:
        return QStringLiteral("不支持的操作");
    case ErrorCode::NotImplemented:
        return QStringLiteral("未实现的操作");
    default:
        return QStringLiteral("未知错误代码");
    }
}

QString DScannerException::getSuggestion(ErrorCode code)
{
    switch (code) {
    case ErrorCode::DeviceNotFound:
        return QStringLiteral("请检查扫描仪是否正确连接并开机");
    case ErrorCode::DeviceNotConnected:
        return QStringLiteral("请重新连接扫描仪设备");
    case ErrorCode::DeviceBusy:
        return QStringLiteral("请关闭其他使用扫描仪的应用程序后重试");
    case ErrorCode::DriverNotFound:
        return QStringLiteral("请安装相应的设备驱动程序");
    case ErrorCode::DriverLoadError:
        return QStringLiteral("请重新安装设备驱动程序或联系技术支持");
    case ErrorCode::InvalidParameter:
        return QStringLiteral("请检查扫描参数设置是否正确");
    case ErrorCode::OutOfMemory:
        return QStringLiteral("请降低扫描分辨率或释放系统内存");
    case ErrorCode::TimeoutError:
        return QStringLiteral("请检查设备连接或网络状态");
    case ErrorCode::NetworkError:
        return QStringLiteral("请检查网络连接和设备IP地址");
    case ErrorCode::PermissionDenied:
        return QStringLiteral("请以管理员权限运行程序");
    case ErrorCode::IncompatibleDevice:
        return QStringLiteral("此设备不被当前版本支持，请更新软件或联系技术支持");
    case ErrorCode::CommunicationError:
        return QStringLiteral("请检查USB线缆或网络连接");
    case ErrorCode::UnsupportedOperation:
        return QStringLiteral("此设备不支持当前操作");
    case ErrorCode::ConfigurationError:
        return QStringLiteral("请重置设备配置或恢复默认设置");
    case ErrorCode::DataCorruption:
        return QStringLiteral("请重新扫描或检查设备状态");
    case ErrorCode::InitializationError:
        return QStringLiteral("请重启应用程序或重新连接设备");
    default:
        return QStringLiteral("请联系技术支持获取帮助");
    }
}

bool DScannerException::isRecoverable(ErrorCode code)
{
    switch (code) {
    case ErrorCode::DeviceNotConnected:
    case ErrorCode::DeviceBusy:
    case ErrorCode::InvalidParameter:
    case ErrorCode::ScanCancelled:
    case ErrorCode::TimeoutError:
    case ErrorCode::NetworkError:
    case ErrorCode::ConfigurationError:
        return true;
    case ErrorCode::DeviceNotFound:
    case ErrorCode::DriverNotFound:
    case ErrorCode::DriverLoadError:
    case ErrorCode::OutOfMemory:
    case ErrorCode::HardwareError:
    case ErrorCode::PermissionDenied:
    case ErrorCode::IncompatibleDevice:
    case ErrorCode::DataCorruption:
    case ErrorCode::LicenseError:
        return false;
    default:
        return false;
    }
}

QString DScannerException::getRecoveryAction(ErrorCode code)
{
    switch (code) {
    case ErrorCode::DeviceNotConnected:
        return QStringLiteral("尝试重新连接设备");
    case ErrorCode::DeviceBusy:
        return QStringLiteral("等待设备空闲后重试");
    case ErrorCode::InvalidParameter:
        return QStringLiteral("使用默认参数重试");
    case ErrorCode::ScanCancelled:
        return QStringLiteral("重新开始扫描");
    case ErrorCode::TimeoutError:
        return QStringLiteral("增加超时时间重试");
    case ErrorCode::NetworkError:
        return QStringLiteral("检查网络连接后重试");
    case ErrorCode::ConfigurationError:
        return QStringLiteral("重置配置后重试");
    default:
        return QStringLiteral("无自动恢复方案");
    }
}

// 静态函数实现
QString DScannerException::defaultErrorMessage(DScannerException::ErrorCode code) {
    switch (code) {
    case ErrorCode::DeviceNotFound:
        return QStringLiteral("扫描仪设备未找到");
    case ErrorCode::DriverNotFound:
        return QStringLiteral("驱动程序未找到");
    case ErrorCode::InvalidParameter:
        return QStringLiteral("参数无效");
    case ErrorCode::DeviceNotConnected:
        return QStringLiteral("设备未连接");
    case ErrorCode::DeviceBusy:
        return QStringLiteral("设备忙碌");
    case ErrorCode::TimeoutError:
        return QStringLiteral("操作超时");
    case ErrorCode::NetworkError:
        return QStringLiteral("网络错误");
    default:
        return QStringLiteral("未知错误");
    }
} 