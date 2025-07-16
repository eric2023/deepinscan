// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEVICELISTWIDGET_H
#define DEVICELISTWIDGET_H

#include <DWidget>
#include <DListWidget>
#include <DPushButton>
#include <DLabel>
#include <DSpinner>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QListWidgetItem>

#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerTypes.h"

DWIDGET_USE_NAMESPACE
using namespace Dtk::Scanner;

/**
 * @brief 设备列表管理组件
 * 
 * 提供扫描设备的列表显示、选择和管理功能
 * 支持USB设备和网络设备的自动发现和状态监控
 */
class DeviceListWidget : public DWidget
{
    Q_OBJECT

public:
    explicit DeviceListWidget(QWidget *parent = nullptr);
    ~DeviceListWidget();

    /**
     * @brief 设置扫描仪管理器
     * @param manager 扫描仪管理器实例
     */
    void setScannerManager(DScannerManager *manager);

    /**
     * @brief 获取当前选中的设备
     * @return 当前选中的设备信息，如果没有选中则返回无效的DeviceInfo
     */
    DeviceInfo currentSelectedDevice() const;

    /**
     * @brief 刷新设备列表
     */
    void refreshDeviceList();

    /**
     * @brief 清空设备列表
     */
    void clearDeviceList();

    /**
     * @brief 设置自动刷新间隔
     * @param interval 自动刷新间隔（秒），0表示禁用自动刷新
     */
    void setAutoRefreshInterval(int interval);

signals:
    /**
     * @brief 设备被选中时发出此信号
     * @param device 被选中的设备信息
     */
    void deviceSelected(const DeviceInfo &device);

    /**
     * @brief 请求刷新设备列表时发出此信号
     */
    void deviceRefreshRequested();

    /**
     * @brief 设备连接状态改变时发出此信号
     * @param deviceId 设备ID
     * @param connected 是否已连接
     */
    void deviceConnectionChanged(const QString &deviceId, bool connected);

private slots:
    void onDeviceItemClicked(QListWidgetItem *item);
    void onDeviceItemDoubleClicked(QListWidgetItem *item);
    void onRefreshButtonClicked();
    void onAutoRefreshTimeout();
    void onDeviceDiscovered();
    void onDeviceStatusChanged();

private:
    void setupUI();
    void setupConnections();
    void updateDeviceList();
    void addDeviceItem(const DeviceInfo &device);
    void updateDeviceItem(const DeviceInfo &device);
    void removeDeviceItem(const QString &deviceId);
    
    QListWidgetItem *findDeviceItem(const QString &deviceId);
    void setDeviceItemData(QListWidgetItem *item, const DeviceInfo &device);
    DeviceInfo getDeviceItemData(QListWidgetItem *item) const;
    
    void showLoadingState(bool loading);
    void updateStatusText();

private:
    // 界面组件
    DListWidget *m_deviceListWidget;
    DPushButton *m_refreshButton;
    DLabel *m_statusLabel;
    DSpinner *m_loadingSpinner;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;

    // 核心组件
    DScannerManager *m_scannerManager;

    // 状态管理
    QList<DeviceInfo> m_deviceList;
    QString m_selectedDeviceId;
    bool m_isRefreshing;
    
    // 自动刷新
    QTimer *m_autoRefreshTimer;
    int m_autoRefreshInterval;

    // 设备状态图标
    QIcon m_connectedIcon;
    QIcon m_disconnectedIcon;
    QIcon m_busyIcon;
    QIcon m_errorIcon;
    QIcon m_unknownIcon;
};

/**
 * @brief 自定义设备列表项
 * 
 * 用于在设备列表中显示设备信息的自定义列表项
 */
class DeviceListItem : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceListItem(const DeviceInfo &device, QWidget *parent = nullptr);

    /**
     * @brief 更新设备信息
     * @param device 新的设备信息
     */
    void updateDevice(const DeviceInfo &device);

    /**
     * @brief 获取设备信息
     * @return 当前设备信息
     */
    DeviceInfo deviceInfo() const { return m_device; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void clicked();
    void doubleClicked();

private:
    void setupUI();
    void updateStatusIcon();
    void updateTooltip();

private:
    DeviceInfo m_device;
    
    // 界面组件
    QHBoxLayout *m_layout;
    DLabel *m_iconLabel;
    DLabel *m_nameLabel;
    DLabel *m_statusLabel;
    DLabel *m_typeLabel;
    
    // 状态
    bool m_hovered;
    bool m_pressed;
};

#endif // DEVICELISTWIDGET_H 