// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicelistwidget.h"
#include "Scanner/DScannerException.h"
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
    , m_deviceListWidget(nullptr)
    , m_refreshButton(nullptr)
    , m_connectButton(nullptr)
    , m_disconnectButton(nullptr)
    , m_propertiesButton(nullptr)
    , m_statusLabel(nullptr)
    , m_loadingSpinner(nullptr)
    , m_autoRefreshTimer(nullptr)
{
    qDebug() << "DeviceListWidget: 初始化设备列表界面组件";
    setupUI();
    initConnections();
    setupRefreshTimer();
}

DeviceListWidget::~DeviceListWidget()
{
    qDebug() << "DeviceListWidget: 销毁设备列表界面组件";
    if (m_autoRefreshTimer) {
        m_autoRefreshTimer->stop();
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
                this, QOverload<const DeviceInfo &>::of(&DeviceListWidget::onDeviceDiscovered));
        connect(m_scannerManager, &DScannerManager::deviceRemoved,
                this, &DeviceListWidget::onDeviceRemoved);
        connect(m_scannerManager, &DScannerManager::deviceOpened,
                this, &DeviceListWidget::onDeviceConnected);
        connect(m_scannerManager, &DScannerManager::deviceClosed,
                this, &DeviceListWidget::onDeviceDisconnected);
        
        // 立即刷新设备列表
        refreshDeviceList();
    }
}

void DeviceListWidget::setupUI()
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
    m_deviceListWidget = new DListWidget(this);
    m_deviceListWidget->setMinimumHeight(200);
    m_deviceListWidget->setAlternatingRowColors(true);
    mainLayout->addWidget(m_deviceListWidget);
    
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
    
    m_loadingSpinner = new DSpinner(this);
    m_loadingSpinner->setFixedSize(16, 16);
    m_loadingSpinner->hide();
    
    m_statusLabel = new DLabel("准备就绪", this);
    
    statusLayout->addWidget(m_loadingSpinner);
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
    connect(m_deviceListWidget, &DListWidget::itemSelectionChanged,
            this, &DeviceListWidget::onDeviceSelectionChanged);
    connect(m_deviceListWidget, &DListWidget::itemDoubleClicked,
            this, &DeviceListWidget::connectSelectedDevice);
}

void DeviceListWidget::setupRefreshTimer()
{
    qDebug() << "DeviceListWidget: 设置自动刷新定时器";
    
    m_autoRefreshTimer = new QTimer(this);
    m_autoRefreshTimer->setInterval(30000); // 30秒自动刷新
    connect(m_autoRefreshTimer, &QTimer::timeout,
            this, &DeviceListWidget::refreshDeviceList);
    m_autoRefreshTimer->start();
}

void DeviceListWidget::refreshDeviceList()
{
    if (!m_scannerManager) {
        qWarning() << "DeviceListWidget: 扫描仪管理器未设置，无法刷新设备列表";
        return;
    }
    
    qDebug() << "DeviceListWidget: 开始刷新设备列表";
    
    // 显示加载状态
    m_loadingSpinner->show();
    m_loadingSpinner->start();
    m_statusLabel->setText("正在搜索设备...");
    m_refreshButton->setEnabled(false);
    
    // 清空当前列表
    m_deviceListWidget->clear();
    m_devices.clear();
    
    // 开始设备发现
    m_scannerManager->discoverDevices();
}

void DeviceListWidget::connectSelectedDevice()
{
    auto *currentItem = m_deviceListWidget->currentItem();
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
            m_scannerManager->openDevice(deviceId);
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
    auto *currentItem = m_deviceListWidget->currentItem();
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
            m_scannerManager->closeDevice(deviceId);
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
    auto *currentItem = m_deviceListWidget->currentItem();
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
    auto *currentItem = m_deviceListWidget->currentItem();
    bool hasSelection = (currentItem != nullptr);
    
    m_propertiesButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString deviceId = currentItem->data(Qt::UserRole).toString();
        auto it = m_devices.find(deviceId);
        if (it != m_devices.end()) {
            DeviceInfo deviceInfo = it.value();
            
            // 根据设备状态更新按钮
            bool isOpen = m_scannerManager && m_scannerManager->isDeviceOpen(deviceId);
            m_connectButton->setEnabled(deviceInfo.isAvailable && !isOpen);
            m_disconnectButton->setEnabled(isOpen);
            
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
    qDebug() << "DeviceListWidget: 发现设备:" << device.deviceId << device.name;
    
    // 添加到设备映射
    m_devices[device.deviceId] = device;
    
    // 创建列表项
    auto *item = new QListWidgetItem();
    item->setData(Qt::UserRole, device.deviceId);
    
    // 设置显示文本
    QString displayText = QString("%1 (%2)").arg(device.name, device.manufacturer);
    if (!device.model.isEmpty() && device.model != device.name) {
        displayText += QString(" - %1").arg(device.model);
    }
    item->setText(displayText);
    
    // 设置图标
    QString iconName;
    switch (device.protocol) {
        case CommunicationProtocol::Network:
            iconName = "network-scanner";
            break;
        case CommunicationProtocol::USB:
            iconName = "scanner";
            break;
        default:
            iconName = "scanner";
            break;
    }
    item->setIcon(QIcon::fromTheme(iconName));
    
    // 设置状态提示
    QString tooltip = QString("设备: %1\n制造商: %2\n型号: %3\n连接方式: %4\n状态: %5")
                        .arg(device.name)
                        .arg(device.manufacturer)
                        .arg(device.model)
                                                         .arg(device.protocol == CommunicationProtocol::Network ? "网络" : "USB")
                                                         .arg(device.isAvailable ? "可用" : "不可用");
    item->setToolTip(tooltip);
    
    m_deviceListWidget->addItem(item);
}

void DeviceListWidget::onDeviceRemoved(const QString &deviceId)
{
    qDebug() << "DeviceListWidget: 移除设备:" << deviceId;
    
    // 从设备映射中移除
    m_devices.remove(deviceId);
    
    // 从列表中移除
    for (int i = 0; i < m_deviceListWidget->count(); ++i) {
        auto *item = m_deviceListWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == deviceId) {
            delete m_deviceListWidget->takeItem(i);
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
    m_loadingSpinner->stop();
    m_loadingSpinner->hide();
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
    for (int i = 0; i < m_deviceListWidget->count(); ++i) {
        auto *item = m_deviceListWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == deviceId) {
            // 更新显示文本
            QString displayText = QString("%1 (%2)").arg(device.name, device.manufacturer);
            if (!device.model.isEmpty() && device.model != device.name) {
                displayText += QString(" - %1").arg(device.model);
            }
            
            // 添加连接状态指示
            if (device.isAvailable) {
                displayText += " [已连接]";
            }
            
            item->setText(displayText);
            
            // 更新工具提示
            QString tooltip = QString("设备: %1\n制造商: %2\n型号: %3\n连接方式: %4\n状态: %5")
                                .arg(device.name)
                                .arg(device.manufacturer)
                                .arg(device.model)
                                .arg(device.protocol == CommunicationProtocol::Network ? "网络" : "USB")
                                .arg(device.isAvailable ? "可用" : "不可用");
            item->setToolTip(tooltip);
            
            break;
        }
    }
    
    // 更新按钮状态
    onDeviceSelectionChanged();
}

// 缺失的槽函数实现 - 存根版本
void DeviceListWidget::onDeviceItemClicked(QListWidgetItem *item)
{
    Q_UNUSED(item)
    qDebug() << "DeviceListWidget::onDeviceItemClicked - 存根实现";
}

void DeviceListWidget::onDeviceItemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item)
    qDebug() << "DeviceListWidget::onDeviceItemDoubleClicked - 存根实现";
}

void DeviceListWidget::onRefreshButtonClicked()
{
    qDebug() << "DeviceListWidget::onRefreshButtonClicked - 存根实现";
    refreshDeviceList(); // 调用现有的刷新方法
}

void DeviceListWidget::onAutoRefreshTimeout()
{
    qDebug() << "DeviceListWidget::onAutoRefreshTimeout - 存根实现";
    refreshDeviceList(); // 调用现有的刷新方法
}

void DeviceListWidget::onDeviceDiscovered()
{
    qDebug() << "DeviceListWidget::onDeviceDiscovered - 存根实现";
}

void DeviceListWidget::onDeviceStatusChanged()
{
    qDebug() << "DeviceListWidget::onDeviceStatusChanged - 存根实现";
}

// DeviceListItem事件处理实现 - 存根版本
void DeviceListItem::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    qDebug() << "DeviceListItem::mousePressEvent - 存根实现";
}

void DeviceListItem::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    qDebug() << "DeviceListItem::mouseReleaseEvent - 存根实现";
    emit clicked();
}

void DeviceListItem::enterEvent(QEvent *event)
{
    Q_UNUSED(event)
    qDebug() << "DeviceListItem::enterEvent - 存根实现";
}

void DeviceListItem::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    qDebug() << "DeviceListItem::leaveEvent - 存根实现";
}

void DeviceListItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    qDebug() << "DeviceListItem::paintEvent - 存根实现";
    // 调用父类的实现以确保基本绘制
    QWidget::paintEvent(event);
} 