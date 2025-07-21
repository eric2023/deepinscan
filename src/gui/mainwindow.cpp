// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "widgets/devicelistwidget.h"
#include "widgets/scancontrolwidget.h"
#include "widgets/imagepreviewwidget.h"
#include "widgets/imageprocessingwidget.h"
#include "widgets/batchprocesswidget.h"
#include "dialogs/settingsdialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/advancedexportdialog.h"

#include <DApplication>
#include <DMenu>
#include <DMessageBox>
#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DProgressBar>
#include <QCloseEvent>
#include <QShowEvent>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
    , m_scannerManager(nullptr)
    , m_currentDevice(nullptr)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_deviceList(nullptr)
    , m_scanControl(nullptr)
    , m_imagePreview(nullptr)
    , m_imageProcessing(nullptr)
    , m_batchProcess(nullptr)
    , m_rightPanel(nullptr)
    , m_toolBar(nullptr)
    , m_statusBar(nullptr)
    , m_isInitialized(false)
    , m_isScanInProgress(false)
    , m_currentViewMode(SingleScanMode)
    , m_previewButton(nullptr)
    , m_scanButton(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_previewLabel(nullptr)
    , m_resolutionCombo(nullptr)
    , m_modeCombo(nullptr)
    , m_imagePreviewWidget(nullptr)
    , m_batchProcessWidget(nullptr)
    , m_deviceListWidget(nullptr)
    , m_scanControlWidget(nullptr)
    , m_injectedImageProcessor(nullptr)
    , m_injectedNetworkDiscovery(nullptr)
    , m_componentsInjected(false)
{
    setWindowTitle(tr("DeepinScan - 专业扫描仪应用"));
    setWindowIcon(QIcon(":/icons/deepinscan.svg"));
    setMinimumSize(1024, 768);
    
    // 设置窗口属性
    setAttribute(Qt::WA_DeleteOnClose);
    
    qDebug() << "MainWindow constructed";
}

MainWindow::~MainWindow()
{
    cleanup();
    qDebug() << "MainWindow destroyed";
}

bool MainWindow::initialize()
{
    if (m_isInitialized) {
        return true;
    }

    qDebug() << "Initializing MainWindow...";

    try {
        // 创建扫描仪管理器
        m_scannerManager = DScannerManager::instance();
        if (!m_scannerManager) {
            showErrorMessage(tr("无法创建扫描仪管理器"));
            return false;
        }

        // 初始化扫描仪管理器
        if (!m_scannerManager->initialize()) {
            showErrorMessage(tr("无法初始化扫描仪管理器"));
            return false;
        }

        // 设置用户界面
        setupUI();
        setupConnections();
        
        // 新增：如果组件已注入，立即配置
        if (m_componentsInjected) {
            configureWidgetComponents();
        }
        
        // 加载设置
        loadSettings();
        
        // 启动设备发现
        QTimer::singleShot(100, this, [this]() {
            m_deviceList->refreshDeviceList();
        });

        m_isInitialized = true;
        showMessage(tr("DeepinScan 已成功初始化"));
        
        qDebug() << "MainWindow initialized successfully";
        return true;

    } catch (const std::exception &e) {
        showErrorMessage(tr("初始化失败: %1").arg(e.what()));
        return false;
    }
}

void MainWindow::cleanup()
{
    if (!m_isInitialized) {
        return;
    }

    qDebug() << "Cleaning up MainWindow...";

    // 保存设置
    saveSettings();

    // 停止扫描
    if (m_isScanInProgress && m_currentDevice) {
        m_currentDevice->pauseScan(); // DScannerDevice没有cancelScan方法，使用pauseScan
    }

    // 关闭当前设备
    if (m_currentDevice) {
        m_currentDevice->close();
        m_currentDevice = nullptr;
    }

    // 关闭扫描仪管理器
    if (m_scannerManager) {
        m_scannerManager->shutdown();
    }

    m_isInitialized = false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 如果正在扫描，询问用户是否确认退出
    if (m_isScanInProgress) {
        DDialog dialog(this);
        dialog.setWindowTitle(tr("确认退出"));
        dialog.setMessage(tr("正在进行扫描操作，确定要退出应用程序吗？"));
        dialog.setIcon(QIcon::fromTheme("dialog-question"));
        dialog.addButton(tr("取消"), false);
        dialog.addButton(tr("退出"), true, DDialog::ButtonWarning);
        
        if (dialog.exec() == DDialog::Accepted) {
            cleanup();
            event->accept();
        } else {
            event->ignore();
            return;
        }
    } else {
        cleanup();
        event->accept();
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    DMainWindow::showEvent(event);
    
    // 首次显示时初始化
    if (!m_isInitialized) {
        QTimer::singleShot(100, this, &MainWindow::initialize);
    }
}

void MainWindow::setupUI()
{
    // 创建中央部件
    m_centralWidget = new DWidget(this);
    setCentralWidget(m_centralWidget);

    // 创建主分割器
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    m_rightSplitter = new QSplitter(Qt::Vertical, m_centralWidget);

    // 创建功能组件
    m_deviceList = new DeviceListWidget(this);
    m_scanControl = new ScanControlWidget(this);
    m_imagePreview = new ImagePreviewWidget(this);
    m_imageProcessing = new DImageProcessingWidget(this);
    m_batchProcess = new BatchProcessWidget(this);

    // 创建右侧面板
    m_rightPanel = new DStackedWidget(this);
    m_rightPanel->addWidget(m_scanControl);      // 单张扫描模式
    m_rightPanel->addWidget(m_batchProcess);     // 批量扫描模式
    m_rightPanel->addWidget(m_imageProcessing);  // 图像处理模式

    // 设置右侧分割器
    m_rightSplitter->addWidget(m_imagePreview);
    m_rightSplitter->addWidget(m_rightPanel);
    m_rightSplitter->setStretchFactor(0, 3);  // 预览区域占更多空间
    m_rightSplitter->setStretchFactor(1, 1);

    // 设置主分割器
    m_mainSplitter->addWidget(m_deviceList);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setStretchFactor(0, 1);  // 设备列表
    m_mainSplitter->setStretchFactor(1, 3);  // 主工作区域

    // 设置中央部件布局
    QHBoxLayout *centralLayout = new QHBoxLayout(m_centralWidget);
    centralLayout->setContentsMargins(10, 10, 10, 10);
    centralLayout->addWidget(m_mainSplitter);

    // 设置界面组件
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    // 设置初始视图模式
    m_rightPanel->setCurrentIndex(SingleScanMode);
    m_currentViewMode = SingleScanMode;
}

void MainWindow::setupMenuBar()
{
    DMenuBar *menuBar = this->menuBar();

    // 文件菜单
    DMenu *fileMenu = new DMenu(tr("文件(&F)"), this);
    fileMenu->addAction(tr("新建扫描(&N)"), this, [this]() {
        // 使用默认扫描参数
        ScanParameters defaultParams;
        defaultParams.resolution = 300;
        defaultParams.colorMode = ColorMode::Color;
        defaultParams.format = ImageFormat::PNG;
        onScanRequested(defaultParams);
    }, QKeySequence::New);
    fileMenu->addAction(tr("批量扫描(&B)"), this, [this]() {
        onBatchScanRequested();
    }, QKeySequence("Ctrl+B"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("保存图像(&S)"), this, &MainWindow::onImageSaveRequested, QKeySequence::Save);
    fileMenu->addAction(tr("导出(&E)"), this, &MainWindow::onImageExportRequested, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出(&Q)"), this, &MainWindow::onExitRequested, QKeySequence::Quit);
    menuBar->addMenu(fileMenu);

    // 设备菜单
    DMenu *deviceMenu = new DMenu(tr("设备(&D)"), this);
    deviceMenu->addAction(tr("刷新设备列表(&R)"), this, &MainWindow::onDeviceRefreshRequested, QKeySequence::Refresh);
    deviceMenu->addSeparator();
    deviceMenu->addAction(tr("设备属性(&P)"), this, [this]() {
        if (m_currentDevice) {
            // 显示设备属性对话框 - 完整的设备属性显示
    qDebug() << "显示设备属性对话框";
    
    if (!m_scannerManager) {
        QMessageBox::warning(this, tr("错误"), tr("扫描仪管理器未初始化"));
        return;
    }
    
    // 获取当前设备 - 如果有已选择的设备ID，使用已打开的设备
    auto currentDevice = m_currentDevice;
    if (!currentDevice) {
        QMessageBox::warning(this, tr("错误"), tr("未选择扫描设备"));
        return;
    }
    
    // 创建设备属性对话框
    QDialog *propertiesDialog = new QDialog(this);
    propertiesDialog->setWindowTitle(tr("设备属性"));
    propertiesDialog->setModal(true);
    propertiesDialog->resize(600, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(propertiesDialog);
    
    // 创建标签页控件
    QTabWidget *tabWidget = new QTabWidget(propertiesDialog);
    mainLayout->addWidget(tabWidget);
    
    // 基本信息标签页
    QWidget *basicInfoTab = new QWidget();
    QFormLayout *basicLayout = new QFormLayout(basicInfoTab);
    
    DeviceInfo deviceInfo = currentDevice->deviceInfo();
    basicLayout->addRow(tr("设备名称:"), new QLabel(deviceInfo.name));
    basicLayout->addRow(tr("制造商:"), new QLabel(deviceInfo.manufacturer));
    basicLayout->addRow(tr("型号:"), new QLabel(deviceInfo.model));
    basicLayout->addRow(tr("设备ID:"), new QLabel(deviceInfo.deviceId));
    basicLayout->addRow(tr("连接类型:"), new QLabel(deviceInfo.connectionString));
    // 根据驱动类型显示设备类型
    QString deviceTypeStr;
    switch (deviceInfo.driverType) {
        case DriverType::SANE:
            deviceTypeStr = tr("SANE扫描仪");
            break;
        case DriverType::Genesys:
            deviceTypeStr = tr("Genesys扫描仪");
            break;
        case DriverType::Generic:
            deviceTypeStr = tr("通用扫描仪");
            break;
        default:
            deviceTypeStr = tr("未知类型");
            break;
    }
    basicLayout->addRow(tr("设备类型:"), new QLabel(deviceTypeStr));
    basicLayout->addRow(tr("设备状态:"), new QLabel(deviceInfo.isAvailable ? tr("可用") : tr("不可用")));
    
    tabWidget->addTab(basicInfoTab, tr("基本信息"));
    
    // 设备能力标签页
    QWidget *capabilitiesTab = new QWidget();
    QVBoxLayout *capLayout = new QVBoxLayout(capabilitiesTab);
    
    QTreeWidget *capTree = new QTreeWidget();
    capTree->setHeaderLabels({tr("属性"), tr("值")});
    
    ScannerCapabilities caps = currentDevice->capabilities();
    
    auto addTreeItem = [&](QTreeWidgetItem *parent, const QString &key, const QVariant &value) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, key);
        
        if (value.type() == QVariant::List) {
            QVariantList list = value.toList();
            QStringList stringList;
            for (const QVariant &v : list) {
                stringList << v.toString();
            }
            item->setText(1, stringList.join(", "));
        } else if (value.type() == QVariant::Map) {
            item->setText(1, tr("复合属性"));
            QVariantMap map = value.toMap();
            for (auto it = map.begin(); it != map.end(); ++it) {
                QTreeWidgetItem *childItem = new QTreeWidgetItem();
                childItem->setText(0, it.key());
                childItem->setText(1, it.value().toString());
                item->addChild(childItem);
            }
        } else {
            item->setText(1, value.toString());
        }
        
        if (parent) {
            parent->addChild(item);
        } else {
            capTree->addTopLevelItem(item);
        }
    };
    
    // 手动添加ScannerCapabilities的各个字段
    QStringList resolutions;
    for (int res : caps.supportedResolutions) {
        resolutions << QString::number(res);
    }
    addTreeItem(nullptr, tr("支持的分辨率"), resolutions.join(", "));
    
    QStringList colorModes;
    for (ColorMode mode : caps.supportedColorModes) {
        switch (mode) {
            case ColorMode::Lineart: colorModes << tr("黑白"); break;
            case ColorMode::Grayscale: colorModes << tr("灰度"); break;
            case ColorMode::Color: colorModes << tr("彩色"); break;
        }
    }
    addTreeItem(nullptr, tr("支持的颜色模式"), colorModes.join(", "));
    
    QStringList formats;
    for (ImageFormat fmt : caps.supportedFormats) {
        switch (fmt) {
            case ImageFormat::PNG: formats << "PNG"; break;
            case ImageFormat::JPEG: formats << "JPEG"; break;
            case ImageFormat::TIFF: formats << "TIFF"; break;
            case ImageFormat::BMP: formats << "BMP"; break;
            case ImageFormat::PDF: formats << "PDF"; break;
        }
    }
    addTreeItem(nullptr, tr("支持的格式"), formats.join(", "));
    
    addTreeItem(nullptr, tr("最大扫描区域"), QString("%1x%2mm").arg(caps.maxScanArea.width).arg(caps.maxScanArea.height));
    addTreeItem(nullptr, tr("有ADF"), caps.hasADF ? tr("是") : tr("否"));
    addTreeItem(nullptr, tr("支持双面"), caps.hasDuplex ? tr("是") : tr("否"));
    addTreeItem(nullptr, tr("有预览"), caps.hasPreview ? tr("是") : tr("否"));
    
    capTree->expandAll();
    capLayout->addWidget(capTree);
    tabWidget->addTab(capabilitiesTab, tr("设备能力"));
    
    // 连接信息标签页
    QWidget *connectionTab = new QWidget();
    QFormLayout *connLayout = new QFormLayout(connectionTab);
    
    // DeviceInfo没有properties字段，使用现有字段显示连接信息
    if (!deviceInfo.manufacturer.isEmpty()) {
        connLayout->addRow(tr("制造商:"), new QLabel(deviceInfo.manufacturer));
    }
    if (!deviceInfo.model.isEmpty()) {
        connLayout->addRow(tr("型号:"), new QLabel(deviceInfo.model));
    }
    if (!deviceInfo.serialNumber.isEmpty()) {
        connLayout->addRow(tr("序列号:"), new QLabel(deviceInfo.serialNumber));
    }
    
    // 显示通信协议
    QString protocolStr;
    switch (deviceInfo.protocol) {
        case CommunicationProtocol::USB:
            protocolStr = tr("USB");
            break;
        case CommunicationProtocol::Network:
            protocolStr = tr("网络");
            break;
        case CommunicationProtocol::Serial:
            protocolStr = tr("串口");
            break;
        default:
            protocolStr = tr("未知");
            break;
    }
    connLayout->addRow(tr("通信协议:"), new QLabel(protocolStr));
    
    if (!deviceInfo.connectionString.isEmpty()) {
        QStringList addresses = {deviceInfo.connectionString};
        connLayout->addRow(tr("网络地址:"), new QLabel(addresses.join(", ")));
    }
    
    tabWidget->addTab(connectionTab, tr("连接信息"));
    
    // 支持的选项标签页
    QWidget *optionsTab = new QWidget();
    QVBoxLayout *optLayout = new QVBoxLayout(optionsTab);
    
    QListWidget *optionsList = new QListWidget();
    QVariantMap params = currentDevice->parameters();
    QStringList supportedOptions = params.keys();
    for (const QString &option : supportedOptions) {
        QListWidgetItem *item = new QListWidgetItem(option);
        QVariant optionValue = currentDevice->parameter(option);
        if (optionValue.isValid()) {
            item->setText(QString("%1: %2").arg(option).arg(optionValue.toString()));
        }
        optionsList->addItem(item);
    }
    
    optLayout->addWidget(optionsList);
    tabWidget->addTab(optionsTab, tr("支持的选项"));
    
    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *refreshButton = new QPushButton(tr("刷新"));
    QPushButton *closeButton = new QPushButton(tr("关闭"));
    
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接按钮信号
    connect(refreshButton, &QPushButton::clicked, [=]() {
        // 刷新设备信息 - 重新获取设备信息
        DeviceInfo refreshedInfo = currentDevice->deviceInfo();
        QMessageBox::information(propertiesDialog, tr("提示"), tr("设备信息已刷新"));
    });
    
    connect(closeButton, &QPushButton::clicked, propertiesDialog, &QDialog::accept);
    
    // 显示对话框
    propertiesDialog->exec();
    propertiesDialog->deleteLater();
        }
    });
    menuBar->addMenu(deviceMenu);

    // 图像菜单
    DMenu *imageMenu = new DMenu(tr("图像(&I)"), this);
    imageMenu->addAction(tr("图像处理(&P)"), this, &MainWindow::onImageProcessingRequested, QKeySequence("Ctrl+P"));
    menuBar->addMenu(imageMenu);

    // 视图菜单
    DMenu *viewMenu = new DMenu(tr("视图(&V)"), this);
    viewMenu->addAction(tr("单张扫描模式(&S)"), this, [this]() { onViewModeChanged(SingleScanMode); });
    viewMenu->addAction(tr("批量扫描模式(&B)"), this, [this]() { onViewModeChanged(BatchScanMode); });
    viewMenu->addAction(tr("图像处理模式(&P)"), this, [this]() { onViewModeChanged(ProcessingMode); });
    menuBar->addMenu(viewMenu);

    // 帮助菜单
    DMenu *helpMenu = new DMenu(tr("帮助(&H)"), this);
    helpMenu->addAction(tr("用户手册(&M)"), this, &MainWindow::onHelpRequested, QKeySequence::HelpContents);
    helpMenu->addSeparator();
    helpMenu->addAction(tr("关于 DeepinScan(&A)"), this, &MainWindow::onAboutRequested);
    menuBar->addMenu(helpMenu);

    // 设置菜单
    DMenu *settingsMenu = new DMenu(tr("设置(&S)"), this);
    settingsMenu->addAction(tr("首选项(&P)"), this, &MainWindow::onSettingsRequested, QKeySequence::Preferences);
    menuBar->addMenu(settingsMenu);
}

void MainWindow::setupToolBar()
{
    m_toolBar = new DToolBar(this);
    addToolBar(m_toolBar);

    // 添加工具栏按钮
    m_toolBar->addAction(QIcon::fromTheme("document-new"), tr("新建扫描"), 
                        this, [this]() {
                            // 使用默认扫描参数
                            ScanParameters defaultParams;
                            defaultParams.resolution = 300;
                            defaultParams.colorMode = ColorMode::Color;
                            defaultParams.format = ImageFormat::PNG;
                            onScanRequested(defaultParams);
                        });
    m_toolBar->addAction(QIcon::fromTheme("view-preview"), tr("预览"), 
                        this, [this]() {
                            if (m_scanControl) {
                                // 修复：获取扫描参数并发射预览请求
                                ScanParameters params = m_scanControl->getScanParameters();
                                onPreviewRequested(params);
                            }
                        });
    m_toolBar->addSeparator();
    
    m_toolBar->addAction(QIcon::fromTheme("document-save"), tr("保存"), 
                        this, &MainWindow::onImageSaveRequested);
    m_toolBar->addAction(QIcon::fromTheme("document-export"), tr("导出"), 
                        this, &MainWindow::onImageExportRequested);
    m_toolBar->addSeparator();
    
    m_toolBar->addAction(QIcon::fromTheme("configure"), tr("设置"), 
                        this, &MainWindow::onSettingsRequested);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new DStatusBar(this);
    setStatusBar(m_statusBar);

    // 添加状态栏组件
    DLabel *statusLabel = new DLabel(tr("就绪"), this);
    m_statusBar->addWidget(statusLabel);

    m_statusBar->addPermanentWidget(new DLabel(tr("DeepinScan v1.0.0"), this));
}

void MainWindow::setupConnections()
{
    // 设备列表连接
    if (m_deviceList) {
        connect(m_deviceList, &DeviceListWidget::deviceSelected,
                this, &MainWindow::onDeviceSelected);
        connect(m_deviceList, &DeviceListWidget::deviceRefreshRequested,
                this, &MainWindow::onDeviceRefreshRequested);
    }

    // 扫描控制连接
    if (m_scanControl) {
        connect(m_scanControl, &ScanControlWidget::scanRequested,
                this, &MainWindow::onScanRequested);
        connect(m_scanControl, &ScanControlWidget::previewRequested,
                this, &MainWindow::onPreviewRequested);
        connect(m_scanControl, &ScanControlWidget::cancelScanRequested,
                this, &MainWindow::onScanCancelled);
    }

    // 图像预览连接
    if (m_imagePreview) {
        connect(m_imagePreview, &ImagePreviewWidget::imageProcessingRequested,
                this, &MainWindow::onImageProcessingRequested);
        connect(m_imagePreview, &ImagePreviewWidget::imageSaveRequested,
                this, &MainWindow::onImageSaveRequested);
    }

    // 扫描仪管理器连接
    if (m_scannerManager) {
            connect(m_scannerManager, &DScannerManager::deviceOpened,
            this, &MainWindow::onDeviceConnected);
    connect(m_scannerManager, &DScannerManager::deviceClosed,
            this, &MainWindow::onDeviceDisconnected);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // 恢复窗口几何形状
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // 恢复分割器状态
    if (m_mainSplitter) {
        m_mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
    }
    if (m_rightSplitter) {
        m_rightSplitter->restoreState(settings.value("rightSplitter").toByteArray());
    }
    
    // 恢复视图模式
    int viewMode = settings.value("viewMode", SingleScanMode).toInt();
    onViewModeChanged(viewMode);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    // 保存窗口几何形状
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    // 保存分割器状态
    if (m_mainSplitter) {
        settings.setValue("mainSplitter", m_mainSplitter->saveState());
    }
    if (m_rightSplitter) {
        settings.setValue("rightSplitter", m_rightSplitter->saveState());
    }
    
    // 保存视图模式
    settings.setValue("viewMode", static_cast<int>(m_currentViewMode));
}

void MainWindow::showMessage(const QString &message, int timeout)
{
    if (m_statusBar) {
        m_statusBar->showMessage(message, timeout);
    }
    qDebug() << "Status:" << message;
}

void MainWindow::showErrorMessage(const QString &error)
{
    DMessageBox::warning(this, tr("错误"), error);
    showMessage(tr("错误: %1").arg(error), 5000);
    qWarning() << "Error:" << error;
}

// 槽函数实现 - 设备管理
void MainWindow::onDeviceSelected(const DeviceInfo &device)
{
    qDebug() << "Device selected:" << device.name;
    
    // 关闭当前设备
    if (m_currentDevice) {
        m_currentDevice->close();
        m_currentDevice = nullptr;
    }
    
    // 打开新设备
    if (m_scannerManager) {
        m_currentDevice = m_scannerManager->openDevice(device.deviceId);
        if (m_currentDevice) {
            m_currentDeviceId = device.deviceId;
            
            // 连接设备信号
            connect(m_currentDevice, &DScannerDevice::scanCompleted,
                    this, &MainWindow::onScanCompleted);
            connect(m_currentDevice, QOverload<int>::of(&DScannerDevice::scanProgress),
                    this, &MainWindow::onScanProgress);
            
            // 更新界面状态
            updateDeviceStatus();
            showMessage(tr("已连接到设备: %1").arg(device.name));
            
            // 通知扫描控制器设备已连接
            if (m_scanControl) {
                m_scanControl->setCurrentDevice(m_currentDevice);
            }
            
            // 新增：启用扫描功能
            enableScanningCapabilities();
        } else {
            showErrorMessage(tr("无法连接到设备: %1").arg(device.name));
        }
    }
}

void MainWindow::onDeviceConnected(const QString &deviceId)
{
    Q_UNUSED(deviceId)
    if (m_deviceList) {
        m_deviceList->refreshDeviceList();
    }
    showMessage(tr("检测到新设备"));
}

void MainWindow::onDeviceDisconnected(const QString &deviceId)
{
    if (deviceId == m_currentDeviceId) {
        m_currentDevice = nullptr;
        m_currentDeviceId.clear();
        updateDeviceStatus();
        showMessage(tr("设备已断开连接"));
    }
    
    if (m_deviceList) {
        m_deviceList->refreshDeviceList();
    }
}

void MainWindow::onDeviceRefreshRequested()
{
    if (m_deviceList) {
        m_deviceList->refreshDeviceList();
    }
    showMessage(tr("正在刷新设备列表..."));
}

// 槽函数实现 - 扫描控制
void MainWindow::onScanRequested(const ScanParameters &params)
{
    if (!m_currentDevice) {
        showErrorMessage(tr("请先选择一个扫描设备"));
        return;
    }
    
    if (m_isScanInProgress) {
        showErrorMessage(tr("扫描正在进行中，请稍候"));
        return;
    }
    
    qDebug() << "Starting scan with parameters:" << params.resolution << "DPI";
    
    m_isScanInProgress = true;
    updateScanStatus();
    
    // 开始扫描
    if (!m_currentDevice->startScan(params)) {
        m_isScanInProgress = false;
        updateScanStatus();
        showErrorMessage(tr("无法启动扫描"));
    } else {
        showMessage(tr("正在扫描..."));
    }
}

void MainWindow::onPreviewRequested(const ScanParameters &params)
{
    if (!m_currentDevice) {
        showErrorMessage(tr("请先选择一个扫描设备"));
        return;
    }
    
    qDebug() << "Starting preview scan";
    
    // 实现预览扫描 - 完整的预览扫描功能
    qDebug() << "开始预览扫描";
    
    if (!m_scannerManager) {
        QMessageBox::warning(this, tr("错误"), tr("扫描仪管理器未初始化"));
        return;
    }
    
    // 获取当前选中的设备
    auto currentDevice = m_currentDevice;
    if (!currentDevice) {
        QMessageBox::warning(this, tr("错误"), tr("未选择扫描设备"));
        return;
    }
    
    // 设置预览扫描参数
    ScanParameters previewParams;
    previewParams.resolution = 150;  // 预览使用低分辨率
    previewParams.colorMode = ColorMode::Color;
    previewParams.format = ImageFormat::JPEG;
    // 设置A4尺寸扫描区域 (150 DPI)
    previewParams.area = {0, 0, 210, 297}; // A4 in mm
    
    // 从界面获取用户设置
    if (m_resolutionCombo) {
        // 为预览使用固定的低分辨率以提高速度
        previewParams.resolution = 150;
    }
    
    if (m_modeCombo) {
        QString modeText = m_modeCombo->currentText();
        if (modeText == tr("黑白")) {
            previewParams.colorMode = ColorMode::Lineart;
        } else if (modeText == tr("灰度")) {
            previewParams.colorMode = ColorMode::Grayscale;
        } else {
            previewParams.colorMode = ColorMode::Color;
        }
    }
    
    // 设置预览标志
    // previewParams.isPreview = true; // ScanParameters没有isPreview字段
    
    // 禁用预览按钮，启用进度指示
    m_previewButton->setEnabled(false);
    m_previewButton->setText(tr("预览中..."));
    
    // 显示进度条
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_statusLabel->setText(tr("正在进行预览扫描..."));
    
    // 连接扫描信号
    connect(m_currentDevice, &DScannerDevice::scanStarted, this, [this]() {
        qDebug() << "预览扫描开始";
        m_statusLabel->setText(tr("预览扫描进行中..."));
    });
    
    connect(m_currentDevice, QOverload<int>::of(&DScannerDevice::scanProgress), this, [this](int progress) {
        m_progressBar->setValue(progress);
        m_statusLabel->setText(tr("预览扫描进度: %1%").arg(progress));
    });
    
    connect(m_currentDevice, &DScannerDevice::scanCompleted, this, [this](const QImage &image) {
        qDebug() << "预览扫描完成，图像大小:" << image.size();
        
        // 恢复预览按钮
        m_previewButton->setEnabled(true);
        m_previewButton->setText(tr("预览"));
        m_progressBar->setVisible(false);
        m_statusLabel->setText(tr("预览扫描完成"));
        
        // 处理预览图像
        if (!image.isNull()) {
            // 在预览区域显示图像
            if (m_previewLabel) {
                QPixmap pixmap = QPixmap::fromImage(image);
                QSize labelSize = m_previewLabel->size();
                QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                m_previewLabel->setPixmap(scaledPixmap);
                
                // 保存原始预览图像用于裁剪
                m_previewImage = image;
                
                // 启用扫描按钮
                if (m_scanButton) {
                    m_scanButton->setEnabled(true);
                }
            }
            
            QMessageBox::information(this, tr("预览完成"), 
                                    tr("预览扫描完成！您可以选择扫描区域或直接进行扫描。"));
        } else {
            QMessageBox::warning(this, tr("错误"), tr("无法加载预览图像"));
        }
        
        // 断开连接
        m_currentDevice->disconnect(this);
    });
    
    connect(m_currentDevice, QOverload<const QString &>::of(&DScannerDevice::errorOccurred), this, [this](const QString &error) {
        qWarning() << "预览扫描错误:" << error;
        
        // 恢复预览按钮
        m_previewButton->setEnabled(true);
        m_previewButton->setText(tr("预览"));
        m_progressBar->setVisible(false);
        m_statusLabel->setText(tr("预览扫描失败"));
        
        QMessageBox::critical(this, tr("预览错误"), tr("预览扫描失败: %1").arg(error));
        
        // 断开连接
        m_currentDevice->disconnect(this);
    });
    
    // 开始预览扫描
    bool scanStarted = currentDevice->startScan(previewParams);
    if (!scanStarted) {
        m_previewButton->setEnabled(true);
        m_previewButton->setText(tr("预览"));
        m_progressBar->setVisible(false);
        m_statusLabel->setText(tr("准备就绪"));
        
        QMessageBox::critical(this, tr("错误"), tr("无法启动预览扫描"));
    }
    showMessage(tr("正在生成预览..."));
}

void MainWindow::onScanCancelled()
{
    if (m_currentDevice && m_isScanInProgress) {
        m_currentDevice->pauseScan(); // DScannerDevice没有cancelScan方法，使用pauseScan
        m_isScanInProgress = false;
        updateScanStatus();
        showMessage(tr("扫描已取消"));
    }
}

void MainWindow::onScanCompleted(const QImage &image)
{
    m_isScanInProgress = false;
    updateScanStatus();
    
    if (!image.isNull()) {
        m_currentImage = image;
        
        // 在预览组件中显示图像
        if (m_imagePreview) {
            m_imagePreview->setPreviewImage(QPixmap::fromImage(image));
        }
        
        showMessage(tr("扫描完成"));
        qDebug() << "Scan completed, image size:" << image.size();
    } else {
        showErrorMessage(tr("扫描失败"));
    }
}

void MainWindow::onScanProgress(int percentage)
{
    showMessage(tr("扫描进度: %1%").arg(percentage));
}

// 槽函数实现 - 图像处理
void MainWindow::onImageProcessingRequested()
{
    if (m_currentImage.isNull()) {
        showErrorMessage(tr("没有可处理的图像"));
        return;
    }
    
    // 切换到图像处理模式
    onViewModeChanged(ProcessingMode);
    
    // 设置要处理的图像
    if (m_imageProcessing) {
        m_imageProcessing->setSourceImage(QPixmap::fromImage(m_currentImage));
    }
}

void MainWindow::onImageSaveRequested()
{
    if (m_currentImage.isNull()) {
        showErrorMessage(tr("没有可保存的图像"));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("保存图像"), QString("scan_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        tr("图像文件 (*.png *.jpg *.jpeg *.bmp *.tiff)"));
    
    if (!fileName.isEmpty()) {
        if (m_currentImage.save(fileName)) {
            showMessage(tr("图像已保存到: %1").arg(fileName));
        } else {
            showErrorMessage(tr("保存图像失败"));
        }
    }
}

void MainWindow::onImageExportRequested()
{
    qDebug() << "MainWindow::onImageExportRequested: 开始高级导出";
    
    // 获取当前扫描的图像
    QList<QImage> images;
    if (m_imagePreview && !m_imagePreview->getPreviewImage().isNull()) {
        images.append(m_imagePreview->getPreviewImage().toImage());
    }
    
    if (images.isEmpty()) {
        showMessage(tr("没有可导出的图像"));
        return;
    }
    
    // 创建高级导出对话框
    Dtk::Scanner::AdvancedExportDialog *exportDialog = new Dtk::Scanner::AdvancedExportDialog(this);
    // 设置要导出的图像
    exportDialog->setImages(images);
    
    // 连接信号
    connect(exportDialog, &Dtk::Scanner::AdvancedExportDialog::exportStarted,
            this, [this]() {
                showMessage(tr("开始导出..."));
                setEnabled(false);
            });
            
    connect(exportDialog, &Dtk::Scanner::AdvancedExportDialog::exportProgress,
            this, [this](int current, int total) {
                int progress = total > 0 ? (current * 100 / total) : 0;
                showMessage(tr("导出进度: %1% (%2/%3)").arg(progress).arg(current).arg(total));
            });
            
    connect(exportDialog, &Dtk::Scanner::AdvancedExportDialog::exportFinished,
            this, [this](bool success, const QStringList &outputFiles) {
                setEnabled(true);
                if (success) {
                    showMessage(tr("导出完成，生成 %1 个文件").arg(outputFiles.size()));
                } else {
                    showMessage(tr("导出失败"));
                }
            });
            
    connect(exportDialog, &Dtk::Scanner::AdvancedExportDialog::exportError,
            this, [this](const QString &error) {
                setEnabled(true);
                showMessage(tr("导出错误: %1").arg(error));
            });
    
    // 显示对话框
    exportDialog->exec();
    exportDialog->deleteLater();
    
    qDebug() << "MainWindow::onImageExportRequested: 高级导出对话框已完成";
}

// 槽函数实现 - 批量处理
void MainWindow::onBatchScanRequested()
{
    onViewModeChanged(BatchScanMode);
    showMessage(tr("切换到批量扫描模式"));
}

void MainWindow::onBatchProcessingRequested()
{
    qDebug() << "MainWindow::onBatchProcessingRequested: 开始批量处理";
    
    if (!m_batchProcessWidget) {
        qWarning() << "MainWindow::onBatchProcessingRequested: 批量处理组件未初始化";
        showMessage(tr("批量处理组件未初始化"));
        return;
    }
    
    // 切换到批量处理模式
    onViewModeChanged(BatchScanMode);
    
    // 启动批量处理流程
    m_batchProcess->startBatchProcessing();
    
    // 显示状态信息
    showMessage(tr("批量处理已启动，请查看批量处理面板"));
    
    // 更新UI状态
    updateBatchProcessingStatus();
    
    qDebug() << "MainWindow::onBatchProcessingRequested: 批量处理启动完成";
}

// 槽函数实现 - 菜单和工具栏
void MainWindow::onSettingsRequested()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::onAboutRequested()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::onHelpRequested()
{
    QDesktopServices::openUrl(QUrl("https://github.com/linuxdeepin/deepinscan/wiki"));
}

void MainWindow::onExitRequested()
{
    close();
}

// 槽函数实现 - 视图控制
void MainWindow::onViewModeChanged(int mode)
{
    ViewMode newMode = static_cast<ViewMode>(mode);
    if (newMode == m_currentViewMode) {
        return;
    }
    
    m_currentViewMode = newMode;
    
    if (m_rightPanel) {
        m_rightPanel->setCurrentIndex(mode);
    }
    
    QString modeName;
    switch (newMode) {
    case SingleScanMode:
        modeName = tr("单张扫描模式");
        break;
    case BatchScanMode:
        modeName = tr("批量扫描模式");
        break;
    case ProcessingMode:
        modeName = tr("图像处理模式");
        break;
    }
    
    showMessage(tr("切换到: %1").arg(modeName));
}

void MainWindow::updateDeviceStatus()
{
    qDebug() << "MainWindow::updateDeviceStatus: 更新设备状态显示";
    
    if (!m_deviceListWidget) {
        qWarning() << "MainWindow::updateDeviceStatus: 设备列表组件为空";
        return;
    }
    
    // 获取当前设备信息
    QString deviceStatus;
    QString deviceInfo;
    
    if (m_currentDevice) {
        // 设备已连接
        deviceInfo = m_currentDevice->deviceName();
        
        // 检查设备状态
        if (m_currentDevice->isConnected()) {
            deviceStatus = tr("已连接");
            
            // 获取设备详细信息
            QString deviceModel = m_currentDevice->model();
            QString deviceVendor = m_currentDevice->manufacturer();
            
            if (!deviceModel.isEmpty() && !deviceVendor.isEmpty()) {
                deviceInfo = QString("%1 - %2 %3").arg(deviceStatus).arg(deviceVendor).arg(deviceModel);
            } else {
                deviceInfo = QString("%1 - %2").arg(deviceStatus).arg(m_currentDevice->deviceName());
            }
            
            // 检查设备能力
            ScannerCapabilities caps = m_currentDevice->capabilities();
            if (caps.supportedFormats.isEmpty()) {
                deviceInfo += tr(" (能力未知)");
            } else {
                deviceInfo += tr(" (支持 %1 种格式)").arg(caps.supportedFormats.size());
            }
            
        } else {
            deviceStatus = tr("连接中...");
            deviceInfo = QString("%1 - %2").arg(deviceStatus).arg(m_currentDevice->deviceName());
        }
    } else {
        // 无设备连接
        deviceStatus = tr("未连接设备");
        deviceInfo = deviceStatus;
    }
    
    // 更新状态栏
    if (m_statusBar && !m_isScanInProgress) {
        m_statusBar->showMessage(deviceInfo);
    }
    
    // 更新设备列表显示
    m_deviceList->refreshDeviceList();
    
    // 更新扫描控制面板
    if (m_scanControl) {
        // 通过重新设置当前设备来触发能力更新
        m_scanControl->setCurrentDevice(m_currentDevice);
    }
    
    qDebug() << "MainWindow::updateDeviceStatus: 设备状态更新完成";
    qDebug() << "  - 设备状态:" << deviceStatus;
    qDebug() << "  - 设备信息:" << deviceInfo;
}

void MainWindow::updateScanStatus()
{
    qDebug() << "MainWindow::updateScanStatus: 更新扫描状态显示";
    
    if (!m_scanControl) {
        return;
    }
    
    // 获取扫描状态
    bool isScanning = m_isScanInProgress;
    
    // 更新状态栏
    if (isScanning) {
        if (m_statusBar) {
            m_statusBar->showMessage(tr("正在扫描..."));
        }
    } else {
        if (m_statusBar) {
            if (m_currentDevice) {
                m_statusBar->showMessage(tr("设备就绪: %1").arg(m_currentDevice->deviceName()));
            } else {
                m_statusBar->showMessage(tr("未连接设备"));
            }
        }
    }
    
    qDebug() << "MainWindow::updateScanStatus: 扫描状态更新完成，当前状态:" << (isScanning ? "扫描中" : "就绪");
}

void MainWindow::updateBatchProcessingStatus()
{
    qDebug() << "MainWindow::updateBatchProcessingStatus: 更新批量处理状态";
    
    if (!m_batchProcessWidget) {
        qWarning() << "MainWindow::updateBatchProcessingStatus: 批量处理组件为空";
        return;
    }
    
    // 获取批量处理状态
    // BatchProcessWidget使用不同的方法名
    int totalTasks = m_batchProcess->getTotalItems();
    int completedTasks = m_batchProcess->getCompletedItems();
    
    // 更新状态栏显示
    if (m_statusBar) {
        if (completedTasks < totalTasks) { // 通过比较任务数判断是否在处理
            QString statusText = tr("批量处理进行中: %1/%2").arg(completedTasks).arg(totalTasks);
            m_statusBar->showMessage(statusText);
        } else if (totalTasks > 0) {
            QString statusText = tr("批量处理完成: %1/%2").arg(completedTasks).arg(totalTasks);
            m_statusBar->showMessage(statusText);
        } else {
            m_statusBar->showMessage(tr("批量处理就绪"));
        }
    }
    
    // 更新工具栏按钮状态
    if (m_toolBar) {
        // 根据处理状态启用/禁用相关操作
        // 这里可以添加具体的按钮状态控制逻辑
    }
    
    qDebug() << "MainWindow::updateBatchProcessingStatus: 批量处理状态更新完成";
    qDebug() << "  - 是否正在处理:" << (completedTasks < totalTasks);
    qDebug() << "  - 总任务数:" << totalTasks;
    qDebug() << "  - 已完成任务数:" << completedTasks;
}

// 新增：组件注入方法实现
void MainWindow::setScannerManager(Dtk::Scanner::DScannerManager *manager)
{
    if (m_scannerManager != manager) {
        m_scannerManager = manager;
        qDebug() << "MainWindow: 扫描仪管理器已注入";
        validateComponentInjection();
    }
}

void MainWindow::setImageProcessor(Dtk::Scanner::DScannerImageProcessor *processor)
{
    m_injectedImageProcessor = processor;
    qDebug() << "MainWindow: 图像处理器已注入";
    validateComponentInjection();
}

void MainWindow::setNetworkDiscovery(Dtk::Scanner::DScannerNetworkDiscovery *discovery)
{
    m_injectedNetworkDiscovery = discovery;
    qDebug() << "MainWindow: 网络发现组件已注入";
    validateComponentInjection();
}

void MainWindow::validateComponentInjection()
{
    bool allComponentsReady = (m_scannerManager != nullptr);
    
    if (allComponentsReady && !m_componentsInjected) {
        m_componentsInjected = true;
        qDebug() << "MainWindow: 所有组件注入完成";
        
        // 如果UI已经创建，立即配置组件
        if (m_isInitialized) {
            configureWidgetComponents();
        }
    }
}

void MainWindow::configureWidgetComponents()
{
    qDebug() << "MainWindow: 开始配置子组件";
    
    // 配置设备列表组件
    if (m_scannerManager && m_deviceList) {
        m_deviceList->setScannerManager(m_scannerManager);
        qDebug() << "MainWindow: 设备列表组件已配置";
    }
    
    // 配置图像处理组件
    if (m_injectedImageProcessor && m_imageProcessing) {
        m_imageProcessing->setImageProcessor(m_injectedImageProcessor);
        qDebug() << "MainWindow: 图像处理组件已配置";
    }
    
    // 激活扫描功能
    enableScanningCapabilities();
}

void MainWindow::enableScanningCapabilities()
{
    if (m_scanControl) {
        // 如果有当前设备，启用扫描控制
        if (m_currentDevice) {
            m_scanControl->setCurrentDevice(m_currentDevice);
            m_scanControl->setEnabled(true);
            qDebug() << "MainWindow: 扫描控制已启用";
        } else {
            // 没有设备时保持禁用状态，但显示提示
            m_scanControl->setEnabled(false);
            qDebug() << "MainWindow: 等待设备连接";
        }
    }
} 