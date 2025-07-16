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
    case ErrorCode::DeviceAlreadyOpen:
        return QStringLiteral("DeviceAlreadyOpen");
    case ErrorCode::DeviceNotOpen:
        return QStringLiteral("DeviceNotOpen");
    case ErrorCode::DeviceNotReady:
        return QStringLiteral("DeviceNotReady");
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
    case ErrorCode::PermissionDenied:
        return QStringLiteral("PermissionDenied");
    case ErrorCode::ConfigurationError:
        return QStringLiteral("ConfigurationError");
    case ErrorCode::CalibrationError:
        return QStringLiteral("CalibrationError");
    case ErrorCode::NotSupported:
        return QStringLiteral("NotSupported");
    case ErrorCode::NotImplemented:
        return QStringLiteral("NotImplemented");
    default:
        return QStringLiteral("Unknown");
    }
}

QString DScannerException::defaultErrorMessage(ErrorCode code)
{
    switch (code) {
    case ErrorCode::Unknown:
        return QStringLiteral("Unknown error occurred");
    case ErrorCode::DeviceNotFound:
        return QStringLiteral("Scanner device not found");
    case ErrorCode::DeviceNotAvailable:
        return QStringLiteral("Scanner device not available");
    case ErrorCode::DeviceAlreadyOpen:
        return QStringLiteral("Scanner device already open");
    case ErrorCode::DeviceNotOpen:
        return QStringLiteral("Scanner device not open");
    case ErrorCode::DeviceNotReady:
        return QStringLiteral("Scanner device not ready");
    case ErrorCode::DeviceError:
        return QStringLiteral("Scanner device error");
    case ErrorCode::DriverNotFound:
        return QStringLiteral("Scanner driver not found");
    case ErrorCode::DriverError:
        return QStringLiteral("Scanner driver error");
    case ErrorCode::CommunicationError:
        return QStringLiteral("Communication error with scanner");
    case ErrorCode::InvalidParameter:
        return QStringLiteral("Invalid parameter");
    case ErrorCode::InvalidScanArea:
        return QStringLiteral("Invalid scan area");
    case ErrorCode::InvalidResolution:
        return QStringLiteral("Invalid resolution");
    case ErrorCode::ScanInProgress:
        return QStringLiteral("Scan already in progress");
    case ErrorCode::ScanNotInProgress:
        return QStringLiteral("Scan not in progress");
    case ErrorCode::ScanCancelled:
        return QStringLiteral("Scan was cancelled");
    case ErrorCode::ScanFailed:
        return QStringLiteral("Scan failed");
    case ErrorCode::ImageProcessingError:
        return QStringLiteral("Image processing error");
    case ErrorCode::FileIOError:
        return QStringLiteral("File I/O error");
    case ErrorCode::OutOfMemory:
        return QStringLiteral("Out of memory");
    case ErrorCode::Timeout:
        return QStringLiteral("Operation timeout");
    case ErrorCode::PermissionDenied:
        return QStringLiteral("Permission denied");
    case ErrorCode::ConfigurationError:
        return QStringLiteral("Configuration error");
    case ErrorCode::CalibrationError:
        return QStringLiteral("Calibration error");
    case ErrorCode::NotSupported:
        return QStringLiteral("Operation not supported");
    case ErrorCode::NotImplemented:
        return QStringLiteral("Operation not implemented");
    default:
        return QStringLiteral("Unknown error occurred");
    }
} 