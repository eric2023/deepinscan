// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicelistwidget.h"
#include <DApplication>
#include <DMessageBox>
#include <DDialog>
#include <DFrame>
#include <DFontSizeManager>
#include <QGridLayout>
#include <QIcon>
#include <QDebug>

DWIDGET_USE_NAMESPACE

DeviceListWidget::DeviceListWidget(QWidget *parent)
    : DWidget(parent)
    , m_scannerManager(nullptr)
    , m_deviceList(nullptr)
    , m_refreshButton(nullptr)
    , m_connectButton(nullptr)
    , m_disconnectButton(nullptr)
    , m_propertiesButton(nullptr)
    , m_statusLabel(nullptr)
    , m_spinner(nullptr)
    , m_refreshTimer(nullptr)
{
    qDebug() << "DeviceListWidget: 初始化设备列表界面组件";
    initUI();
    initConnections();
    setupRefreshTimer();
}

DeviceListWidget::~DeviceListWidget()
{
    qDebug() << "DeviceListWidget: 销毁设备列表界面组件";
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

void DeviceListWidget::setScannerManager(DScannerManager *manager)
{
    if (m_scannerManager == manager) {
        return;
    }
    
    qDebug() << "DeviceListWidget: 设置扫描仪管理器";
    m_scannerManager = manager;
    
    if (m_scannerManager) {
        // 连接扫描仪管理器信号
        connect(m_scannerManager, &DScannerManager::deviceDiscovered,
                this, &DeviceListWidget::onDeviceDiscovered);
        connect(m_scannerManager, &DScannerManager::deviceRemoved,
                this, &DeviceListWidget::onDeviceRemoved);
        connect(m_scannerManager, &DScannerManager::deviceConnected,
                this, &DeviceListWidget::onDeviceConnected);
        connect(m_scannerManager, &DScannerManager::deviceDisconnected,
                this, &DeviceListWidget::onDeviceDisconnected);
        connect(m_scannerManager, &DScannerManager::discoveryFinished,
                this, &DeviceListWidget::onDiscoveryFinished);
        
        // 立即刷新设备列表
        refreshDeviceList();
    }
}

void DeviceListWidget::initUI()
{
    qDebug() << "DeviceListWidget: 初始化用户界面";
    
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 创建标题区域
    auto *titleFrame = new DFrame(this);
    titleFrame->setFrameStyle(DFrame::NoFrame);
    auto *titleLayout = new QHBoxLayout(titleFrame);
    
    auto *titleLabel = new DLabel("扫描设备", this);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T5, QFont::DemiBold);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
    m_refreshButton = new DPushButton("刷新", this);
    m_refreshButton->setIcon(QIcon::fromTheme("view-refresh"));
    titleLayout->addWidget(m_refreshButton);
    
    mainLayout->addWidget(titleFrame);
    
    // 创建设备列表
    m_deviceList = new DListWidget(this);
    m_deviceList->setMinimumHeight(200);
    m_deviceList->setAlternatingRowColors(true);
    mainLayout->addWidget(m_deviceList);
    
    // 创建操作按钮区域
    auto *buttonFrame = new DFrame(this);
    buttonFrame->setFrameStyle(DFrame::NoFrame);
    auto *buttonLayout = new QHBoxLayout(buttonFrame);
    
    m_connectButton = new DPushButton("连接", this);
    m_connectButton->setIcon(QIcon::fromTheme("network-connect"));
    m_connectButton->setEnabled(false);
    
    m_disconnectButton = new DPushButton("断开", this);
    m_disconnectButton->setIcon(QIcon::fromTheme("network-disconnect"));
    m_disconnectButton->setEnabled(false);
    
    m_propertiesButton = new DPushButton("属性", this);
    m_propertiesButton->setIcon(QIcon::fromTheme("document-properties"));
    m_propertiesButton->setEnabled(false);
    
    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addWidget(m_propertiesButton);
    buttonLayout->addStretch();
    
    mainLayout->addWidget(buttonFrame);
    
    // 创建状态显示区域
    auto *statusFrame = new DFrame(this);
    statusFrame->setFrameStyle(DFrame::NoFrame);
    auto *statusLayout = new QHBoxLayout(statusFrame);
    
    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(16, 16);
    m_spinner->hide();
    
    m_statusLabel = new DLabel("准备就绪", this);
    
    statusLayout->addWidget(m_spinner);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    
    mainLayout->addWidget(statusFrame);
    
    setLayout(mainLayout);
}

void DeviceListWidget::initConnections()
{
    qDebug() << "DeviceListWidget: 初始化信号连接";
    
    // 按钮信号连接
    connect(m_refreshButton, &DPushButton::clicked,
            this, &DeviceListWidget::refreshDeviceList);
    connect(m_connectButton, &DPushButton::clicked,
            this, &DeviceListWidget::connectSelectedDevice);
    connect(m_disconnectButton, &DPushButton::clicked,
            this, &DeviceListWidget::disconnectSelectedDevice);
    connect(m_propertiesButton, &DPushButton::clicked,
            this, &DeviceListWidget::showDeviceProperties);
    
    // 列表选择信号连接
    connect(m_deviceList, &DListWidget::itemSelectionChanged,
            this, &DeviceListWidget::onDeviceSelectionChanged);
    connect(m_deviceList, &DListWidget::itemDoubleClicked,
            this, &DeviceListWidget::connectSelectedDevice);
}

void DeviceListWidget::setupRefreshTimer()
{
    qDebug() << "DeviceListWidget: 设置自动刷新定时器";
    
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000); // 30秒自动刷新
    connect(m_refreshTimer, &QTimer::timeout,
            this, &DeviceListWidget::refreshDeviceList);
    m_refreshTimer->start();
}

void DeviceListWidget::refreshDeviceList()
{
    if (!m_scannerManager) {
        qWarning() << "DeviceListWidget: 扫描仪管理器未设置，无法刷新设备列表";
        return;
    }
    
    qDebug() << "DeviceListWidget: 开始刷新设备列表";
    
    // 显示加载状态
    m_spinner->show();
    m_spinner->start();
    m_statusLabel->setText("正在搜索设备...");
    m_refreshButton->setEnabled(false);
    
    // 清空当前列表
    m_deviceList->clear();
    m_devices.clear();
    
    // 开始设备发现
    m_scannerManager->discoverDevices();
}

void DeviceListWidget::connectSelectedDevice()
{
    auto *currentItem = m_deviceList->currentItem();
    if (!currentItem) {
        qWarning() << "DeviceListWidget: 未选择设备";
        return;
    }
    
    QString deviceId = currentItem->data(Qt::UserRole).toString();
    if (deviceId.isEmpty()) {
        qWarning() << "DeviceListWidget: 设备ID为空";
        return;
    }
    
    qDebug() << "DeviceListWidget: 连接设备:" << deviceId;
    
    if (m_scannerManager) {
        try {
            m_scannerManager->connectDevice(deviceId);
            m_statusLabel->setText("正在连接设备...");
        } catch (const DScannerException &e) {
            qCritical() << "DeviceListWidget: 连接设备失败:" << e.what();
            DMessageBox::warning(this, "连接失败", 
                                QString("连接设备失败: %1").arg(e.what()));
        }
    }
}

void DeviceListWidget::disconnectSelectedDevice()
{
    auto *currentItem = m_deviceList->currentItem();
    if (!currentItem) {
        qWarning() << "DeviceListWidget: 未选择设备";
        return;
    }
    
    QString deviceId = currentItem->data(Qt::UserRole).toString();
    if (deviceId.isEmpty()) {
        qWarning() << "DeviceListWidget: 设备ID为空";
        return;
    }
    
    qDebug() << "DeviceListWidget: 断开设备:" << deviceId;
    
    if (m_scannerManager) {
        try {
            m_scannerManager->disconnectDevice(deviceId);
            m_statusLabel->setText("正在断开设备...");
        } catch (const DScannerException &e) {
            qCritical() << "DeviceListWidget: 断开设备失败:" << e.what();
            DMessageBox::warning(this, "断开失败", 
                                QString("断开设备失败: %1").arg(e.what()));
        }
    }
}

void DeviceListWidget::showDeviceProperties()
{
    auto *currentItem = m_deviceList->currentItem();
    if (!currentItem) {
        return;
    }
    
    QString deviceId = currentItem->data(Qt::UserRole).toString();
    if (deviceId.isEmpty()) {
        return;
    }
    
    qDebug() << "DeviceListWidget: 显示设备属性:" << deviceId;
    
    // 发射显示设备属性信号
    emit devicePropertiesRequested(deviceId);
}

void DeviceListWidget::onDeviceSelectionChanged()
{
    auto *currentItem = m_deviceList->currentItem();
    bool hasSelection = (currentItem != nullptr);
    
    m_propertiesButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString deviceId = currentItem->data(Qt::UserRole).toString();
        auto it = m_devices.find(deviceId);
        if (it != m_devices.end()) {
            DeviceInfo deviceInfo = it.value();
            
            // 根据设备状态更新按钮
            m_connectButton->setEnabled(deviceInfo.status == DeviceStatus::Disconnected);
            m_disconnectButton->setEnabled(deviceInfo.status == DeviceStatus::Connected);
            
            // 发射设备选择信号
            emit deviceSelected(deviceInfo);
        }
    } else {
        m_connectButton->setEnabled(false);
        m_disconnectButton->setEnabled(false);
    }
}

void DeviceListWidget::onDeviceDiscovered(const DeviceInfo &device)
{
    qDebug() << "DeviceListWidget: 发现设备:" << device.id << device.name;
    
    // 添加到设备映射
    m_devices[device.id] = device;
    
    // 创建列表项
    auto *item = new QListWidgetItem();
    item->setData(Qt::UserRole, device.id);
    
    // 设置显示文本
    QString displayText = QString("%1 (%2)").arg(device.name, device.vendor);
    if (!device.model.isEmpty() && device.model != device.name) {
        displayText += QString(" - %1").arg(device.model);
    }
    item->setText(displayText);
    
    // 设置图标
    QString iconName;
    switch (device.type) {
        case DeviceType::Flatbed:
            iconName = "scanner";
            break;
        case DeviceType::ADF:
            iconName = "document-scanner";
            break;
        case DeviceType::Network:
            iconName = "network-scanner";
            break;
        default:
            iconName = "scanner";
            break;
    }
    item->setIcon(QIcon::fromTheme(iconName));
    
    // 设置状态提示
    QString tooltip = QString("设备: %1\n制造商: %2\n型号: %3\n连接方式: %4\n状态: %5")
                        .arg(device.name)
                        .arg(device.vendor)
                        .arg(device.model)
                        .arg(device.type == DeviceType::Network ? "网络" : "USB")
                        .arg(device.status == DeviceStatus::Connected ? "已连接" : "未连接");
    item->setToolTip(tooltip);
    
    m_deviceList->addItem(item);
}

void DeviceListWidget::onDeviceRemoved(const QString &deviceId)
{
    qDebug() << "DeviceListWidget: 移除设备:" << deviceId;
    
    // 从设备映射中移除
    m_devices.remove(deviceId);
    
    // 从列表中移除
    for (int i = 0; i < m_deviceList->count(); ++i) {
        auto *item = m_deviceList->item(i);
        if (item && item->data(Qt::UserRole).toString() == deviceId) {
            delete m_deviceList->takeItem(i);
            break;
        }
    }
}

void DeviceListWidget::onDeviceConnected(const QString &deviceId)
{
    qDebug() << "DeviceListWidget: 设备已连接:" << deviceId;
    
    // 更新设备状态
    auto it = m_devices.find(deviceId);
    if (it != m_devices.end()) {
        it.value().status = DeviceStatus::Connected;
        
        // 更新列表项显示
        updateDeviceListItem(deviceId);
        
        // 更新状态
        m_statusLabel->setText("设备连接成功");
        
        // 发射连接成功信号
        emit deviceConnected(it.value());
    }
}

void DeviceListWidget::onDeviceDisconnected(const QString &deviceId)
{
    qDebug() << "DeviceListWidget: 设备已断开:" << deviceId;
    
    // 更新设备状态
    auto it = m_devices.find(deviceId);
    if (it != m_devices.end()) {
        it.value().status = DeviceStatus::Disconnected;
        
        // 更新列表项显示
        updateDeviceListItem(deviceId);
        
        // 更新状态
        m_statusLabel->setText("设备已断开");
        
        // 发射断开信号
        emit deviceDisconnected(deviceId);
    }
}

void DeviceListWidget::onDiscoveryFinished()
{
    qDebug() << "DeviceListWidget: 设备发现完成";
    
    // 隐藏加载状态
    m_spinner->stop();
    m_spinner->hide();
    m_refreshButton->setEnabled(true);
    
    // 更新状态
    int deviceCount = m_devices.size();
    if (deviceCount == 0) {
        m_statusLabel->setText("未发现扫描设备");
    } else {
        m_statusLabel->setText(QString("发现 %1 个扫描设备").arg(deviceCount));
    }
}

void DeviceListWidget::updateDeviceListItem(const QString &deviceId)
{
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end()) {
        return;
    }
    
    const DeviceInfo &device = it.value();
    
    // 查找对应的列表项
    for (int i = 0; i < m_deviceList->count(); ++i) {
        auto *item = m_deviceList->item(i);
        if (item && item->data(Qt::UserRole).toString() == deviceId) {
            // 更新显示文本
            QString displayText = QString("%1 (%2)").arg(device.name, device.vendor);
            if (!device.model.isEmpty() && device.model != device.name) {
                displayText += QString(" - %1").arg(device.model);
            }
            
            // 添加连接状态指示
            if (device.status == DeviceStatus::Connected) {
                displayText += " [已连接]";
            }
            
            item->setText(displayText);
            
            // 更新工具提示
            QString tooltip = QString("设备: %1\n制造商: %2\n型号: %3\n连接方式: %4\n状态: %5")
                                .arg(device.name)
                                .arg(device.vendor)
                                .arg(device.model)
                                .arg(device.type == DeviceType::Network ? "网络" : "USB")
                                .arg(device.status == DeviceStatus::Connected ? "已连接" : "未连接");
            item->setToolTip(tooltip);
            
            break;
        }
    }
    
    // 更新按钮状态
    onDeviceSelectionChanged();
} 