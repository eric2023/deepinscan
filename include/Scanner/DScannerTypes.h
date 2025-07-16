// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERTYPES_H
#define DSCANNERTYPES_H

#include "DScannerGlobal.h"

#include <QObject>
#include <QVariant>
#include <QList>
#include <QImage>
#include <QDateTime>
#include <QMutex>

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief Color mode enumeration
 */
enum class ColorMode {
    Lineart,        ///< Black and white line art
    Monochrome,     ///< Monochrome (1-bit)
    Grayscale,      ///< Grayscale (8-bit)
    Color           ///< Full color (24-bit RGB)
};

/**
 * @brief Image format enumeration
 */
enum class ImageFormat {
    BMP,            ///< Windows Bitmap
    JPEG,           ///< JPEG format
    PNG,            ///< PNG format
    TIFF,           ///< TIFF format
    PDF,            ///< PDF format
    PNM             ///< Portable anymap format
};

/**
 * @brief Communication protocol enumeration
 */
enum class CommunicationProtocol {
    USB,            ///< USB communication
    SCSI,           ///< SCSI communication
    Network,        ///< Network communication
    Serial,         ///< Serial communication
    Parallel        ///< Parallel communication
};

/**
 * @brief Scanner driver type enumeration
 */
enum class DriverType {
    SANE,           ///< SANE backend driver
    Genesys,        ///< Genesys chipset driver
    Canon,          ///< Canon proprietary driver
    Epson,          ///< Epson proprietary driver
    HP,             ///< HP proprietary driver
    Brother,        ///< Brother proprietary driver
    Fujitsu,        ///< Fujitsu proprietary driver
    Generic         ///< Generic USB driver
};

/**
 * @brief Scan area structure (in millimeters)
 */
struct ScanArea {
    double x = 0.0;         ///< X coordinate
    double y = 0.0;         ///< Y coordinate
    double width = 0.0;     ///< Width
    double height = 0.0;    ///< Height
    
    ScanArea() = default;
    ScanArea(double x, double y, double width, double height)
        : x(x), y(y), width(width), height(height) {}
    
    bool isValid() const {
        return width > 0 && height > 0;
    }
    
    QRectF toRectF() const {
        return QRectF(x, y, width, height);
    }
};

/**
 * @brief Scan parameters structure
 */
struct ScanParameters {
    int resolution = 300;                   ///< Resolution in DPI
    ColorMode colorMode = ColorMode::Color; ///< Color mode
    ImageFormat format = ImageFormat::JPEG; ///< Output format
    ScanArea area;                          ///< Scan area
    int quality = 85;                       ///< JPEG quality (0-100)
    bool autoRotate = false;                ///< Auto-rotate scanned image
    bool autoCrop = false;                  ///< Auto-crop scanned image
    bool denoiseEnabled = false;            ///< Enable denoising
    bool sharpenEnabled = false;            ///< Enable sharpening
    double gamma = 1.0;                     ///< Gamma correction
    double brightness = 0.0;                ///< Brightness adjustment (-100 to 100)
    double contrast = 0.0;                  ///< Contrast adjustment (-100 to 100)
    
    ScanParameters() = default;
    
    bool isValid() const {
        return resolution > 0 && resolution <= 9600 &&
               quality >= 0 && quality <= 100 &&
               gamma > 0.0 && gamma <= 3.0 &&
               brightness >= -100.0 && brightness <= 100.0 &&
               contrast >= -100.0 && contrast <= 100.0;
    }
};

/**
 * @brief Device information structure
 */
struct DeviceInfo {
    QString deviceId;                       ///< Unique device identifier
    QString name;                           ///< Human-readable device name
    QString manufacturer;                   ///< Manufacturer name
    QString model;                          ///< Model name
    QString serialNumber;                   ///< Serial number
    DriverType driverType;                  ///< Driver type
    CommunicationProtocol protocol;         ///< Communication protocol
    QString connectionString;               ///< Connection string (USB path, IP, etc.)
    bool isAvailable = false;               ///< Device availability status
    
    DeviceInfo() = default;
    
    bool isValid() const {
        return !deviceId.isEmpty() && !name.isEmpty();
    }
};

/**
 * @brief USB device information structure
 */
struct USBDeviceInfo {
    uint16_t vendorId = 0;                  ///< USB vendor ID
    uint16_t productId = 0;                 ///< USB product ID
    QString devicePath;                     ///< USB device path
    QString serialNumber;                   ///< Serial number
    QString manufacturer;                   ///< Manufacturer name
    QString product;                        ///< Product name
    uint8_t deviceClass = 0;                ///< Device class
    uint8_t deviceSubClass = 0;             ///< Device subclass
    uint8_t deviceProtocol = 0;             ///< Device protocol
    uint8_t busNumber = 0;                  ///< Bus number
    uint8_t deviceAddress = 0;              ///< Device address
    int interfaceNumber = 0;                ///< Interface number
    int configurationValue = 1;             ///< Configuration value
    
    USBDeviceInfo() = default;
    USBDeviceInfo(uint16_t vid, uint16_t pid, const QString &path)
        : vendorId(vid), productId(pid), devicePath(path) {}
    
    bool isValid() const {
        return vendorId != 0 && productId != 0;
    }
};

/**
 * @brief Scanner capabilities structure
 */
struct ScannerCapabilities {
    QList<int> supportedResolutions;        ///< Supported resolutions (DPI)
    QList<ColorMode> supportedColorModes;   ///< Supported color modes
    QList<ImageFormat> supportedFormats;    ///< Supported image formats
    ScanArea maxScanArea;                   ///< Maximum scan area
    ScanArea minScanArea;                   ///< Minimum scan area
    bool hasADF = false;                    ///< Has automatic document feeder
    bool hasDuplex = false;                 ///< Has duplex scanning capability
    bool hasPreview = false;                ///< Has preview capability
    bool hasCalibration = false;            ///< Has calibration capability
    bool hasLamp = false;                   ///< Has lamp control
    int maxBatchSize = 1;                   ///< Maximum batch size
    
    ScannerCapabilities() = default;
    
    bool isValid() const {
        return !supportedResolutions.isEmpty() && 
               !supportedColorModes.isEmpty() &&
               maxScanArea.isValid();
    }
};

/**
 * @brief Scanner status structure
 */
struct ScannerStatus {
    bool isReady = false;                   ///< Device is ready
    bool isScanning = false;                ///< Currently scanning
    bool hasError = false;                  ///< Has error
    bool isWarming = false;                 ///< Lamp is warming up
    bool isCalibrating = false;             ///< Currently calibrating
    bool hasDocument = false;               ///< Document is present
    bool adfEmpty = true;                   ///< ADF is empty
    bool coverOpen = false;                 ///< Cover is open
    QString errorMessage;                   ///< Error message if any
    int temperature = 0;                    ///< Device temperature
    int lampHours = 0;                      ///< Lamp usage hours
    
    ScannerStatus() = default;
    
    bool isOperational() const {
        return isReady && !hasError && !coverOpen;
    }
};

/**
 * @brief Image processing options structure
 */
struct ImageProcessingOptions {
    bool autoRotate = false;                ///< Auto-rotate image
    bool autoCrop = false;                  ///< Auto-crop image
    bool deskew = false;                    ///< Deskew image
    bool despeckle = false;                 ///< Remove speckles
    bool denoise = false;                   ///< Denoise image
    bool sharpen = false;                   ///< Sharpen image
    double gamma = 1.0;                     ///< Gamma correction
    double brightness = 0.0;                ///< Brightness adjustment
    double contrast = 0.0;                  ///< Contrast adjustment
    double saturation = 0.0;                ///< Saturation adjustment
    bool colorCorrection = false;           ///< Enable color correction
    
    ImageProcessingOptions() = default;
    
    bool isValid() const {
        return gamma > 0.0 && gamma <= 3.0 &&
               brightness >= -100.0 && brightness <= 100.0 &&
               contrast >= -100.0 && contrast <= 100.0 &&
               saturation >= -100.0 && saturation <= 100.0;
    }
};

/**
 * @brief Scan job structure
 */
struct ScanJob {
    QString jobId;                          ///< Unique job identifier
    QString deviceId;                       ///< Target device ID
    ScanParameters parameters;              ///< Scan parameters
    ImageProcessingOptions processing;      ///< Image processing options
    QString outputPath;                     ///< Output file path
    QDateTime createdTime;                  ///< Job creation time
    QDateTime startTime;                    ///< Job start time
    QDateTime endTime;                      ///< Job end time
    int progress = 0;                       ///< Job progress (0-100)
    QString status;                         ///< Job status
    QString errorMessage;                   ///< Error message if any
    
    ScanJob() = default;
    
    bool isValid() const {
        return !jobId.isEmpty() && !deviceId.isEmpty() && 
               parameters.isValid();
    }
};

DSCANNER_END_NAMESPACE

// Register meta types for Qt's meta-object system
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ColorMode)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ImageFormat)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::CommunicationProtocol)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::DriverType)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ScanArea)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ScanParameters)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::DeviceInfo)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::USBDeviceInfo)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ScannerCapabilities)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ScannerStatus)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ImageProcessingOptions)
Q_DECLARE_METATYPE(DSCANNER_NAMESPACE::ScanJob)

// Hash function for DriverType enum to support QHash
inline uint qHash(DSCANNER_NAMESPACE::DriverType key, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(static_cast<int>(key), seed);
}

// Also provide a version without the seed parameter for older Qt versions
inline uint qHash(DSCANNER_NAMESPACE::DriverType key) Q_DECL_NOTHROW
{
    return qHash(static_cast<int>(key));
}

#endif // DSCANNERTYPES_H 