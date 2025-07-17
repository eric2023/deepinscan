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
    case NoError:
        return QStringLiteral("NoError");
    case UnknownError:
        return QStringLiteral("UnknownError");
    case DeviceNotFound:
        return QStringLiteral("DeviceNotFound");
    case DeviceNotConnected:
        return QStringLiteral("DeviceNotConnected");
    case DeviceBusy:
        return QStringLiteral("DeviceBusy");
    case DeviceError:
        return QStringLiteral("DeviceError");
    case DriverNotFound:
        return QStringLiteral("DriverNotFound");
    case DriverLoadError:
        return QStringLiteral("DriverLoadError");
    case InvalidParameter:
        return QStringLiteral("InvalidParameter");
    case OperationFailed:
        return QStringLiteral("OperationFailed");
    case ScanCancelled:
        return QStringLiteral("ScanCancelled");
    case OutOfMemory:
        return QStringLiteral("OutOfMemory");
    case TimeoutError:
        return QStringLiteral("TimeoutError");
    case NetworkError:
        return QStringLiteral("NetworkError");
    case FileIOError:
        return QStringLiteral("FileIOError");
    case ImageProcessingError:
        return QStringLiteral("ImageProcessingError");
    case CalibrationError:
        return QStringLiteral("CalibrationError");
    case HardwareError:
        return QStringLiteral("HardwareError");
    case PermissionDenied:
        return QStringLiteral("PermissionDenied");
    case IncompatibleDevice:
        return QStringLiteral("IncompatibleDevice");
    case CommunicationError:
        return QStringLiteral("CommunicationError");
    case UnsupportedOperation:
        return QStringLiteral("UnsupportedOperation");
    case ConfigurationError:
        return QStringLiteral("ConfigurationError");
    case DataCorruption:
        return QStringLiteral("DataCorruption");
    case LicenseError:
        return QStringLiteral("LicenseError");
    case InitializationError:
        return QStringLiteral("InitializationError");
    default:
        return QStringLiteral("UnknownErrorCode");
    }
}

QString DScannerException::getErrorDescription(ErrorCode code)
{
    switch (code) {
    case NoError:
        return QStringLiteral("没有错误");
    case UnknownError:
        return QStringLiteral("未知错误");
    case DeviceNotFound:
        return QStringLiteral("未找到扫描设备");
    case DeviceNotConnected:
        return QStringLiteral("设备未连接");
    case DeviceBusy:
        return QStringLiteral("设备忙碌，正在被其他应用程序使用");
    case DeviceError:
        return QStringLiteral("设备硬件错误");
    case DriverNotFound:
        return QStringLiteral("未找到设备驱动程序");
    case DriverLoadError:
        return QStringLiteral("驱动程序加载失败");
    case InvalidParameter:
        return QStringLiteral("无效的参数");
    case OperationFailed:
        return QStringLiteral("操作失败");
    case ScanCancelled:
        return QStringLiteral("扫描操作被取消");
    case OutOfMemory:
        return QStringLiteral("内存不足");
    case TimeoutError:
        return QStringLiteral("操作超时");
    case NetworkError:
        return QStringLiteral("网络连接错误");
    case FileIOError:
        return QStringLiteral("文件输入输出错误");
    case ImageProcessingError:
        return QStringLiteral("图像处理错误");
    case CalibrationError:
        return QStringLiteral("设备校准错误");
    case HardwareError:
        return QStringLiteral("硬件错误");
    case PermissionDenied:
        return QStringLiteral("权限不足");
    case IncompatibleDevice:
        return QStringLiteral("不兼容的设备");
    case CommunicationError:
        return QStringLiteral("通信错误");
    case UnsupportedOperation:
        return QStringLiteral("不支持的操作");
    case ConfigurationError:
        return QStringLiteral("配置错误");
    case DataCorruption:
        return QStringLiteral("数据损坏");
    case LicenseError:
        return QStringLiteral("许可证错误");
    case InitializationError:
        return QStringLiteral("初始化错误");
    default:
        return QStringLiteral("未知错误代码");
    }
}

QString DScannerException::getSuggestion(ErrorCode code)
{
    switch (code) {
    case DeviceNotFound:
        return QStringLiteral("请检查扫描仪是否正确连接并开机");
    case DeviceNotConnected:
        return QStringLiteral("请重新连接扫描仪设备");
    case DeviceBusy:
        return QStringLiteral("请关闭其他使用扫描仪的应用程序后重试");
    case DriverNotFound:
        return QStringLiteral("请安装相应的设备驱动程序");
    case DriverLoadError:
        return QStringLiteral("请重新安装设备驱动程序或联系技术支持");
    case InvalidParameter:
        return QStringLiteral("请检查扫描参数设置是否正确");
    case OutOfMemory:
        return QStringLiteral("请降低扫描分辨率或释放系统内存");
    case TimeoutError:
        return QStringLiteral("请检查设备连接或网络状态");
    case NetworkError:
        return QStringLiteral("请检查网络连接和设备IP地址");
    case PermissionDenied:
        return QStringLiteral("请以管理员权限运行程序");
    case IncompatibleDevice:
        return QStringLiteral("此设备不被当前版本支持，请更新软件或联系技术支持");
    case CommunicationError:
        return QStringLiteral("请检查USB线缆或网络连接");
    case UnsupportedOperation:
        return QStringLiteral("此设备不支持当前操作");
    case ConfigurationError:
        return QStringLiteral("请重置设备配置或恢复默认设置");
    case DataCorruption:
        return QStringLiteral("请重新扫描或检查设备状态");
    case InitializationError:
        return QStringLiteral("请重启应用程序或重新连接设备");
    default:
        return QStringLiteral("请联系技术支持获取帮助");
    }
}

bool DScannerException::isRecoverable(ErrorCode code)
{
    switch (code) {
    case DeviceNotConnected:
    case DeviceBusy:
    case InvalidParameter:
    case ScanCancelled:
    case TimeoutError:
    case NetworkError:
    case ConfigurationError:
        return true;
    case DeviceNotFound:
    case DriverNotFound:
    case DriverLoadError:
    case OutOfMemory:
    case HardwareError:
    case PermissionDenied:
    case IncompatibleDevice:
    case DataCorruption:
    case LicenseError:
        return false;
    default:
        return false;
    }
}

QString DScannerException::getRecoveryAction(ErrorCode code)
{
    switch (code) {
    case DeviceNotConnected:
        return QStringLiteral("尝试重新连接设备");
    case DeviceBusy:
        return QStringLiteral("等待设备空闲后重试");
    case InvalidParameter:
        return QStringLiteral("使用默认参数重试");
    case ScanCancelled:
        return QStringLiteral("重新开始扫描");
    case TimeoutError:
        return QStringLiteral("增加超时时间重试");
    case NetworkError:
        return QStringLiteral("检查网络连接后重试");
    case ConfigurationError:
        return QStringLiteral("重置配置后重试");
    default:
        return QStringLiteral("无自动恢复方案");
    }
} 