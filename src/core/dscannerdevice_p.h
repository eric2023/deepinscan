// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERDEVICE_P_H
#define DSCANNERDEVICE_P_H

#include "Scanner/DScannerDevice.h"
#include "Scanner/DScannerException.h"
#include "Scanner/DScannerTypes.h"

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QDateTime>

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief The DScannerDevicePrivate class provides private implementation for DScannerDevice
 * 
 * This class contains the internal implementation details of DScannerDevice,
 * including device state management, communication handling, and scan operations.
 */
class DScannerDevicePrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(DScannerDevice)
    
public:
    explicit DScannerDevicePrivate(DScannerDevice *q);
    virtual ~DScannerDevicePrivate();
    
    // Q_DECLARE_PUBLIC requires q_ptr to be declared first
    DScannerDevice *q_ptr;
    
    // Device information and status
    DeviceInfo deviceInfo;
    DScannerDevice::Status status;
    ScannerCapabilities capabilities;
    ScanParameters currentScanParams;
    ImageProcessingOptions processingOptions;
    DriverType driverType;
    
    // Connection and communication
    bool isConnected;
    bool isOpen;
    QString lastError;
    
    // Scan state
    bool isScanInProgress;
    bool isPreviewInProgress;
    bool isPaused;
    int scanProgress;
    QDateTime scanStartTime;
    QDateTime scanEndTime;
    
    // Device handle and driver-specific data
    void *deviceHandle;
    void *driverData;
    
    // Thread safety
    mutable QMutex mutex;
    QWaitCondition waitCondition;
    
    // Timers
    QTimer *statusTimer;
    QTimer *progressTimer;
    
    // Methods
    bool initializeDevice();
    void cleanup();
    bool validateScanParameters(const ScanParameters &params);
    void updateStatus(DScannerDevice::Status newStatus);
    void updateProgress(int percentage);
    void emitError(const QString &error);
    void emitError(int code, const QString &message);
    
    // Device operations
    virtual bool openDevice();
    virtual void closeDevice();
    virtual bool resetDevice();
    virtual bool calibrateDevice();
    virtual bool startScanOperation(const ScanParameters &params);
    virtual void stopScanOperation();
    virtual bool pauseScanOperation();
    virtual bool resumeScanOperation();
    virtual bool startPreviewOperation();
    virtual void stopPreviewOperation();
    virtual bool setDeviceParameter(const QString &name, const QVariant &value);
    virtual QVariant getDeviceParameter(const QString &name) const;
    virtual ScannerCapabilities queryCapabilities();
    
private slots:
    void onStatusTimer();
    void onProgressTimer();
};

DSCANNER_END_NAMESPACE

#endif // DSCANNERDEVICE_P_H 