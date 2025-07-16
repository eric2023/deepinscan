// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"
#include "mainwindow.h"
#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerImageProcessor.h"
#include "Scanner/DScannerNetworkDiscovery.h"

// 现代化扫描应用的核心组件
#include "../drivers/sane/sane_api_complete.h"
#include "../drivers/vendors/genesys/genesys_driver_complete.h"
#include "../drivers/vendors/canon/canon_driver_complete.h"
#include "../drivers/vendors/epson/epson_driver_complete.h"
#include "../communication/network/network_complete_discovery.h"

#include <DApplication>
#include <DWidgetUtil>
#include <DLogManager>
#include <DMainWindow>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QDir>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QThread>
#include <QMutex>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DSCANNER_USE_NAMESPACE

Q_LOGGING_CATEGORY(dscannerApp, "deepinscan.application")

// 现代化扫描应用的全局组件实例
static DScannerManager *g_scannerManager = nullptr;
static DScannerImageProcessor *g_imageProcessor = nullptr;
static DScannerNetworkDiscovery *g_networkDiscovery = nullptr;
static SANEAPIManager *g_saneManager = nullptr;
static NetworkCompleteDiscovery *g_networkCompleteDiscovery = nullptr;
static QMutex g_componentMutex;

Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
    , m_mainWindow(nullptr)
    , m_splashScreen(nullptr)
    , m_systemTrayIcon(nullptr)
    , m_initialized(false)
    , m_componentsInitialized(false)
{
    qCDebug(dscannerApp) << "Application 构造函数开始";
    
    // 设置应用程序信息 - 现代化应用信息架构
    setOrganizationName("eric2023");
    setOrganizationDomain("github.com");
    setApplicationName("DeepinScan");
    setApplicationDisplayName("深度扫描");
    setApplicationVersion("1.0.0");
    setApplicationDescription("现代化深度系统扫描应用程序，支持6000+设备兼容性");
    
    // 设置应用程序图标
    setWindowIcon(QIcon::fromTheme("scanner", QIcon(":/icons/scanner.svg")));
    
    // 启用高DPI支持
    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setAttribute(Qt::AA_EnableHighDpiScaling);
    
    qCDebug(dscannerApp) << "Application 基础设置完成";
}

Application::~Application()
{
    qCDebug(dscannerApp) << "Application 析构函数开始";
    
    cleanup();
    
    qCDebug(dscannerApp) << "Application 析构完成";
}

bool Application::initialize()
{
    qCDebug(dscannerApp) << "初始化应用程序";
    
    if (m_initialized) {
        qCWarning(dscannerApp) << "应用程序已经初始化";
        return true;
    }
    
    // 显示启动画面 - 现代化的启动体验
    showSplashScreen();
    
    // 设置语言和本地化
    if (!setupLocalization()) {
        qCCritical(dscannerApp) << "本地化设置失败";
        return false;
    }
    updateSplashMessage("正在设置本地化...");
    
    // 初始化日志系统
    if (!setupLogging()) {
        qCCritical(dscannerApp) << "日志系统初始化失败";
        return false;
    }
    updateSplashMessage("正在初始化日志系统...");
    
    // 初始化核心组件 - 现代化的组件架构
    if (!initializeCoreComponents()) {
        qCCritical(dscannerApp) << "核心组件初始化失败";
        return false;
    }
    updateSplashMessage("正在初始化核心组件...");
    
    // 初始化扫描仪管理器
    if (!initializeScannerManager()) {
        qCCritical(dscannerApp) << "扫描仪管理器初始化失败";
        return false;
    }
    updateSplashMessage("正在初始化扫描仪管理器...");
    
    // 初始化图像处理器
    if (!initializeImageProcessor()) {
        qCCritical(dscannerApp) << "图像处理器初始化失败";
        return false;
    }
    updateSplashMessage("正在初始化图像处理器...");
    
    // 初始化网络发现
    if (!initializeNetworkDiscovery()) {
        qCCritical(dscannerApp) << "网络发现初始化失败";
        return false;
    }
    updateSplashMessage("正在初始化网络发现...");
    
    // 初始化系统托盘
    if (!initializeSystemTray()) {
        qCWarning(dscannerApp) << "系统托盘初始化失败，继续运行";
    }
    updateSplashMessage("正在初始化系统托盘...");
    
    // 创建主窗口
    if (!createMainWindow()) {
        qCCritical(dscannerApp) << "主窗口创建失败";
        return false;
    }
    updateSplashMessage("正在创建主窗口...");
    
    // 连接信号和槽
    connectSignalsAndSlots();
    updateSplashMessage("正在连接信号和槽...");
    
    m_initialized = true;
    
    qCDebug(dscannerApp) << "应用程序初始化成功";
    return true;
}

void Application::cleanup()
{
    qCDebug(dscannerApp) << "清理应用程序资源";
    
    // 清理主窗口
    if (m_mainWindow) {
        m_mainWindow->close();
        m_mainWindow->deleteLater();
        m_mainWindow = nullptr;
    }
    
    // 清理系统托盘
    if (m_systemTrayIcon) {
        m_systemTrayIcon->hide();
        m_systemTrayIcon->deleteLater();
        m_systemTrayIcon = nullptr;
    }
    
    // 清理启动画面
    if (m_splashScreen) {
        m_splashScreen->close();
        m_splashScreen->deleteLater();
        m_splashScreen = nullptr;
    }
    
    // 清理核心组件 - 现代化的清理架构
    cleanupCoreComponents();
    
    m_initialized = false;
    m_componentsInitialized = false;
}

void Application::showMainWindow()
{
    if (m_mainWindow) {
        // 隐藏启动画面
        if (m_splashScreen) {
            m_splashScreen->finish(m_mainWindow);
            m_splashScreen->deleteLater();
            m_splashScreen = nullptr;
        }
        
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
        
        // 移动到屏幕中央
        Dtk::Widget::moveToCenter(m_mainWindow);
        
        qCDebug(dscannerApp) << "主窗口已显示";
    }
}

void Application::hideMainWindow()
{
    if (m_mainWindow) {
        m_mainWindow->hide();
        qCDebug(dscannerApp) << "主窗口已隐藏";
    }
}

DScannerManager* Application::scannerManager() const
{
    QMutexLocker locker(&g_componentMutex);
    return g_scannerManager;
}

DScannerImageProcessor* Application::imageProcessor() const
{
    QMutexLocker locker(&g_componentMutex);
    return g_imageProcessor;
}

DScannerNetworkDiscovery* Application::networkDiscovery() const
{
    QMutexLocker locker(&g_componentMutex);
    return g_networkDiscovery;
}

MainWindow* Application::mainWindow() const
{
    return m_mainWindow;
}

// 私有方法实现

void Application::showSplashScreen()
{
    qCDebug(dscannerApp) << "显示启动画面";
    
    // 创建启动画面 - 现代化的启动体验设计
    QPixmap splashPixmap(":/images/splash.png");
    if (splashPixmap.isNull()) {
        // 如果没有启动图片，创建一个简单的启动画面
        splashPixmap = QPixmap(400, 300);
        splashPixmap.fill(QColor(41, 128, 185)); // 深蓝色背景
    }
    
    m_splashScreen = new QSplashScreen(splashPixmap);
    m_splashScreen->setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
    
    // 设置启动消息样式
    m_splashScreen->showMessage("正在启动深度扫描...", 
                               Qt::AlignHCenter | Qt::AlignBottom, 
                               Qt::white);
    
    m_splashScreen->show();
    processEvents(); // 确保启动画面显示
}

void Application::updateSplashMessage(const QString &message)
{
    if (m_splashScreen) {
        m_splashScreen->showMessage(message, 
                                   Qt::AlignHCenter | Qt::AlignBottom, 
                                   Qt::white);
        processEvents();
        QThread::msleep(200); // 短暂延迟，让用户看到消息
    }
}

bool Application::setupLocalization()
{
    qCDebug(dscannerApp) << "设置本地化";
    
    // 加载Qt翻译文件
    QTranslator *qtTranslator = new QTranslator(this);
    if (qtTranslator->load("qt_" + QLocale::system().name(),
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        installTranslator(qtTranslator);
        qCDebug(dscannerApp) << "Qt翻译文件加载成功";
    } else {
        delete qtTranslator;
        qCWarning(dscannerApp) << "Qt翻译文件加载失败";
    }
    
    // 加载应用程序翻译文件
    QTranslator *appTranslator = new QTranslator(this);
    QString translationsPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, 
                                                     "translations", 
                                                     QStandardPaths::LocateDirectory);
    if (!translationsPath.isEmpty()) {
        if (appTranslator->load("deepinscan_" + QLocale::system().name(), translationsPath)) {
            installTranslator(appTranslator);
            qCDebug(dscannerApp) << "应用程序翻译文件加载成功";
        } else {
            delete appTranslator;
            qCWarning(dscannerApp) << "应用程序翻译文件加载失败";
        }
    } else {
        delete appTranslator;
        qCWarning(dscannerApp) << "翻译文件目录不存在";
    }
    
    return true;
}

bool Application::setupLogging()
{
    qCDebug(dscannerApp) << "设置日志系统";
    
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(logDir);
    
    // 设置深度日志系统
    DLogManager::setLogDir(logDir);
    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();
    
    // 设置日志级别
    QLoggingCategory::setFilterRules("deepinscan.*.debug=true");
    
    qCDebug(dscannerApp) << "日志目录:" << logDir;
    return true;
}

bool Application::initializeCoreComponents()
{
    qCDebug(dscannerApp) << "初始化核心组件 - 现代化架构";
    
    QMutexLocker locker(&g_componentMutex);
    
    if (m_componentsInitialized) {
        qCWarning(dscannerApp) << "核心组件已经初始化";
        return true;
    }
    
    try {
        // 初始化SANE API管理器 - 高效的SANE实现
        g_saneManager = SANEAPIManager::instance();
        if (!g_saneManager) {
            qCCritical(dscannerApp) << "SANE API管理器创建失败";
            return false;
        }
        
        // 初始化SANE系统
        int versionCode = 0;
        if (!g_saneManager->sane_init_impl(&versionCode, nullptr)) {
            qCCritical(dscannerApp) << "SANE系统初始化失败";
            return false;
        }
        qCDebug(dscannerApp) << "SANE系统初始化成功，版本代码:" << versionCode;
        
        // 初始化完整网络发现引擎 - 多协议发现
        g_networkCompleteDiscovery = new NetworkCompleteDiscovery();
        if (!g_networkCompleteDiscovery) {
            qCCritical(dscannerApp) << "网络完整发现引擎创建失败";
            return false;
        }
        qCDebug(dscannerApp) << "网络完整发现引擎创建成功";
        
        m_componentsInitialized = true;
        qCDebug(dscannerApp) << "核心组件初始化完成";
        
        return true;
    } catch (const std::exception &e) {
        qCCritical(dscannerApp) << "核心组件初始化异常:" << e.what();
        return false;
    }
}

bool Application::initializeScannerManager()
{
    qCDebug(dscannerApp) << "初始化扫描仪管理器";
    
    QMutexLocker locker(&g_componentMutex);
    
            // 创建扫描仪管理器 - 多驱动架构
    g_scannerManager = DScannerManager::instance();
    if (!g_scannerManager) {
        qCCritical(dscannerApp) << "扫描仪管理器创建失败";
        return false;
    }
    
    // 初始化管理器
    if (!g_scannerManager->initialize()) {
        qCCritical(dscannerApp) << "扫描仪管理器初始化失败";
        return false;
    }
    
    // 注册厂商驱动 - 现代化的驱动架构
    
    // 注册Genesys驱动
    auto genesysDriver = new GenesysDriverComplete();
    if (!g_scannerManager->registerDriver(genesysDriver)) {
        qCWarning(dscannerApp) << "Genesys驱动注册失败";
        delete genesysDriver;
    } else {
        qCDebug(dscannerApp) << "Genesys驱动注册成功";
    }
    
    // 注册Canon驱动
    auto canonDriver = new CanonDriverComplete();
    if (!g_scannerManager->registerDriver(canonDriver)) {
        qCWarning(dscannerApp) << "Canon驱动注册失败";
        delete canonDriver;
    } else {
        qCDebug(dscannerApp) << "Canon驱动注册成功";
    }
    
    // 注册Epson驱动
    auto epsonDriver = new EpsonDriverComplete();
    if (!g_scannerManager->registerDriver(epsonDriver)) {
        qCWarning(dscannerApp) << "Epson驱动注册失败";
        delete epsonDriver;
    } else {
        qCDebug(dscannerApp) << "Epson驱动注册成功";
    }
    
    qCDebug(dscannerApp) << "扫描仪管理器初始化完成";
    return true;
}

bool Application::initializeImageProcessor()
{
    qCDebug(dscannerApp) << "初始化图像处理器";
    
    QMutexLocker locker(&g_componentMutex);
    
            // 创建图像处理器 - 高级图像处理管道
    g_imageProcessor = new DScannerImageProcessor();
    if (!g_imageProcessor) {
        qCCritical(dscannerApp) << "图像处理器创建失败";
        return false;
    }
    
            // 设置处理器参数 - 优化配置
    ImageProcessingParameters defaultParams;
    defaultParams.algorithm = ImageProcessingAlgorithm::Advanced;
    defaultParams.quality = ImageQuality::High;
    defaultParams.enableAutoCorrection = true;
    defaultParams.enableNoiseReduction = true;
    defaultParams.enableSharpening = false; // 默认关闭锐化
    
    g_imageProcessor->setDefaultParameters(defaultParams);
    
    qCDebug(dscannerApp) << "图像处理器初始化完成";
    return true;
}

bool Application::initializeNetworkDiscovery()
{
    qCDebug(dscannerApp) << "初始化网络发现";
    
    QMutexLocker locker(&g_componentMutex);
    
    // 创建网络发现 - 集成完整的网络发现引擎
    g_networkDiscovery = new DScannerNetworkDiscovery();
    if (!g_networkDiscovery) {
        qCCritical(dscannerApp) << "网络发现创建失败";
        return false;
    }
    
            // 配置网络发现参数 - 网络配置
    g_networkDiscovery->setDiscoveryTimeout(10000); // 10秒超时
    g_networkDiscovery->setMaxDevices(50); // 最多发现50个设备
    g_networkDiscovery->setAutoDiscovery(true); // 启用自动发现
    
    // 连接到完整网络发现引擎
    if (g_networkCompleteDiscovery) {
        connect(g_networkCompleteDiscovery, &NetworkCompleteDiscovery::deviceFound,
                g_networkDiscovery, &DScannerNetworkDiscovery::onDeviceFound);
        connect(g_networkCompleteDiscovery, &NetworkCompleteDiscovery::discoveryFinished,
                g_networkDiscovery, &DScannerNetworkDiscovery::onDiscoveryFinished);
    }
    
    qCDebug(dscannerApp) << "网络发现初始化完成";
    return true;
}

bool Application::initializeSystemTray()
{
    qCDebug(dscannerApp) << "初始化系统托盘";
    
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qCWarning(dscannerApp) << "系统托盘不可用";
        return false;
    }
    
    m_systemTrayIcon = new QSystemTrayIcon(this);
    m_systemTrayIcon->setIcon(windowIcon());
    m_systemTrayIcon->setToolTip(applicationDisplayName());
    
    // 创建托盘菜单
    QMenu *trayMenu = new QMenu();
    
    QAction *showAction = trayMenu->addAction("显示");
    connect(showAction, &QAction::triggered, this, &Application::showMainWindow);
    
    QAction *hideAction = trayMenu->addAction("隐藏");
    connect(hideAction, &QAction::triggered, this, &Application::hideMainWindow);
    
    trayMenu->addSeparator();
    
    QAction *quitAction = trayMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, this, &QCoreApplication::quit);
    
    m_systemTrayIcon->setContextMenu(trayMenu);
    
    // 连接托盘图标信号
    connect(m_systemTrayIcon, &QSystemTrayIcon::activated,
            this, &Application::onSystemTrayActivated);
    
    m_systemTrayIcon->show();
    
    qCDebug(dscannerApp) << "系统托盘初始化完成";
    return true;
}

bool Application::createMainWindow()
{
    qCDebug(dscannerApp) << "创建主窗口";
    
    m_mainWindow = new MainWindow();
    if (!m_mainWindow) {
        qCCritical(dscannerApp) << "主窗口创建失败";
        return false;
    }
    
    // 设置主窗口属性
    m_mainWindow->setWindowTitle(applicationDisplayName());
    m_mainWindow->setWindowIcon(windowIcon());
    
            // 注入核心组件 - 组件集成架构
    m_mainWindow->setScannerManager(g_scannerManager);
    m_mainWindow->setImageProcessor(g_imageProcessor);
    m_mainWindow->setNetworkDiscovery(g_networkDiscovery);
    
    qCDebug(dscannerApp) << "主窗口创建完成";
    return true;
}

void Application::connectSignalsAndSlots()
{
    qCDebug(dscannerApp) << "连接信号和槽";
    
    // 连接应用程序信号
    connect(this, &QApplication::aboutToQuit, this, &Application::cleanup);
    
    // 连接扫描仪管理器信号
    if (g_scannerManager) {
        connect(g_scannerManager, &DScannerManager::deviceAdded,
                this, &Application::onDeviceAdded);
        connect(g_scannerManager, &DScannerManager::deviceRemoved,
                this, &Application::onDeviceRemoved);
        connect(g_scannerManager, &DScannerManager::scanCompleted,
                this, &Application::onScanCompleted);
    }
    
    // 连接网络发现信号
    if (g_networkDiscovery) {
        connect(g_networkDiscovery, &DScannerNetworkDiscovery::deviceFound,
                this, &Application::onNetworkDeviceFound);
    }
    
    qCDebug(dscannerApp) << "信号和槽连接完成";
}

void Application::cleanupCoreComponents()
{
    qCDebug(dscannerApp) << "清理核心组件";
    
    QMutexLocker locker(&g_componentMutex);
    
    // 清理网络发现
    if (g_networkDiscovery) {
        g_networkDiscovery->deleteLater();
        g_networkDiscovery = nullptr;
    }
    
    if (g_networkCompleteDiscovery) {
        delete g_networkCompleteDiscovery;
        g_networkCompleteDiscovery = nullptr;
    }
    
    // 清理图像处理器
    if (g_imageProcessor) {
        g_imageProcessor->deleteLater();
        g_imageProcessor = nullptr;
    }
    
    // 清理扫描仪管理器
    if (g_scannerManager) {
        g_scannerManager->cleanup();
        DScannerManager::destroyInstance();
        g_scannerManager = nullptr;
    }
    
    // 清理SANE API管理器
    if (g_saneManager) {
        SANEAPIManager::destroyInstance();
        g_saneManager = nullptr;
    }
    
    m_componentsInitialized = false;
    
    qCDebug(dscannerApp) << "核心组件清理完成";
}

// 槽函数实现

void Application::onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if (m_mainWindow && m_mainWindow->isVisible()) {
            hideMainWindow();
        } else {
            showMainWindow();
        }
        break;
    default:
        break;
    }
}

void Application::onDeviceAdded(const QString &deviceId)
{
    qCDebug(dscannerApp) << "设备添加:" << deviceId;
    
    if (m_systemTrayIcon) {
        m_systemTrayIcon->showMessage("设备连接", 
                                     QString("扫描仪设备 %1 已连接").arg(deviceId),
                                     QSystemTrayIcon::Information, 3000);
    }
}

void Application::onDeviceRemoved(const QString &deviceId)
{
    qCDebug(dscannerApp) << "设备移除:" << deviceId;
    
    if (m_systemTrayIcon) {
        m_systemTrayIcon->showMessage("设备断开", 
                                     QString("扫描仪设备 %1 已断开").arg(deviceId),
                                     QSystemTrayIcon::Warning, 3000);
    }
}

void Application::onScanCompleted(const QString &deviceId, const QString &filePath)
{
    qCDebug(dscannerApp) << "扫描完成:" << deviceId << "文件:" << filePath;
    
    if (m_systemTrayIcon) {
        m_systemTrayIcon->showMessage("扫描完成", 
                                     QString("扫描完成，已保存到: %1").arg(filePath),
                                     QSystemTrayIcon::Information, 5000);
    }
}

void Application::onNetworkDeviceFound(const QString &deviceName, const QString &address)
{
    qCDebug(dscannerApp) << "发现网络设备:" << deviceName << "地址:" << address;
}

// 静态方法
Application* Application::instance()
{
    return qobject_cast<Application*>(QCoreApplication::instance());
} 