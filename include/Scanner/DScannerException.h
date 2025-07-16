// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNEREXCEPTION_H
#define DSCANNEREXCEPTION_H

#include <exception>
#include <QString>
#include <QException>

#include "DScannerGlobal.h"

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief The DScannerException class provides exception handling for scanner operations
 * 
 * This class extends QException to provide Qt-compatible exception handling
 * for scanner-related errors. It includes error codes and detailed error messages.
 */
class DSCANNER_EXPORT DScannerException : public QException
{
public:
    /**
     * @brief Error code enumeration
     */
    enum class ErrorCode {
        Unknown = 0,                    ///< Unknown error
        DeviceNotFound,                 ///< Device not found
        DeviceNotAvailable,             ///< Device not available
        DeviceAlreadyOpen,              ///< Device already open
        DeviceNotOpen,                  ///< Device not open
        DeviceNotReady,                 ///< Device not ready
        DeviceError,                    ///< Device error
        DriverNotFound,                 ///< Driver not found
        DriverError,                    ///< Driver error
        CommunicationError,             ///< Communication error
        InvalidParameter,               ///< Invalid parameter
        InvalidScanArea,                ///< Invalid scan area
        InvalidResolution,              ///< Invalid resolution
        ScanInProgress,                 ///< Scan already in progress
        ScanNotInProgress,              ///< Scan not in progress
        ScanCancelled,                  ///< Scan was cancelled
        ScanFailed,                     ///< Scan failed
        ImageProcessingError,           ///< Image processing error
        FileIOError,                    ///< File I/O error
        OutOfMemory,                    ///< Out of memory
        Timeout,                        ///< Operation timeout
        PermissionDenied,               ///< Permission denied
        ConfigurationError,             ///< Configuration error
        CalibrationError,               ///< Calibration error
        NotSupported,                   ///< Operation not supported
        NotImplemented                  ///< Operation not implemented
    };

    /**
     * @brief Construct a new DScannerException object
     * @param code Error code
     * @param message Error message
     */
    explicit DScannerException(ErrorCode code, const QString &message = QString());
    
    /**
     * @brief Construct a new DScannerException object
     * @param message Error message
     */
    explicit DScannerException(const QString &message);
    
    /**
     * @brief Copy constructor
     * @param other Other exception
     */
    DScannerException(const DScannerException &other);
    
    /**
     * @brief Assignment operator
     * @param other Other exception
     * @return Reference to this exception
     */
    DScannerException& operator=(const DScannerException &other);
    
    /**
     * @brief Destroy the DScannerException object
     */
    virtual ~DScannerException();

    /**
     * @brief Get the error code
     * @return Error code
     */
    ErrorCode errorCode() const;
    
    /**
     * @brief Get the error message
     * @return Error message
     */
    QString errorMessage() const;
    
    /**
     * @brief Get the error message as C string
     * @return Error message as C string
     */
    const char* what() const noexcept override;
    
    /**
     * @brief Clone the exception
     * @return Cloned exception
     */
    DScannerException* clone() const override;
    
    /**
     * @brief Raise the exception
     */
    void raise() const override;

    /**
     * @brief Get error code name
     * @param code Error code
     * @return Error code name
     */
    static QString errorCodeName(ErrorCode code);
    
    /**
     * @brief Get default error message for error code
     * @param code Error code
     * @return Default error message
     */
    static QString defaultErrorMessage(ErrorCode code);

private:
    ErrorCode m_errorCode;
    QString m_errorMessage;
    mutable QByteArray m_whatBuffer;
};

DSCANNER_END_NAMESPACE

#endif // DSCANNEREXCEPTION_H 