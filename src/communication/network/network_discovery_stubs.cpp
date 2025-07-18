// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file network_discovery_stubs.cpp
 * @brief 网络发现模块临时存根实现
 * 
 * 提供缺失符号的临时实现，确保核心库可以编译
 */

#include "Scanner/DScannerGlobal.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>

namespace Dtk {
namespace Scanner {

// 前向声明必要的类型
struct NetworkScannerDevice {
    QString name;
    QString address;
    QString protocol;
    QDateTime discoveryTime;
    
    NetworkScannerDevice() : discoveryTime(QDateTime::currentDateTime()) {}
};

// NetworkCompleteDiscovery 存根实现
class DSCANNER_EXPORT NetworkCompleteDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit NetworkCompleteDiscovery(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~NetworkCompleteDiscovery() = default;

    bool startDiscovery() { return true; }
    void stopDiscovery() {}
    bool isDiscovering() const { return false; }
    
    struct Statistics {
        int devicesFound = 0;
        int totalTime = 0;
    };
    
    Statistics getStatistics() const { return Statistics{}; }

Q_SIGNALS:
    void deviceDiscovered(const NetworkScannerDevice &device);
    void discoveryCompleted(const QList<NetworkScannerDevice> &devices);
};

// SANEAPIManager 存根实现
class DSCANNER_EXPORT SANEAPIManager : public QObject
{
    Q_OBJECT

public:
    static SANEAPIManager* instance() {
        static SANEAPIManager s_instance;
        return &s_instance;
    }

    int sane_init_impl(int *version, void (*auth_callback)(const char*, char*, char*)) {
        Q_UNUSED(version)
        Q_UNUSED(auth_callback)
        return 0; // SANE_STATUS_GOOD
    }

    void sane_exit_impl() {}

    int sane_get_devices_impl(const void ***device_list, int local_only) {
        Q_UNUSED(device_list)
        Q_UNUSED(local_only)
        return 0; // SANE_STATUS_GOOD
    }

private:
    SANEAPIManager() = default;
};

// 全局函数存根实现
QString dscannerCore() {
    return "DeepinScan Core v1.0.0";
}

} // namespace Scanner
} // namespace Dtk

#include "network_discovery_stubs.moc" 