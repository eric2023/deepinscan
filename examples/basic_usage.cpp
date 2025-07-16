// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QDebug>
#include <QTimer>

#include <Scanner/DScannerManager.h>
#include <Scanner/DScannerDevice.h>
#include <Scanner/DScannerException.h>

using namespace Dtk::Scanner;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "DeepinScan Basic Usage Example";
    qDebug() << "Version:" << DScannerManager::version();
    
    try {
        // Get scanner manager instance
        auto manager = DScannerManager::instance();
        
        // Initialize the manager
        if (!manager->initialize()) {
            qCritical() << "Failed to initialize scanner manager";
            return -1;
        }
        
        qDebug() << "Scanner manager initialized successfully";
        
        // Discover available devices
        qDebug() << "Discovering scanner devices...";
        auto devices = manager->discoverDevices();
        
        qDebug() << "Found" << devices.size() << "scanner devices:";
        for (const auto &device : devices) {
            qDebug() << "  -" << device.name 
                     << "(" << device.manufacturer << device.model << ")"
                     << "Available:" << device.isAvailable;
        }
        
        if (devices.isEmpty()) {
            qWarning() << "No scanner devices found";
            qDebug() << "This is normal if no physical scanners are connected";
            qDebug() << "The framework is ready to use when devices are available";
        } else {
            // Try to open the first available device
            auto firstDevice = devices.first();
            qDebug() << "Attempting to open device:" << firstDevice.name;
            
            auto device = manager->openDevice(firstDevice.deviceId);
            if (device) {
                qDebug() << "Device opened successfully";
                qDebug() << "Device status:" << static_cast<int>(device->status());
                qDebug() << "Device ready:" << device->isReady();
                
                // Get device capabilities
                auto capabilities = device->capabilities();
                qDebug() << "Device capabilities:";
                qDebug() << "  Supported resolutions:" << capabilities.supportedResolutions;
                qDebug() << "  Max scan area:" << capabilities.maxScanArea.width 
                         << "x" << capabilities.maxScanArea.height << "mm";
                qDebug() << "  Has ADF:" << capabilities.hasADF;
                qDebug() << "  Has duplex:" << capabilities.hasDuplex;
                
                // Configure scan parameters
                ScanParameters params;
                params.resolution = 300;
                params.colorMode = ColorMode::Color;
                params.format = ImageFormat::JPEG;
                params.quality = 85;
                
                qDebug() << "Setting scan parameters...";
                if (device->setScanParameters(params)) {
                    qDebug() << "Scan parameters set successfully";
                    
                    // Note: We won't actually start scanning in this example
                    // as it requires a physical device
                    qDebug() << "Ready to scan (not starting in this example)";
                } else {
                    qWarning() << "Failed to set scan parameters";
                }
                
                // Close the device
                device->close();
                qDebug() << "Device closed";
            } else {
                qWarning() << "Failed to open device";
            }
        }
        
        // Shutdown the manager
        manager->shutdown();
        qDebug() << "Scanner manager shutdown complete";
        
    } catch (const DScannerException &e) {
        qCritical() << "Scanner exception:" << e.errorMessage();
        qCritical() << "Error code:" << DScannerException::errorCodeName(e.errorCode());
        return -1;
    } catch (const std::exception &e) {
        qCritical() << "Standard exception:" << e.what();
        return -1;
    }
    
    qDebug() << "Example completed successfully";
    
    // Exit immediately instead of running event loop
    QTimer::singleShot(0, &app, &QApplication::quit);
    
    return app.exec();
} 