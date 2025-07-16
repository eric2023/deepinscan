// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERDRIVER_H
#define DSCANNERDRIVER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QList>
#include <memory>

#include "DScannerTypes.h"
#include "DScannerGlobal.h"

DSCANNER_BEGIN_NAMESPACE

class DScannerDriverPrivate;

/**
 * @brief The DScannerDriver class provides an abstract interface for scanner drivers
 * 
 * This class defines the interface that all scanner drivers must implement.
 * It provides methods for device detection, initialization, scanning, and
 * parameter management.
 * 
 * Example implementation:
 * @code
 * class MyDriver : public DScannerDriver
 * {
 * public:
 *     bool detectDevice(const USBDeviceInfo &info) override {
 *         return info.vendorId == 0x04A9; // Canon
 *     }
 *     
 *     bool openDevice(const QString &deviceName) override {
 *         // Open device connection
 *         return true;
 *     }
 *     
 *     QImage performScan(const ScanParameters &params) override {
 *         // Perform actual scanning
 *         return QImage();
 *     }
 * };
 * @endcode
 */
class DSCANNER_EXPORT DScannerDriver : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DScannerDriver)
    
public:
    /**
     * @brief Construct a new DScannerDriver object
     * @param parent Parent QObject
     */
    explicit DScannerDriver(QObject *parent = nullptr);
    
    /**
     * @brief Destroy the DScannerDriver object
     */
    virtual ~DScannerDriver();

    // Driver Information
    /**
     * @brief Get the driver name
     * @return Driver name string
     */
    virtual QString driverName() const = 0;
    
    /**
     * @brief Get the driver version
     * @return Driver version string
     */
    virtual QString driverVersion() const = 0;
    
    /**
     * @brief Get the driver type
     * @return Driver type enumeration
     */
    virtual DriverType driverType() const = 0;
    
    /**
     * @brief Get the supported manufacturer names
     * @return List of supported manufacturers
     */
    virtual QStringList supportedManufacturers() const = 0;
    
    /**
     * @brief Get the supported device models
     * @return List of supported device models
     */
    virtual QStringList supportedModels() const = 0;

    // Device Detection
    /**
     * @brief Detect if the driver supports a USB device
     * @param usbInfo USB device information
     * @return true if device is supported, false otherwise
     */
    virtual bool detectDevice(const USBDeviceInfo &usbInfo) const = 0;
    
    /**
     * @brief Detect if the driver supports a device by name
     * @param deviceName Device name
     * @return true if device is supported, false otherwise
     */
    virtual bool detectDevice(const QString &deviceName) const;
    
    /**
     * @brief Get device information from USB device
     * @param usbInfo USB device information
     * @return Device information, or invalid DeviceInfo if not supported
     */
    virtual DeviceInfo deviceInfoFromUSB(const USBDeviceInfo &usbInfo) const = 0;
    
    /**
     * @brief Discover devices supported by this driver
     * @return List of discovered devices
     */
    virtual QList<DeviceInfo> discoverDevices() const = 0;

    // Device Management
    /**
     * @brief Open a device
     * @param deviceName Device name or identifier
     * @return true if device opened successfully, false otherwise
     */
    virtual bool openDevice(const QString &deviceName) = 0;
    
    /**
     * @brief Close the current device
     */
    virtual void closeDevice() = 0;
    
    /**
     * @brief Check if a device is currently open
     * @return true if device is open, false otherwise
     */
    virtual bool isDeviceOpen() const = 0;
    
    /**
     * @brief Get the current device name
     * @return Device name, or empty string if no device is open
     */
    virtual QString currentDeviceName() const = 0;
    
    /**
     * @brief Reset the current device
     * @return true if reset successful, false otherwise
     */
    virtual bool resetDevice() = 0;

    // Device Capabilities
    /**
     * @brief Get device capabilities
     * @return Scanner capabilities structure
     */
    virtual ScannerCapabilities getCapabilities() const = 0;
    
    /**
     * @brief Get device status
     * @return Scanner status structure
     */
    virtual ScannerStatus getStatus() const = 0;
    
    /**
     * @brief Check if device is ready for scanning
     * @return true if ready, false otherwise
     */
    virtual bool isReady() const = 0;

    // Scanning Operations
    /**
     * @brief Set scan parameters
     * @param params Scan parameters
     * @return true if parameters set successfully, false otherwise
     */
    virtual bool setScanParameters(const ScanParameters &params) = 0;
    
    /**
     * @brief Get current scan parameters
     * @return Current scan parameters
     */
    virtual ScanParameters getScanParameters() const = 0;
    
    /**
     * @brief Start scanning operation
     * @return true if scan started successfully, false otherwise
     */
    virtual bool startScan() = 0;
    
    /**
     * @brief Stop scanning operation
     */
    virtual void stopScan() = 0;
    
    /**
     * @brief Get scan progress
     * @return Progress percentage (0-100)
     */
    virtual int getScanProgress() const = 0;
    
    /**
     * @brief Check if scanning is in progress
     * @return true if scanning, false otherwise
     */
    virtual bool isScanning() const = 0;
    
    /**
     * @brief Get scanned image data
     * @return Scanned image, or null image if no data available
     */
    virtual QImage getScanData() = 0;

    // Preview Operations
    /**
     * @brief Start preview scan
     * @return true if preview started successfully, false otherwise
     */
    virtual bool startPreview();
    
    /**
     * @brief Stop preview scan
     */
    virtual void stopPreview();
    
    /**
     * @brief Get preview image
     * @return Preview image, or null image if no preview available
     */
    virtual QImage getPreviewData();
    
    /**
     * @brief Check if preview is available
     * @return true if preview is supported, false otherwise
     */
    virtual bool isPreviewSupported() const;

    // Calibration
    /**
     * @brief Calibrate the device
     * @return true if calibration successful, false otherwise
     */
    virtual bool calibrateDevice();
    
    /**
     * @brief Check if calibration is supported
     * @return true if calibration is supported, false otherwise
     */
    virtual bool isCalibrationSupported() const;
    
    /**
     * @brief Get calibration status
     * @return true if device is calibrated, false otherwise
     */
    virtual bool isCalibrated() const;

    // Parameter Management
    /**
     * @brief Set a device parameter
     * @param name Parameter name
     * @param value Parameter value
     * @return true if parameter set successfully, false otherwise
     */
    virtual bool setParameter(const QString &name, const QVariant &value);
    
    /**
     * @brief Get a device parameter
     * @param name Parameter name
     * @return Parameter value, or invalid QVariant if not found
     */
    virtual QVariant getParameter(const QString &name) const;
    
    /**
     * @brief Get all available parameters
     * @return Map of parameter names to values
     */
    virtual QVariantMap getParameters() const;
    
    /**
     * @brief Get parameter constraints
     * @param name Parameter name
     * @return Parameter constraints (min, max, step, etc.)
     */
    virtual QVariantMap getParameterConstraints(const QString &name) const;

    // Error Handling
    /**
     * @brief Get the last error message
     * @return Error message string
     */
    QString lastError() const;
    
    /**
     * @brief Clear the last error
     */
    void clearError();

signals:
    /**
     * @brief Emitted when scan progress changes
     * @param progress Progress percentage (0-100)
     */
    void scanProgress(int progress);
    
    /**
     * @brief Emitted when scan completes
     * @param image Scanned image
     */
    void scanCompleted(const QImage &image);
    
    /**
     * @brief Emitted when scan is cancelled
     */
    void scanCancelled();
    
    /**
     * @brief Emitted when preview completes
     * @param image Preview image
     */
    void previewCompleted(const QImage &image);
    
    /**
     * @brief Emitted when device status changes
     * @param status New device status
     */
    void statusChanged(const ScannerStatus &status);
    
    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString &error);

protected:
    /**
     * @brief Constructor for derived classes
     * @param dd Private implementation
     * @param parent Parent QObject
     */
    explicit DScannerDriver(DScannerDriverPrivate &dd, QObject *parent = nullptr);
    
    /**
     * @brief Set the last error message
     * @param error Error message
     */
    void setLastError(const QString &error);

private:
    Q_DISABLE_COPY(DScannerDriver)
    QScopedPointer<DScannerDriverPrivate> d_ptr;
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERDRIVER_H 