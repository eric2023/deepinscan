/*
 * SPDX-FileCopyrightText: 2024-2025 eric2023
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "settingsdialog.h"
#include <DLabel>
#include <DPushButton>
#include <DComboBox>
#include <DSpinBox>
#include <DCheckBox>
#include <DSlider>
#include <DLineEdit>
#include <DGroupBox>
#include <DListWidget>
#include <DStackedWidget>
#include <DFrame>
#include <DScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

DWIDGET_USE_NAMESPACE

DSettingsDialog::DSettingsDialog(QWidget *parent)
    : DDialog(parent)
    , m_settings(nullptr)
{
    qDebug() << "初始化设置对话框";
    
    initializeSettings();
    setupUI();
    connectSignals();
    loadSettings();
    
    qDebug() << "设置对话框初始化完成";
}

DSettingsDialog::~DSettingsDialog()
{
    qDebug() << "销毁设置对话框";
}

void DSettingsDialog::initializeSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir configDir(configPath + "/deepinscan");
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    
    QString settingsFile = configDir.absoluteFilePath("settings.ini");
    m_settings = new QSettings(settingsFile, QSettings::IniFormat, this);
    
    qDebug() << QString("设置文件路径: %1").arg(settingsFile);
}

void DSettingsDialog::setupUI()
{
    qDebug() << "设置对话框UI";
    
    setTitle("设置");
    setFixedSize(800, 600);
    
    // 创建主布局
    auto *mainWidget = new DWidget(this);
    auto *mainLayout = new QHBoxLayout(mainWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 左侧分类列表
    setupCategoryList();
    mainLayout->addWidget(m_categoryList);
    
    // 右侧设置内容
    setupSettingsContent();
    mainLayout->addWidget(m_stackedWidget);
    
    addContent(mainWidget);
    
    // 设置按钮
    addButton("取消", false, DDialog::ButtonNormal);
    addButton("应用", false, DDialog::ButtonRecommend);
    addButton("确定", true, DDialog::ButtonRecommend);
    
    qDebug() << "设置对话框UI设置完成";
}

void DSettingsDialog::setupCategoryList()
{
    qDebug() << "设置分类列表";
    
    m_categoryList = new DListWidget(this);
    m_categoryList->setFixedWidth(150);
    m_categoryList->setAlternatingRowColors(true);
    
    // 添加分类项
    QStringList categories = {
        "常规",
        "扫描设置", 
        "图像处理",
        "界面主题",
        "高级选项",
        "关于"
    };
    
    for (const QString &category : categories) {
        QListWidgetItem *item = new QListWidgetItem(category);
        item->setSizeHint(QSize(140, 40));
        m_categoryList->addItem(item);
    }
    
    m_categoryList->setCurrentRow(0);
    
    qDebug() << "分类列表设置完成";
}

void DSettingsDialog::setupSettingsContent()
{
    qDebug() << "设置内容页面";
    
    m_stackedWidget = new DStackedWidget(this);
    
    // 添加各个设置页面
    setupGeneralPage();
    setupScanningPage();
    setupImageProcessingPage();
    setupThemePage();
    setupAdvancedPage();
    setupAboutPage();
    
    qDebug() << "设置内容页面设置完成";
}

void DSettingsDialog::setupGeneralPage()
{
    qDebug() << "设置常规页面";
    
    auto *page = new DScrollArea(this);
    auto *widget = new DWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    // 应用程序设置组
    auto *appGroup = new DGroupBox("应用程序设置", widget);
    auto *appLayout = new QGridLayout(appGroup);
    appLayout->setSpacing(10);
    
    // 启动设置
    appLayout->addWidget(new DLabel("启动选项:", appGroup), 0, 0);
    m_startupModeCombo = new DComboBox(appGroup);
    m_startupModeCombo->addItems({"正常启动", "最小化启动", "记住上次窗口状态"});
    m_startupModeCombo->setToolTip("设置应用程序启动时的窗口状态");
    appLayout->addWidget(m_startupModeCombo, 0, 1, 1, 2);
    
    // 自动保存间隔
    appLayout->addWidget(new DLabel("自动保存间隔:", appGroup), 1, 0);
    m_autoSaveInterval = new DSpinBox(appGroup);
    m_autoSaveInterval->setRange(1, 60);
    m_autoSaveInterval->setValue(5);
    m_autoSaveInterval->setSuffix(" 分钟");
    m_autoSaveInterval->setToolTip("设置配置自动保存的时间间隔");
    appLayout->addWidget(m_autoSaveInterval, 1, 1);
    
    // 最大历史记录
    appLayout->addWidget(new DLabel("最大历史记录:", appGroup), 1, 2);
    m_maxHistoryCount = new DSpinBox(appGroup);
    m_maxHistoryCount->setRange(10, 1000);
    m_maxHistoryCount->setValue(100);
    m_maxHistoryCount->setToolTip("设置最大保存的历史记录数量");
    appLayout->addWidget(m_maxHistoryCount, 1, 3);
    
    // 功能选项
    m_minimizeToTray = new DCheckBox("最小化到系统托盘", appGroup);
    m_minimizeToTray->setToolTip("应用程序最小化时隐藏到系统托盘");
    appLayout->addWidget(m_minimizeToTray, 2, 0, 1, 2);
    
    m_startWithSystem = new DCheckBox("开机自启动", appGroup);
    m_startWithSystem->setToolTip("系统启动时自动启动深度扫描");
    appLayout->addWidget(m_startWithSystem, 2, 2, 1, 2);
    
    m_checkUpdates = new DCheckBox("自动检查更新", appGroup);
    m_checkUpdates->setChecked(true);
    m_checkUpdates->setToolTip("定期检查应用程序更新");
    appLayout->addWidget(m_checkUpdates, 3, 0, 1, 2);
    
    m_sendUsageStats = new DCheckBox("发送使用统计", appGroup);
    m_sendUsageStats->setToolTip("匿名发送使用统计信息帮助改进软件");
    appLayout->addWidget(m_sendUsageStats, 3, 2, 1, 2);
    
    layout->addWidget(appGroup);
    
    // 默认目录设置组
    auto *dirGroup = new DGroupBox("默认目录设置", widget);
    auto *dirLayout = new QGridLayout(dirGroup);
    dirLayout->setSpacing(10);
    
    // 默认输出目录
    dirLayout->addWidget(new DLabel("默认输出目录:", dirGroup), 0, 0);
    m_defaultOutputDir = new DLineEdit(dirGroup);
    m_defaultOutputDir->setPlaceholderText("选择默认的扫描输出目录");
    dirLayout->addWidget(m_defaultOutputDir, 0, 1);
    
    m_browseOutputDir = new DPushButton("浏览", dirGroup);
    dirLayout->addWidget(m_browseOutputDir, 0, 2);
    
    // 配置文件目录
    dirLayout->addWidget(new DLabel("配置文件目录:", dirGroup), 1, 0);
    m_configDir = new DLineEdit(dirGroup);
    m_configDir->setReadOnly(true);
    m_configDir->setText(m_settings->fileName());
    dirLayout->addWidget(m_configDir, 1, 1, 1, 2);
    
    layout->addWidget(dirGroup);
    layout->addStretch();
    
    widget->setLayout(layout);
    page->setWidget(widget);
    m_stackedWidget->addWidget(page);
    
    qDebug() << "常规页面设置完成";
}

void DSettingsDialog::setupScanningPage()
{
    qDebug() << "设置扫描页面";
    
    auto *page = new DScrollArea(this);
    auto *widget = new DWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    // 默认扫描设置组
    auto *scanGroup = new DGroupBox("默认扫描设置", widget);
    auto *scanLayout = new QGridLayout(scanGroup);
    scanLayout->setSpacing(10);
    
    // 默认分辨率
    scanLayout->addWidget(new DLabel("默认分辨率:", scanGroup), 0, 0);
    m_defaultResolution = new DComboBox(scanGroup);
    m_defaultResolution->addItems({"75 DPI", "150 DPI", "300 DPI", "600 DPI", "1200 DPI"});
    m_defaultResolution->setCurrentText("300 DPI");
    scanLayout->addWidget(m_defaultResolution, 0, 1);
    
    // 默认颜色模式
    scanLayout->addWidget(new DLabel("默认颜色模式:", scanGroup), 0, 2);
    m_defaultColorMode = new DComboBox(scanGroup);
    m_defaultColorMode->addItems({"彩色", "灰度", "黑白"});
    scanLayout->addWidget(m_defaultColorMode, 0, 3);
    
    // 默认扫描源
    scanLayout->addWidget(new DLabel("默认扫描源:", scanGroup), 1, 0);
    m_defaultSource = new DComboBox(scanGroup);
    m_defaultSource->addItems({"平板", "自动文档馈送器", "双面扫描"});
    scanLayout->addWidget(m_defaultSource, 1, 1);
    
    // 默认文件格式
    scanLayout->addWidget(new DLabel("默认文件格式:", scanGroup), 1, 2);
    m_defaultFormat = new DComboBox(scanGroup);
    m_defaultFormat->addItems({"JPEG", "PNG", "TIFF", "PDF"});
    scanLayout->addWidget(m_defaultFormat, 1, 3);
    
    layout->addWidget(scanGroup);
    
    // 扫描行为设置组
    auto *behaviorGroup = new DGroupBox("扫描行为设置", widget);
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);
    behaviorLayout->setSpacing(10);
    
    // 扫描选项
    m_autoPreview = new DCheckBox("自动预览扫描", behaviorGroup);
    m_autoPreview->setChecked(true);
    m_autoPreview->setToolTip("扫描完成后自动显示预览");
    behaviorLayout->addWidget(m_autoPreview);
    
    m_autoSave = new DCheckBox("自动保存扫描结果", behaviorGroup);
    m_autoSave->setToolTip("扫描完成后自动保存到默认目录");
    behaviorLayout->addWidget(m_autoSave);
    
    m_showProgress = new DCheckBox("显示扫描进度", behaviorGroup);
    m_showProgress->setChecked(true);
    m_showProgress->setToolTip("扫描时显示进度对话框");
    behaviorLayout->addWidget(m_showProgress);
    
    m_playSound = new DCheckBox("播放完成音效", behaviorGroup);
    m_playSound->setToolTip("扫描完成时播放音效");
    behaviorLayout->addWidget(m_playSound);
    
    layout->addWidget(behaviorGroup);
    
    // 设备设置组
    auto *deviceGroup = new DGroupBox("设备设置", widget);
    auto *deviceLayout = new QGridLayout(deviceGroup);
    deviceLayout->setSpacing(10);
    
    // 设备连接超时
    deviceLayout->addWidget(new DLabel("连接超时:", deviceGroup), 0, 0);
    m_connectionTimeout = new DSpinBox(deviceGroup);
    m_connectionTimeout->setRange(5, 60);
    m_connectionTimeout->setValue(15);
    m_connectionTimeout->setSuffix(" 秒");
    deviceLayout->addWidget(m_connectionTimeout, 0, 1);
    
    // 设备自动发现间隔
    deviceLayout->addWidget(new DLabel("自动发现间隔:", deviceGroup), 0, 2);
    m_discoveryInterval = new DSpinBox(deviceGroup);
    m_discoveryInterval->setRange(10, 300);
    m_discoveryInterval->setValue(30);
    m_discoveryInterval->setSuffix(" 秒");
    deviceLayout->addWidget(m_discoveryInterval, 0, 3);
    
    // 设备选项
    m_autoReconnect = new DCheckBox("自动重连断开的设备", deviceGroup);
    m_autoReconnect->setChecked(true);
    deviceLayout->addWidget(m_autoReconnect, 1, 0, 1, 2);
    
    m_preferUSB = new DCheckBox("优先使用USB连接", deviceGroup);
    m_preferUSB->setChecked(true);
    deviceLayout->addWidget(m_preferUSB, 1, 2, 1, 2);
    
    layout->addWidget(deviceGroup);
    layout->addStretch();
    
    widget->setLayout(layout);
    page->setWidget(widget);
    m_stackedWidget->addWidget(page);
    
    qDebug() << "扫描页面设置完成";
}

void DSettingsDialog::setupImageProcessingPage()
{
    qDebug() << "设置图像处理页面";
    
    auto *page = new DScrollArea(this);
    auto *widget = new DWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    // 默认处理设置组
    auto *processGroup = new DGroupBox("默认处理设置", widget);
    auto *processLayout = new QGridLayout(processGroup);
    processLayout->setSpacing(10);
    
    // 图像质量
    processLayout->addWidget(new DLabel("默认图像质量:", processGroup), 0, 0);
    m_defaultQuality = new DSlider(Qt::Horizontal, processGroup);
    m_defaultQuality->setRange(1, 100);
    m_defaultQuality->setValue(90);
    processLayout->addWidget(m_defaultQuality, 0, 1);
    
    m_qualityLabel = new DLabel("90%", processGroup);
    processLayout->addWidget(m_qualityLabel, 0, 2);
    
    // 压缩级别
    processLayout->addWidget(new DLabel("压缩级别:", processGroup), 1, 0);
    m_compressionLevel = new DComboBox(processGroup);
    m_compressionLevel->addItems({"无压缩", "轻度压缩", "标准压缩", "高度压缩"});
    m_compressionLevel->setCurrentText("标准压缩");
    processLayout->addWidget(m_compressionLevel, 1, 1, 1, 2);
    
    layout->addWidget(processGroup);
    
    // 自动增强选项组
    auto *enhanceGroup = new DGroupBox("自动增强选项", widget);
    auto *enhanceLayout = new QVBoxLayout(enhanceGroup);
    enhanceLayout->setSpacing(10);
    
    m_autoColorCorrection = new DCheckBox("自动色彩校正", enhanceGroup);
    m_autoColorCorrection->setToolTip("自动调整图像的色彩平衡");
    enhanceLayout->addWidget(m_autoColorCorrection);
    
    m_autoContrast = new DCheckBox("自动对比度增强", enhanceGroup);
    m_autoContrast->setToolTip("自动增强图像对比度");
    enhanceLayout->addWidget(m_autoContrast);
    
    m_autoSharpen = new DCheckBox("自动锐化", enhanceGroup);
    m_autoSharpen->setToolTip("自动应用锐化滤镜");
    enhanceLayout->addWidget(m_autoSharpen);
    
    m_noiseReduction = new DCheckBox("噪点消除", enhanceGroup);
    m_noiseReduction->setToolTip("自动消除图像噪点");
    enhanceLayout->addWidget(m_noiseReduction);
    
    layout->addWidget(enhanceGroup);
    
    // 处理性能设置组
    auto *perfGroup = new DGroupBox("处理性能设置", widget);
    auto *perfLayout = new QGridLayout(perfGroup);
    perfLayout->setSpacing(10);
    
    // 处理线程数
    perfLayout->addWidget(new DLabel("处理线程数:", perfGroup), 0, 0);
    m_processingThreads = new DSpinBox(perfGroup);
    m_processingThreads->setRange(1, 8);
    m_processingThreads->setValue(2);
    m_processingThreads->setToolTip("用于图像处理的线程数量");
    perfLayout->addWidget(m_processingThreads, 0, 1);
    
    // 内存缓存大小
    perfLayout->addWidget(new DLabel("内存缓存:", perfGroup), 0, 2);
    m_memoryCacheSize = new DSpinBox(perfGroup);
    m_memoryCacheSize->setRange(64, 2048);
    m_memoryCacheSize->setValue(256);
    m_memoryCacheSize->setSuffix(" MB");
    m_memoryCacheSize->setToolTip("用于图像处理的内存缓存大小");
    perfLayout->addWidget(m_memoryCacheSize, 0, 3);
    
    // 性能选项
    m_useHardwareAccel = new DCheckBox("启用硬件加速", perfGroup);
    m_useHardwareAccel->setToolTip("使用GPU加速图像处理");
    perfLayout->addWidget(m_useHardwareAccel, 1, 0, 1, 2);
    
    m_enablePreview = new DCheckBox("启用实时预览", perfGroup);
    m_enablePreview->setChecked(true);
    m_enablePreview->setToolTip("参数调整时实时显示预览效果");
    perfLayout->addWidget(m_enablePreview, 1, 2, 1, 2);
    
    layout->addWidget(perfGroup);
    layout->addStretch();
    
    widget->setLayout(layout);
    page->setWidget(widget);
    m_stackedWidget->addWidget(page);
    
    qDebug() << "图像处理页面设置完成";
}

void DSettingsDialog::setupThemePage()
{
    qDebug() << "设置主题页面";
    
    auto *page = new DScrollArea(this);
    auto *widget = new DWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    // 外观主题组
    auto *themeGroup = new DGroupBox("外观主题", widget);
    auto *themeLayout = new QGridLayout(themeGroup);
    themeLayout->setSpacing(10);
    
    // 主题选择
    themeLayout->addWidget(new DLabel("主题模式:", themeGroup), 0, 0);
    m_themeMode = new DComboBox(themeGroup);
    m_themeMode->addItems({"自动", "浅色主题", "深色主题"});
    m_themeMode->setToolTip("选择应用程序外观主题");
    themeLayout->addWidget(m_themeMode, 0, 1);
    
    // 强调色
    themeLayout->addWidget(new DLabel("强调色:", themeGroup), 0, 2);
    m_accentColor = new DComboBox(themeGroup);
    m_accentColor->addItems({"系统默认", "蓝色", "绿色", "红色", "紫色", "橙色"});
    themeLayout->addWidget(m_accentColor, 0, 3);
    
    // 字体设置
    themeLayout->addWidget(new DLabel("界面字体:", themeGroup), 1, 0);
    m_interfaceFont = new DComboBox(themeGroup);
    m_interfaceFont->addItems({"系统默认", "思源黑体", "文泉驿微米黑", "Noto Sans CJK"});
    themeLayout->addWidget(m_interfaceFont, 1, 1);
    
    // 字体大小
    themeLayout->addWidget(new DLabel("字体大小:", themeGroup), 1, 2);
    m_fontSize = new DSpinBox(themeGroup);
    m_fontSize->setRange(8, 24);
    m_fontSize->setValue(11);
    m_fontSize->setSuffix(" pt");
    themeLayout->addWidget(m_fontSize, 1, 3);
    
    layout->addWidget(themeGroup);
    
    // 界面设置组
    auto *uiGroup = new DGroupBox("界面设置", widget);
    auto *uiLayout = new QVBoxLayout(uiGroup);
    uiLayout->setSpacing(10);
    
    // 界面选项
    m_showToolbar = new DCheckBox("显示工具栏", uiGroup);
    m_showToolbar->setChecked(true);
    uiLayout->addWidget(m_showToolbar);
    
    m_showStatusbar = new DCheckBox("显示状态栏", uiGroup);
    m_showStatusbar->setChecked(true);
    uiLayout->addWidget(m_showStatusbar);
    
    m_compactMode = new DCheckBox("紧凑模式", uiGroup);
    m_compactMode->setToolTip("使用更紧凑的界面布局");
    uiLayout->addWidget(m_compactMode);
    
    m_animationEffects = new DCheckBox("启用动画效果", uiGroup);
    m_animationEffects->setChecked(true);
    m_animationEffects->setToolTip("启用界面切换和交互动画");
    uiLayout->addWidget(m_animationEffects);
    
    layout->addWidget(uiGroup);
    
    // 窗口设置组
    auto *windowGroup = new DGroupBox("窗口设置", widget);
    auto *windowLayout = new QGridLayout(windowGroup);
    windowLayout->setSpacing(10);
    
    // 默认窗口大小
    windowLayout->addWidget(new DLabel("默认窗口大小:", windowGroup), 0, 0);
    m_defaultWindowSize = new DComboBox(windowGroup);
    m_defaultWindowSize->addItems({"记住上次大小", "1024x768", "1280x800", "1600x900", "1920x1080"});
    windowLayout->addWidget(m_defaultWindowSize, 0, 1, 1, 2);
    
    // 窗口选项
    m_alwaysOnTop = new DCheckBox("窗口总在最前", windowGroup);
    windowLayout->addWidget(m_alwaysOnTop, 1, 0, 1, 2);
    
    m_rememberPosition = new DCheckBox("记住窗口位置", windowGroup);
    m_rememberPosition->setChecked(true);
    windowLayout->addWidget(m_rememberPosition, 1, 2, 1, 2);
    
    layout->addWidget(windowGroup);
    layout->addStretch();
    
    widget->setLayout(layout);
    page->setWidget(widget);
    m_stackedWidget->addWidget(page);
    
    qDebug() << "主题页面设置完成";
}

void DSettingsDialog::setupAdvancedPage()
{
    qDebug() << "设置高级页面";
    
    auto *page = new DScrollArea(this);
    auto *widget = new DWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    // 调试设置组
    auto *debugGroup = new DGroupBox("调试设置", widget);
    auto *debugLayout = new QGridLayout(debugGroup);
    debugLayout->setSpacing(10);
    
    // 日志级别
    debugLayout->addWidget(new DLabel("日志级别:", debugGroup), 0, 0);
    m_logLevel = new DComboBox(debugGroup);
    m_logLevel->addItems({"关闭", "错误", "警告", "信息", "调试"});
    m_logLevel->setCurrentText("信息");
    debugLayout->addWidget(m_logLevel, 0, 1);
    
    // 日志文件大小
    debugLayout->addWidget(new DLabel("日志文件大小:", debugGroup), 0, 2);
    m_logFileSize = new DSpinBox(debugGroup);
    m_logFileSize->setRange(1, 100);
    m_logFileSize->setValue(10);
    m_logFileSize->setSuffix(" MB");
    debugLayout->addWidget(m_logFileSize, 0, 3);
    
    // 调试选项
    m_enableDebugMode = new DCheckBox("启用调试模式", debugGroup);
    m_enableDebugMode->setToolTip("显示调试信息和额外的诊断工具");
    debugLayout->addWidget(m_enableDebugMode, 1, 0, 1, 2);
    
    m_saveDebugLog = new DCheckBox("保存调试日志", debugGroup);
    m_saveDebugLog->setToolTip("将调试信息保存到文件");
    debugLayout->addWidget(m_saveDebugLog, 1, 2, 1, 2);
    
    layout->addWidget(debugGroup);
    
    // 网络设置组
    auto *networkGroup = new DGroupBox("网络设置", widget);
    auto *networkLayout = new QGridLayout(networkGroup);
    networkLayout->setSpacing(10);
    
    // 网络超时
    networkLayout->addWidget(new DLabel("网络超时:", networkGroup), 0, 0);
    m_networkTimeout = new DSpinBox(networkGroup);
    m_networkTimeout->setRange(5, 120);
    m_networkTimeout->setValue(30);
    m_networkTimeout->setSuffix(" 秒");
    networkLayout->addWidget(m_networkTimeout, 0, 1);
    
    // 重试次数
    networkLayout->addWidget(new DLabel("重试次数:", networkGroup), 0, 2);
    m_retryCount = new DSpinBox(networkGroup);
    m_retryCount->setRange(0, 10);
    m_retryCount->setValue(3);
    networkLayout->addWidget(m_retryCount, 0, 3);
    
    // 网络选项
    m_useProxy = new DCheckBox("使用代理服务器", networkGroup);
    networkLayout->addWidget(m_useProxy, 1, 0, 1, 2);
    
    m_offlineMode = new DCheckBox("离线模式", networkGroup);
    m_offlineMode->setToolTip("禁用所有网络功能");
    networkLayout->addWidget(m_offlineMode, 1, 2, 1, 2);
    
    layout->addWidget(networkGroup);
    
    // 实验性功能组
    auto *experimentalGroup = new DGroupBox("实验性功能", widget);
    auto *experimentalLayout = new QVBoxLayout(experimentalGroup);
    experimentalLayout->setSpacing(10);
    
    // 警告标签
    auto *warningLabel = new DLabel("⚠️ 以下功能仍在开发中，可能不稳定", experimentalGroup);
    warningLabel->setStyleSheet("color: orange; font-weight: bold;");
    experimentalLayout->addWidget(warningLabel);
    
    // 实验性选项
    m_enableAI = new DCheckBox("启用AI图像增强", experimentalGroup);
    m_enableAI->setToolTip("使用人工智能技术自动增强扫描图像");
    experimentalLayout->addWidget(m_enableAI);
    
    m_enableOCR = new DCheckBox("启用OCR文字识别", experimentalGroup);
    m_enableOCR->setToolTip("自动识别扫描图像中的文字");
    experimentalLayout->addWidget(m_enableOCR);
    
    m_enableCloud = new DCheckBox("启用云同步", experimentalGroup);
    m_enableCloud->setToolTip("将扫描结果同步到云存储");
    experimentalLayout->addWidget(m_enableCloud);
    
    layout->addWidget(experimentalGroup);
    
    // 操作按钮组
    auto *actionGroup = new DGroupBox("操作", widget);
    auto *actionLayout = new QHBoxLayout(actionGroup);
    actionLayout->setSpacing(10);
    
    m_resetSettingsButton = new DPushButton("重置所有设置", actionGroup);
    m_resetSettingsButton->setToolTip("将所有设置重置为默认值");
    actionLayout->addWidget(m_resetSettingsButton);
    
    m_exportSettingsButton = new DPushButton("导出设置", actionGroup);
    m_exportSettingsButton->setToolTip("将当前设置导出为文件");
    actionLayout->addWidget(m_exportSettingsButton);
    
    m_importSettingsButton = new DPushButton("导入设置", actionGroup);
    m_importSettingsButton->setToolTip("从文件导入设置");
    actionLayout->addWidget(m_importSettingsButton);
    
    actionLayout->addStretch();
    layout->addWidget(actionGroup);
    layout->addStretch();
    
    widget->setLayout(layout);
    page->setWidget(widget);
    m_stackedWidget->addWidget(page);
    
    qDebug() << "高级页面设置完成";
}

void DSettingsDialog::setupAboutPage()
{
    qDebug() << "设置关于页面";
    
    auto *page = new DScrollArea(this);
    auto *widget = new DWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);
    
    // 应用程序信息
    auto *appInfo = new DFrame(widget);
    appInfo->setFrameStyle(QFrame::Box);
    auto *appLayout = new QVBoxLayout(appInfo);
    appLayout->setSpacing(10);
    
    // 应用程序图标和名称
    auto *titleLayout = new QHBoxLayout();
    auto *iconLabel = new DLabel(appInfo);
    iconLabel->setFixedSize(64, 64);
    iconLabel->setStyleSheet("border: 1px solid gray; background-color: lightblue;");
    iconLabel->setText("📷"); // 临时图标
    iconLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(iconLabel);
    
    auto *titleInfo = new QVBoxLayout();
    auto *appName = new DLabel("深度扫描 (DeepinScan)", appInfo);
    appName->setStyleSheet("font-size: 18px; font-weight: bold;");
    titleInfo->addWidget(appName);
    
    auto *version = new DLabel("版本 1.0.0", appInfo);
    titleInfo->addWidget(version);
    
    auto *description = new DLabel("现代化的文档和图像扫描应用程序", appInfo);
    titleInfo->addWidget(description);
    
    titleLayout->addLayout(titleInfo);
    titleLayout->addStretch();
    appLayout->addLayout(titleLayout);
    
    // 版权信息
    auto *copyright = new DLabel("© 2024-2025 eric2023", appInfo);
    copyright->setStyleSheet("color: gray;");
    appLayout->addWidget(copyright);
    
    layout->addWidget(appInfo);
    
    // 系统信息
    auto *sysGroup = new DGroupBox("系统信息", widget);
    auto *sysLayout = new QGridLayout(sysGroup);
    sysLayout->setSpacing(8);
    
    sysLayout->addWidget(new DLabel("操作系统:", sysGroup), 0, 0);
    sysLayout->addWidget(new DLabel("Deepin 20.9", sysGroup), 0, 1);
    
    sysLayout->addWidget(new DLabel("Qt版本:", sysGroup), 1, 0);
    sysLayout->addWidget(new DLabel(qVersion(), sysGroup), 1, 1);
    
    sysLayout->addWidget(new DLabel("DTK版本:", sysGroup), 2, 0);
    sysLayout->addWidget(new DLabel("5.6.12", sysGroup), 2, 1);
    
    sysLayout->addWidget(new DLabel("SANE版本:", sysGroup), 3, 0);
    sysLayout->addWidget(new DLabel("1.0.32", sysGroup), 3, 1);
    
    layout->addWidget(sysGroup);
    
    // 许可证信息
    auto *licenseGroup = new DGroupBox("许可证", widget);
    auto *licenseLayout = new QVBoxLayout(licenseGroup);
    
    auto *licenseText = new DLabel(
        "本软件基于 GPL-3.0-or-later 许可证发布。\n"
        "您可以自由使用、修改和分发本软件。\n"
        "详细许可证条款请参阅 LICENSE 文件。", licenseGroup);
    licenseText->setWordWrap(true);
    licenseLayout->addWidget(licenseText);
    
    layout->addWidget(licenseGroup);
    layout->addStretch();
    
    widget->setLayout(layout);
    page->setWidget(widget);
    m_stackedWidget->addWidget(page);
    
    qDebug() << "关于页面设置完成";
}

void DSettingsDialog::connectSignals()
{
    qDebug() << "连接设置对话框信号";
    
    // 分类列表信号
    connect(m_categoryList, &DListWidget::currentRowChanged,
            m_stackedWidget, &DStackedWidget::setCurrentIndex);
    
    // 图像质量滑块信号
    connect(m_defaultQuality, &DSlider::valueChanged, [this](int value) {
        m_qualityLabel->setText(QString("%1%").arg(value));
    });
    
    // 对话框按钮信号
    connect(this, &DDialog::buttonClicked, this, &DSettingsDialog::onButtonClicked);
    
    // 高级页面按钮信号
    connect(m_resetSettingsButton, &DPushButton::clicked,
            this, &DSettingsDialog::resetSettings);
    connect(m_exportSettingsButton, &DPushButton::clicked,
            this, &DSettingsDialog::exportSettings);
    connect(m_importSettingsButton, &DPushButton::clicked,
            this, &DSettingsDialog::importSettings);
    
    qDebug() << "设置对话框信号连接完成";
}

void DSettingsDialog::loadSettings()
{
    qDebug() << "加载设置";
    
    // 加载常规设置
    m_startupModeCombo->setCurrentText(m_settings->value("General/StartupMode", "正常启动").toString());
    m_autoSaveInterval->setValue(m_settings->value("General/AutoSaveInterval", 5).toInt());
    m_maxHistoryCount->setValue(m_settings->value("General/MaxHistoryCount", 100).toInt());
    m_minimizeToTray->setChecked(m_settings->value("General/MinimizeToTray", false).toBool());
    m_startWithSystem->setChecked(m_settings->value("General/StartWithSystem", false).toBool());
    m_checkUpdates->setChecked(m_settings->value("General/CheckUpdates", true).toBool());
    m_sendUsageStats->setChecked(m_settings->value("General/SendUsageStats", false).toBool());
    m_defaultOutputDir->setText(m_settings->value("General/DefaultOutputDir", 
                                QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).toString());
    
    // 加载扫描设置
    m_defaultResolution->setCurrentText(m_settings->value("Scanning/DefaultResolution", "300 DPI").toString());
    m_defaultColorMode->setCurrentText(m_settings->value("Scanning/DefaultColorMode", "彩色").toString());
    m_defaultSource->setCurrentText(m_settings->value("Scanning/DefaultSource", "平板").toString());
    m_defaultFormat->setCurrentText(m_settings->value("Scanning/DefaultFormat", "JPEG").toString());
    m_autoPreview->setChecked(m_settings->value("Scanning/AutoPreview", true).toBool());
    m_autoSave->setChecked(m_settings->value("Scanning/AutoSave", false).toBool());
    m_showProgress->setChecked(m_settings->value("Scanning/ShowProgress", true).toBool());
    m_playSound->setChecked(m_settings->value("Scanning/PlaySound", false).toBool());
    m_connectionTimeout->setValue(m_settings->value("Scanning/ConnectionTimeout", 15).toInt());
    m_discoveryInterval->setValue(m_settings->value("Scanning/DiscoveryInterval", 30).toInt());
    m_autoReconnect->setChecked(m_settings->value("Scanning/AutoReconnect", true).toBool());
    m_preferUSB->setChecked(m_settings->value("Scanning/PreferUSB", true).toBool());
    
    // 加载图像处理设置
    m_defaultQuality->setValue(m_settings->value("ImageProcessing/DefaultQuality", 90).toInt());
    m_compressionLevel->setCurrentText(m_settings->value("ImageProcessing/CompressionLevel", "标准压缩").toString());
    m_autoColorCorrection->setChecked(m_settings->value("ImageProcessing/AutoColorCorrection", false).toBool());
    m_autoContrast->setChecked(m_settings->value("ImageProcessing/AutoContrast", false).toBool());
    m_autoSharpen->setChecked(m_settings->value("ImageProcessing/AutoSharpen", false).toBool());
    m_noiseReduction->setChecked(m_settings->value("ImageProcessing/NoiseReduction", false).toBool());
    m_processingThreads->setValue(m_settings->value("ImageProcessing/ProcessingThreads", 2).toInt());
    m_memoryCacheSize->setValue(m_settings->value("ImageProcessing/MemoryCacheSize", 256).toInt());
    m_useHardwareAccel->setChecked(m_settings->value("ImageProcessing/UseHardwareAccel", false).toBool());
    m_enablePreview->setChecked(m_settings->value("ImageProcessing/EnablePreview", true).toBool());
    
    // 加载主题设置
    m_themeMode->setCurrentText(m_settings->value("Theme/ThemeMode", "自动").toString());
    m_accentColor->setCurrentText(m_settings->value("Theme/AccentColor", "系统默认").toString());
    m_interfaceFont->setCurrentText(m_settings->value("Theme/InterfaceFont", "系统默认").toString());
    m_fontSize->setValue(m_settings->value("Theme/FontSize", 11).toInt());
    m_showToolbar->setChecked(m_settings->value("Theme/ShowToolbar", true).toBool());
    m_showStatusbar->setChecked(m_settings->value("Theme/ShowStatusbar", true).toBool());
    m_compactMode->setChecked(m_settings->value("Theme/CompactMode", false).toBool());
    m_animationEffects->setChecked(m_settings->value("Theme/AnimationEffects", true).toBool());
    m_defaultWindowSize->setCurrentText(m_settings->value("Theme/DefaultWindowSize", "记住上次大小").toString());
    m_alwaysOnTop->setChecked(m_settings->value("Theme/AlwaysOnTop", false).toBool());
    m_rememberPosition->setChecked(m_settings->value("Theme/RememberPosition", true).toBool());
    
    // 加载高级设置
    m_logLevel->setCurrentText(m_settings->value("Advanced/LogLevel", "信息").toString());
    m_logFileSize->setValue(m_settings->value("Advanced/LogFileSize", 10).toInt());
    m_enableDebugMode->setChecked(m_settings->value("Advanced/EnableDebugMode", false).toBool());
    m_saveDebugLog->setChecked(m_settings->value("Advanced/SaveDebugLog", false).toBool());
    m_networkTimeout->setValue(m_settings->value("Advanced/NetworkTimeout", 30).toInt());
    m_retryCount->setValue(m_settings->value("Advanced/RetryCount", 3).toInt());
    m_useProxy->setChecked(m_settings->value("Advanced/UseProxy", false).toBool());
    m_offlineMode->setChecked(m_settings->value("Advanced/OfflineMode", false).toBool());
    m_enableAI->setChecked(m_settings->value("Advanced/EnableAI", false).toBool());
    m_enableOCR->setChecked(m_settings->value("Advanced/EnableOCR", false).toBool());
    m_enableCloud->setChecked(m_settings->value("Advanced/EnableCloud", false).toBool());
    
    qDebug() << "设置加载完成";
}

void DSettingsDialog::saveSettings()
{
    qDebug() << "保存设置";
    
    // 保存常规设置
    m_settings->setValue("General/StartupMode", m_startupModeCombo->currentText());
    m_settings->setValue("General/AutoSaveInterval", m_autoSaveInterval->value());
    m_settings->setValue("General/MaxHistoryCount", m_maxHistoryCount->value());
    m_settings->setValue("General/MinimizeToTray", m_minimizeToTray->isChecked());
    m_settings->setValue("General/StartWithSystem", m_startWithSystem->isChecked());
    m_settings->setValue("General/CheckUpdates", m_checkUpdates->isChecked());
    m_settings->setValue("General/SendUsageStats", m_sendUsageStats->isChecked());
    m_settings->setValue("General/DefaultOutputDir", m_defaultOutputDir->text());
    
    // 保存扫描设置
    m_settings->setValue("Scanning/DefaultResolution", m_defaultResolution->currentText());
    m_settings->setValue("Scanning/DefaultColorMode", m_defaultColorMode->currentText());
    m_settings->setValue("Scanning/DefaultSource", m_defaultSource->currentText());
    m_settings->setValue("Scanning/DefaultFormat", m_defaultFormat->currentText());
    m_settings->setValue("Scanning/AutoPreview", m_autoPreview->isChecked());
    m_settings->setValue("Scanning/AutoSave", m_autoSave->isChecked());
    m_settings->setValue("Scanning/ShowProgress", m_showProgress->isChecked());
    m_settings->setValue("Scanning/PlaySound", m_playSound->isChecked());
    m_settings->setValue("Scanning/ConnectionTimeout", m_connectionTimeout->value());
    m_settings->setValue("Scanning/DiscoveryInterval", m_discoveryInterval->value());
    m_settings->setValue("Scanning/AutoReconnect", m_autoReconnect->isChecked());
    m_settings->setValue("Scanning/PreferUSB", m_preferUSB->isChecked());
    
    // 保存图像处理设置
    m_settings->setValue("ImageProcessing/DefaultQuality", m_defaultQuality->value());
    m_settings->setValue("ImageProcessing/CompressionLevel", m_compressionLevel->currentText());
    m_settings->setValue("ImageProcessing/AutoColorCorrection", m_autoColorCorrection->isChecked());
    m_settings->setValue("ImageProcessing/AutoContrast", m_autoContrast->isChecked());
    m_settings->setValue("ImageProcessing/AutoSharpen", m_autoSharpen->isChecked());
    m_settings->setValue("ImageProcessing/NoiseReduction", m_noiseReduction->isChecked());
    m_settings->setValue("ImageProcessing/ProcessingThreads", m_processingThreads->value());
    m_settings->setValue("ImageProcessing/MemoryCacheSize", m_memoryCacheSize->value());
    m_settings->setValue("ImageProcessing/UseHardwareAccel", m_useHardwareAccel->isChecked());
    m_settings->setValue("ImageProcessing/EnablePreview", m_enablePreview->isChecked());
    
    // 保存主题设置
    m_settings->setValue("Theme/ThemeMode", m_themeMode->currentText());
    m_settings->setValue("Theme/AccentColor", m_accentColor->currentText());
    m_settings->setValue("Theme/InterfaceFont", m_interfaceFont->currentText());
    m_settings->setValue("Theme/FontSize", m_fontSize->value());
    m_settings->setValue("Theme/ShowToolbar", m_showToolbar->isChecked());
    m_settings->setValue("Theme/ShowStatusbar", m_showStatusbar->isChecked());
    m_settings->setValue("Theme/CompactMode", m_compactMode->isChecked());
    m_settings->setValue("Theme/AnimationEffects", m_animationEffects->isChecked());
    m_settings->setValue("Theme/DefaultWindowSize", m_defaultWindowSize->currentText());
    m_settings->setValue("Theme/AlwaysOnTop", m_alwaysOnTop->isChecked());
    m_settings->setValue("Theme/RememberPosition", m_rememberPosition->isChecked());
    
    // 保存高级设置
    m_settings->setValue("Advanced/LogLevel", m_logLevel->currentText());
    m_settings->setValue("Advanced/LogFileSize", m_logFileSize->value());
    m_settings->setValue("Advanced/EnableDebugMode", m_enableDebugMode->isChecked());
    m_settings->setValue("Advanced/SaveDebugLog", m_saveDebugLog->isChecked());
    m_settings->setValue("Advanced/NetworkTimeout", m_networkTimeout->value());
    m_settings->setValue("Advanced/RetryCount", m_retryCount->value());
    m_settings->setValue("Advanced/UseProxy", m_useProxy->isChecked());
    m_settings->setValue("Advanced/OfflineMode", m_offlineMode->isChecked());
    m_settings->setValue("Advanced/EnableAI", m_enableAI->isChecked());
    m_settings->setValue("Advanced/EnableOCR", m_enableOCR->isChecked());
    m_settings->setValue("Advanced/EnableCloud", m_enableCloud->isChecked());
    
    m_settings->sync();
    
    qDebug() << "设置保存完成";
    
    emit settingsChanged();
}

void DSettingsDialog::resetSettings()
{
    qDebug() << "重置所有设置";
    
    int ret = DMessageBox::question(this, "确认重置", 
                                   "确定要将所有设置重置为默认值吗？此操作不可撤销。",
                                   DMessageBox::Yes | DMessageBox::No);
    if (ret == DMessageBox::Yes) {
        m_settings->clear();
        loadSettings();
        qDebug() << "设置已重置为默认值";
    }
}

void DSettingsDialog::exportSettings()
{
    qDebug() << "DSettingsDialog::exportSettings: 开始导出设置";
    
    // 选择导出文件
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("导出设置"),
        QDir::homePath() + "/deepinscan_settings_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json",
        tr("JSON文件 (*.json);;配置文件 (*.conf);;所有文件 (*)")
    );
    
    if (fileName.isEmpty()) {
        qDebug() << "DSettingsDialog::exportSettings: 用户取消了导出操作";
        return;
    }
    
    try {
        // 收集所有设置
        QJsonObject settingsObject;
        
        // 扫描设置
        QJsonObject scanSettings;
        scanSettings["default_resolution"] = m_resolutionSpinBox->value();
        scanSettings["default_color_mode"] = m_colorModeCombo->currentText();
        scanSettings["default_format"] = m_formatCombo->currentText();
        scanSettings["auto_crop"] = m_autoCropCheckBox->isChecked();
        scanSettings["auto_deskew"] = m_autoDeskewCheckBox->isChecked();
        scanSettings["auto_brightness"] = m_autoBrightnessCheckBox->isChecked();
        settingsObject["scan"] = scanSettings;
        
        // 界面设置
        QJsonObject uiSettings;
        uiSettings["language"] = m_languageCombo->currentText();
        uiSettings["theme"] = m_themeCombo->currentText();
        uiSettings["show_toolbar"] = m_showToolbarCheckBox->isChecked();
        uiSettings["show_statusbar"] = m_showStatusbarCheckBox->isChecked();
        uiSettings["auto_preview"] = m_autoPreviewCheckBox->isChecked();
        settingsObject["ui"] = uiSettings;
        
        // 设备设置
        QJsonObject deviceSettings;
        deviceSettings["auto_detect"] = m_autoDetectCheckBox->isChecked();
        deviceSettings["device_timeout"] = m_deviceTimeoutSpinBox->value();
        deviceSettings["retry_count"] = m_retryCountSpinBox->value();
        settingsObject["device"] = deviceSettings;
        
        // 高级设置
        QJsonObject advancedSettings;
        advancedSettings["enable_debug"] = m_enableDebugCheckBox->isChecked();
        advancedSettings["log_level"] = m_logLevelCombo->currentText();
        advancedSettings["cache_size"] = m_cacheSizeSpinBox->value();
        advancedSettings["thread_count"] = m_threadCountSpinBox->value();
        settingsObject["advanced"] = advancedSettings;
        
        // 添加元数据
        QJsonObject metadata;
        metadata["version"] = "1.0.0";
        metadata["export_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        metadata["application"] = "DeepinScan";
        settingsObject["metadata"] = metadata;
        
        // 写入文件
        QJsonDocument doc(settingsObject);
        QFile file(fileName);
        
        if (!file.open(QIODevice::WriteOnly)) {
            throw QString(tr("无法创建文件: %1").arg(file.errorString()));
        }
        
        qint64 written = file.write(doc.toJson());
        if (written == -1) {
            throw QString(tr("写入文件失败: %1").arg(file.errorString()));
        }
        
        file.close();
        
        // 显示成功消息
        QMessageBox::information(this, tr("导出成功"), 
                                 tr("设置已成功导出到:\n%1").arg(fileName));
        
        qDebug() << "DSettingsDialog::exportSettings: 设置导出成功，文件:" << fileName;
        
    } catch (const QString &error) {
        qWarning() << "DSettingsDialog::exportSettings: 导出失败:" << error;
        QMessageBox::critical(this, tr("导出失败"), error);
    }
}

void DSettingsDialog::importSettings()
{
    qDebug() << "DSettingsDialog::importSettings: 开始导入设置";
    
    // 选择导入文件
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("导入设置"),
        QDir::homePath(),
        tr("JSON文件 (*.json);;配置文件 (*.conf);;所有文件 (*)")
    );
    
    if (fileName.isEmpty()) {
        qDebug() << "DSettingsDialog::importSettings: 用户取消了导入操作";
        return;
    }
    
    try {
        // 读取文件
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            throw QString(tr("无法打开文件: %1").arg(file.errorString()));
        }
        
        QByteArray data = file.readAll();
        file.close();
        
        // 解析JSON
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            throw QString(tr("JSON格式错误: %1").arg(error.errorString()));
        }
        
        QJsonObject settingsObject = doc.object();
        
        // 验证文件格式
        if (!settingsObject.contains("metadata")) {
            throw QString(tr("不是有效的设置文件"));
        }
        
        QJsonObject metadata = settingsObject["metadata"].toObject();
        QString version = metadata["version"].toString();
        QString application = metadata["application"].toString();
        
        if (application != "DeepinScan") {
            if (QMessageBox::question(this, tr("兼容性警告"), 
                                      tr("此设置文件来自其他应用程序(%1)，是否继续导入？").arg(application),
                                      QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
                return;
            }
        }
        
        // 确认导入
        if (QMessageBox::question(this, tr("确认导入"), 
                                  tr("导入设置将覆盖当前配置，是否继续？"),
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        
        // 导入扫描设置
        if (settingsObject.contains("scan")) {
            QJsonObject scanSettings = settingsObject["scan"].toObject();
            
            if (scanSettings.contains("default_resolution")) {
                m_resolutionSpinBox->setValue(scanSettings["default_resolution"].toInt());
            }
            if (scanSettings.contains("default_color_mode")) {
                QString colorMode = scanSettings["default_color_mode"].toString();
                int index = m_colorModeCombo->findText(colorMode);
                if (index >= 0) m_colorModeCombo->setCurrentIndex(index);
            }
            if (scanSettings.contains("default_format")) {
                QString format = scanSettings["default_format"].toString();
                int index = m_formatCombo->findText(format);
                if (index >= 0) m_formatCombo->setCurrentIndex(index);
            }
            if (scanSettings.contains("auto_crop")) {
                m_autoCropCheckBox->setChecked(scanSettings["auto_crop"].toBool());
            }
            if (scanSettings.contains("auto_deskew")) {
                m_autoDeskewCheckBox->setChecked(scanSettings["auto_deskew"].toBool());
            }
            if (scanSettings.contains("auto_brightness")) {
                m_autoBrightnessCheckBox->setChecked(scanSettings["auto_brightness"].toBool());
            }
        }
        
        // 导入界面设置
        if (settingsObject.contains("ui")) {
            QJsonObject uiSettings = settingsObject["ui"].toObject();
            
            if (uiSettings.contains("language")) {
                QString language = uiSettings["language"].toString();
                int index = m_languageCombo->findText(language);
                if (index >= 0) m_languageCombo->setCurrentIndex(index);
            }
            if (uiSettings.contains("theme")) {
                QString theme = uiSettings["theme"].toString();
                int index = m_themeCombo->findText(theme);
                if (index >= 0) m_themeCombo->setCurrentIndex(index);
            }
            if (uiSettings.contains("show_toolbar")) {
                m_showToolbarCheckBox->setChecked(uiSettings["show_toolbar"].toBool());
            }
            if (uiSettings.contains("show_statusbar")) {
                m_showStatusbarCheckBox->setChecked(uiSettings["show_statusbar"].toBool());
            }
            if (uiSettings.contains("auto_preview")) {
                m_autoPreviewCheckBox->setChecked(uiSettings["auto_preview"].toBool());
            }
        }
        
        // 导入设备设置
        if (settingsObject.contains("device")) {
            QJsonObject deviceSettings = settingsObject["device"].toObject();
            
            if (deviceSettings.contains("auto_detect")) {
                m_autoDetectCheckBox->setChecked(deviceSettings["auto_detect"].toBool());
            }
            if (deviceSettings.contains("device_timeout")) {
                m_deviceTimeoutSpinBox->setValue(deviceSettings["device_timeout"].toInt());
            }
            if (deviceSettings.contains("retry_count")) {
                m_retryCountSpinBox->setValue(deviceSettings["retry_count"].toInt());
            }
        }
        
        // 导入高级设置
        if (settingsObject.contains("advanced")) {
            QJsonObject advancedSettings = settingsObject["advanced"].toObject();
            
            if (advancedSettings.contains("enable_debug")) {
                m_enableDebugCheckBox->setChecked(advancedSettings["enable_debug"].toBool());
            }
            if (advancedSettings.contains("log_level")) {
                QString logLevel = advancedSettings["log_level"].toString();
                int index = m_logLevelCombo->findText(logLevel);
                if (index >= 0) m_logLevelCombo->setCurrentIndex(index);
            }
            if (advancedSettings.contains("cache_size")) {
                m_cacheSizeSpinBox->setValue(advancedSettings["cache_size"].toInt());
            }
            if (advancedSettings.contains("thread_count")) {
                m_threadCountSpinBox->setValue(advancedSettings["thread_count"].toInt());
            }
        }
        
        // 显示成功消息
        QMessageBox::information(this, tr("导入成功"), 
                                 tr("设置已成功导入，请点击\"应用\"或\"确定\"保存更改"));
        
        qDebug() << "DSettingsDialog::importSettings: 设置导入成功，文件:" << fileName;
        
    } catch (const QString &error) {
        qWarning() << "DSettingsDialog::importSettings: 导入失败:" << error;
        QMessageBox::critical(this, tr("导入失败"), error);
    }
}

void DSettingsDialog::onButtonClicked(int index, const QString &text)
{
    qDebug() << QString("对话框按钮点击: %1 (%2)").arg(text).arg(index);
    
    if (text == "确定") {
        saveSettings();
        accept();
    } else if (text == "应用") {
        saveSettings();
    } else if (text == "取消") {
        reject();
    }
} 