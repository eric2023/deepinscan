// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERDEVICE_H
#define DSCANNERDEVICE_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QVariant>
#include <QScopedPointer>
#include <memory>

#include "DScannerTypes.h"
#include "DScannerGlobal.h"

DSCANNER_BEGIN_NAMESPACE

class DScannerDevicePrivate;

/**
 * @brief The DScannerDevice class provides a unified interface for scanner devices
 * 
 * This class represents a physical scanner device and provides methods to
 * control scanning operations, configure device parameters, and retrieve
 * device information.
 * 
 * Example usage:
 * @code
 * auto device = manager->openDevice("Canon PIXMA MG3600");
 * if (device) {
 *     ScanParameters params;
 *     params.resolution = 300;
 *     params.colorMode = ColorMode::Color;
 *     
 *     connect(device, &DScannerDevice::scanCompleted,
 *             [](const QImage &image) {
 *         image.save("scanned_image.jpg");
 *     });
 *     
 *     device->startScan(params);
 * }
 * @endcode
 */
class DSCANNER_EXPORT DScannerDevice : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DScannerDevice)
    
public:
    /**
     * @brief Device status enumeration
     */
    enum class Status {
        Unknown,        ///< Device status is unknown
        Ready,          ///< Device is ready for scanning
        Busy,           ///< Device is currently scanning
        Error,          ///< Device has encountered an error
        Offline,        ///< Device is offline or disconnected
        Warming         ///< Device is warming up
    };
    Q_ENUM(Status)

    /**
     * @brief Construct a new DScannerDevice object
     * @param deviceInfo Device information
     * @param parent Parent QObject
     */
    explicit DScannerDevice(const DeviceInfo &deviceInfo, QObject *parent = nullptr);
    
    /**
     * @brief Construct a new DScannerDevice object
     * @param parent Parent QObject
     */
    explicit DScannerDevice(QObject *parent = nullptr);
    
    /**
     * @brief Destroy the DScannerDevice object
     */
    virtual ~DScannerDevice();

    // Device Information
    /**
     * @brief Get the device information
     * @return DeviceInfo structure
     */
    DeviceInfo deviceInfo() const;
    
    /**
     * @brief Get the unique device identifier
     * @return Device ID string
     */
    QString deviceId() const;
    
    /**
     * @brief Get the human-readable device name
     * @return Device name string
     */
    QString deviceName() const;
    
    /**
     * @brief Get the device name (alias for deviceName)
     * @return Device name string
     */
    QString name() const;
    
    /**
     * @brief Get the device manufacturer
     * @return Manufacturer name string
     */
    QString manufacturer() const;
    
    /**
     * @brief Get the device model
     * @return Model name string
     */
    QString model() const;
    
    /**
     * @brief Get the device driver type
     * @return Driver type string
     */
    QString driverType() const;
    
    /**
     * @brief Get the current device status
     * @return Current status
     */
    Status status() const;
    
    /**
     * @brief Check if the device is currently connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Check if the device is ready for scanning
     * @return true if ready, false otherwise
     */
    bool isReady() const;

    // Device Capabilities
    /**
     * @brief Get the device capabilities
     * @return ScannerCapabilities structure
     */
    ScannerCapabilities capabilities() const;
    
    /**
     * @brief Get supported resolutions
     * @return List of supported DPI values
     */
    QList<int> supportedResolutions() const;
    
    /**
     * @brief Get supported color modes
     * @return List of supported color modes
     */
    QList<ColorMode> supportedColorModes() const;
    
    /**
     * @brief Get supported image formats
     * @return List of supported image formats
     */
    QList<ImageFormat> supportedFormats() const;
    
    /**
     * @brief Get the maximum scan area
     * @return Maximum scan area in millimeters
     */
    ScanArea maxScanArea() const;

    // Device Control
    /**
     * @brief Open connection to the device
     * @return true if successful, false otherwise
     * @throws DScannerException if device cannot be opened
     */
    bool open();
    
    /**
     * @brief Close connection to the device
     */
    void close();
    
    /**
     * @brief Reset the device to default state
     * @return true if successful, false otherwise
     */
    bool reset();
    
    /**
     * @brief Calibrate the device
     * @return true if successful, false otherwise
     */
    bool calibrate();

    // Scanning Operations
    /**
     * @brief Start scanning with specified parameters
     * @param params Scan parameters
     * @return true if scan started successfully, false otherwise
     * @throws DScannerException if parameters are invalid
     */
    bool startScan(const ScanParameters &params);
    
    /**
     * @brief Stop the current scanning operation
     */
    void stopScan();
    
    /**
     * @brief Pause the current scanning operation
     * @return true if paused successfully, false otherwise
     */
    bool pauseScan();
    
    /**
     * @brief Resume a paused scanning operation
     * @return true if resumed successfully, false otherwise
     */
    bool resumeScan();
    
    /**
     * @brief Get the current scan progress
     * @return Progress percentage (0-100)
     */
    int scanProgress() const;

    // Parameter Management
    /**
     * @brief Set scan parameters
     * @param params Scan parameters to set
     * @return true if parameters were set successfully, false otherwise
     */
    bool setScanParameters(const ScanParameters &params);
    
    /**
     * @brief Get current scan parameters
     * @return Current scan parameters
     */
    ScanParameters scanParameters() const;
    
    /**
     * @brief Set a specific device parameter
     * @param name Parameter name
     * @param value Parameter value
     * @return true if parameter was set successfully, false otherwise
     */
    bool setParameter(const QString &name, const QVariant &value);
    
    /**
     * @brief Get a specific device parameter
     * @param name Parameter name
     * @return Parameter value, or invalid QVariant if not found
     */
    QVariant parameter(const QString &name) const;
    
    /**
     * @brief Get all available parameters
     * @return Map of parameter names to values
     */
    QVariantMap parameters() const;

    // Preview Operations
    /**
     * @brief Start preview scan
     * @return true if preview started successfully, false otherwise
     */
    bool startPreview();
    
    /**
     * @brief Stop preview scan
     */
    void stopPreview();
    
    /**
     * @brief Check if preview is available
     * @return true if preview is supported, false otherwise
     */
    bool isPreviewAvailable() const;

signals:
    /**
     * @brief Emitted when device status changes
     * @param status New device status
     */
    void statusChanged(Status status);
    
    /**
     * @brief Emitted when scan operation starts
     */
    void scanStarted();
    
    /**
     * @brief Emitted during scan operation to report progress
     * @param percentage Progress percentage (0-100)
     */
    void scanProgress(int percentage);
    
    /**
     * @brief Emitted when scan operation completes successfully
     * @param image Scanned image
     */
    void scanCompleted(const QImage &image);
    
    /**
     * @brief Emitted when scan operation is cancelled
     */
    void scanCancelled();
    
    /**
     * @brief Emitted when scan operation is paused
     */
    void scanPaused();
    
    /**
     * @brief Emitted when scan operation is resumed
     */
    void scanResumed();
    
    /**
     * @brief Emitted when preview scan completes
     * @param image Preview image
     */
    void previewCompleted(const QImage &image);
    
    /**
     * @brief Emitted when an error occurs
     * @param error Error description
     */
    void errorOccurred(const QString &error);
    
    /**
     * @brief Emitted when an error occurs with error code
     * @param code Error code
     * @param message Error message
     */
    void errorOccurred(int code, const QString &message);
    
    /**
     * @brief Emitted when scan is stopped
     */
    void scanStopped();
    
    /**
     * @brief Emitted when device is connected
     */
    void connected();
    
    /**
     * @brief Emitted when device is disconnected
     */
    void disconnected();

protected:
    /**
     * @brief Constructor for derived classes
     * @param dd Private implementation
     * @param parent Parent QObject
     */
    explicit DScannerDevice(DScannerDevicePrivate &dd, QObject *parent = nullptr);

private:
    Q_DISABLE_COPY(DScannerDevice)
    QScopedPointer<DScannerDevicePrivate> d_ptr;
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERDEVICE_H 