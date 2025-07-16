// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERMANAGER_H
#define DSCANNERMANAGER_H

#include "DScannerGlobal.h"
#include "DScannerTypes.h"

#include <QObject>
#include <QList>
#include <QMutex>
#include <QString>
#include <QScopedPointer>
#include <memory>

DSCANNER_BEGIN_NAMESPACE

class DScannerDevice;
class DScannerDriver;
class DScannerManagerPrivate;

/**
 * @brief The DScannerManager class manages scanner devices and drivers
 * 
 * This class is the main entry point for scanner operations. It handles
 * device discovery, driver management, and device lifecycle management.
 * 
 * Example usage:
 * @code
 * auto manager = DScannerManager::instance();
 * 
 * // Discover available devices
 * auto devices = manager->discoverDevices();
 * 
 * // Open a device
 * auto device = manager->openDevice(devices.first().deviceId);
 * if (device) {
 *     // Use the device for scanning
 *     device->startScan(params);
 * }
 * @endcode
 */
class DSCANNER_EXPORT DScannerManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DScannerManager)
    
public:
    /**
     * @brief Get the singleton instance of DScannerManager
     * @return Singleton instance
     */
    static DScannerManager* instance();
    
    /**
     * @brief Destroy the DScannerManager instance
     */
    ~DScannerManager();

    // Device Discovery
    /**
     * @brief Discover available scanner devices
     * @param forceRefresh Force refresh of device list
     * @return List of available devices
     */
    QList<DeviceInfo> discoverDevices(bool forceRefresh = false);
    
    /**
     * @brief Get cached device list
     * @return List of cached devices
     */
    QList<DeviceInfo> availableDevices() const;
    
    /**
     * @brief Check if a device is available
     * @param deviceId Device identifier
     * @return true if device is available, false otherwise
     */
    bool isDeviceAvailable(const QString &deviceId) const;
    
    /**
     * @brief Get device information by ID
     * @param deviceId Device identifier
     * @return Device information, or invalid DeviceInfo if not found
     */
    DeviceInfo deviceInfo(const QString &deviceId) const;

    // Device Management
    /**
     * @brief Open a scanner device
     * @param deviceId Device identifier
     * @return Pointer to opened device, or nullptr if failed
     * @throws DScannerException if device cannot be opened
     */
    DScannerDevice* openDevice(const QString &deviceId);
    
    /**
     * @brief Close a scanner device
     * @param deviceId Device identifier
     */
    void closeDevice(const QString &deviceId);
    
    /**
     * @brief Close all open devices
     */
    void closeAllDevices();
    
    /**
     * @brief Get list of open devices
     * @return List of open device IDs
     */
    QStringList openDevices() const;
    
    /**
     * @brief Check if a device is currently open
     * @param deviceId Device identifier
     * @return true if device is open, false otherwise
     */
    bool isDeviceOpen(const QString &deviceId) const;
    
    /**
     * @brief Get an open device by ID
     * @param deviceId Device identifier
     * @return Pointer to device, or nullptr if not open
     */
    DScannerDevice* device(const QString &deviceId) const;

    // Driver Management
    /**
     * @brief Register a scanner driver
     * @param driver Driver instance
     * @return true if registered successfully, false otherwise
     */
    bool registerDriver(std::shared_ptr<DScannerDriver> driver);
    
    /**
     * @brief Unregister a scanner driver
     * @param driverType Driver type to unregister
     */
    void unregisterDriver(DriverType driverType);
    
    /**
     * @brief Get list of registered drivers
     * @return List of driver types
     */
    QList<DriverType> registeredDrivers() const;
    
    /**
     * @brief Get driver for a specific device
     * @param deviceInfo Device information
     * @return Pointer to driver, or nullptr if no suitable driver found
     */
    std::shared_ptr<DScannerDriver> driverForDevice(const DeviceInfo &deviceInfo) const;

    // Configuration
    /**
     * @brief Set configuration value
     * @param key Configuration key
     * @param value Configuration value
     */
    void setConfiguration(const QString &key, const QVariant &value);
    
    /**
     * @brief Get configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value
     */
    QVariant configuration(const QString &key, const QVariant &defaultValue = QVariant()) const;
    
    /**
     * @brief Save configuration to file
     * @return true if saved successfully, false otherwise
     */
    bool saveConfiguration();
    
    /**
     * @brief Load configuration from file
     * @return true if loaded successfully, false otherwise
     */
    bool loadConfiguration();

    // USB Device Management
    /**
     * @brief Discover USB scanner devices
     * @return List of USB device information
     */
    QList<USBDeviceInfo> discoverUSBDevices();
    
    /**
     * @brief Check if USB device is a scanner
     * @param usbInfo USB device information
     * @return true if device is a scanner, false otherwise
     */
    bool isUSBScanner(const USBDeviceInfo &usbInfo) const;
    
    /**
     * @brief Get device information from USB device
     * @param usbInfo USB device information
     * @return Device information, or invalid DeviceInfo if not a scanner
     */
    DeviceInfo deviceInfoFromUSB(const USBDeviceInfo &usbInfo) const;

    // System Integration
    /**
     * @brief Initialize the scanner manager
     * @return true if initialized successfully, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Shutdown the scanner manager
     */
    void shutdown();
    
    /**
     * @brief Check if manager is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;
    
    /**
     * @brief Get system information
     * @return System information map
     */
    QVariantMap systemInfo() const;

    // Utility Functions
    /**
     * @brief Get version information
     * @return Version string
     */
    static QString version();
    
    /**
     * @brief Get build information
     * @return Build information map
     */
    static QVariantMap buildInfo();
    
    /**
     * @brief Enable debug logging
     * @param enabled Enable or disable debug logging
     */
    void setDebugLogging(bool enabled);
    
    /**
     * @brief Check if debug logging is enabled
     * @return true if enabled, false otherwise
     */
    bool isDebugLoggingEnabled() const;

signals:
    /**
     * @brief Emitted when a device is discovered
     * @param deviceInfo Device information
     */
    void deviceDiscovered(const DeviceInfo &deviceInfo);
    
    /**
     * @brief Emitted when a device is removed
     * @param deviceId Device identifier
     */
    void deviceRemoved(const QString &deviceId);
    
    /**
     * @brief Emitted when a device is opened
     * @param deviceId Device identifier
     */
    void deviceOpened(const QString &deviceId);
    
    /**
     * @brief Emitted when a device is closed
     * @param deviceId Device identifier
     */
    void deviceClosed(const QString &deviceId);
    
    /**
     * @brief Emitted when device list is refreshed
     */
    void deviceListRefreshed();
    
    /**
     * @brief Emitted when a driver is registered
     * @param driverType Driver type
     */
    void driverRegistered(DriverType driverType);
    
    /**
     * @brief Emitted when a driver is unregistered
     * @param driverType Driver type
     */
    void driverUnregistered(DriverType driverType);
    
    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString &error);

protected:
    /**
     * @brief Constructor (singleton pattern)
     * @param parent Parent QObject
     */
    explicit DScannerManager(QObject *parent = nullptr);

private:
    Q_DISABLE_COPY(DScannerManager)
    QScopedPointer<DScannerManagerPrivate> d_ptr;
    
    static DScannerManager* s_instance;
    static QMutex s_instanceMutex;
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERMANAGER_H 