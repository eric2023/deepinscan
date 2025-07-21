// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPLICATION_H
#define APPLICATION_H

#include <DApplication>
#include <QTranslator>
#include <QSettings>
#include <QSplashScreen>
#include <QSystemTrayIcon>

DWIDGET_USE_NAMESPACE

class MainWindow;

// 使用完整命名空间避免冲突
namespace Dtk {
namespace Scanner {
    class DScannerManager;
    class DScannerImageProcessor;
    class DScannerNetworkDiscovery;
    class NetworkCompleteDiscovery;
}
}

/**
 * @brief 深度扫描应用程序类
 * 
 * 基于DTK的扫描仪应用程序主类
 * 负责应用程序的初始化、配置管理和国际化
 */
class Application : public DApplication
{
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    /**
     * @brief 初始化应用程序
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * @brief 显示主窗口
     */
    void showMainWindow();

    /**
     * @brief 隐藏主窗口
     */
    void hideMainWindow();

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 获取扫描仪管理器
     * @return 扫描仪管理器指针
     */
    Dtk::Scanner::DScannerManager *scannerManager() const;

    /**
     * @brief 获取图像处理器
     * @return 图像处理器指针
     */
    Dtk::Scanner::DScannerImageProcessor *imageProcessor() const;

    /**
     * @brief 获取网络发现组件
     * @return 网络发现组件指针
     */
    Dtk::Scanner::DScannerNetworkDiscovery *networkDiscovery() const;

    /**
     * @brief 获取主窗口
     * @return 主窗口指针
     */
    MainWindow *mainWindow() const;

    /**
     * @brief 获取应用实例
     * @return 应用实例指针
     */
    static Application *instance();

private slots:
    // 系统托盘相关
    void onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason);
    
    // 设备管理相关
    void onDeviceAdded(const QString &deviceId);
    void onDeviceRemoved(const QString &deviceId);
    void onScanCompleted(const QString &deviceId, const QString &filePath);
    void onNetworkDeviceFound(const QString &deviceName, const QString &address);

private:
    // 初始化方法
    void showSplashScreen();
    void updateSplashMessage(const QString &message);
    bool setupLocalization();
    bool setupLogging();
    bool initializeCoreComponents();
    bool initializeScannerManager();
    bool initializeImageProcessor();
    bool initializeNetworkDiscovery();
    bool initializeSystemTray();
    bool createMainWindow();
    void connectSignalsAndSlots();
    void cleanupCoreComponents();

private:
    MainWindow *m_mainWindow;
    QSplashScreen *m_splashScreen;
    QSystemTrayIcon *m_systemTrayIcon;
    bool m_initialized;
    bool m_componentsInitialized;
    
    // 核心组件
    Dtk::Scanner::DScannerManager *m_scannerManager;
    Dtk::Scanner::DScannerImageProcessor *m_imageProcessor;
    Dtk::Scanner::DScannerNetworkDiscovery *m_networkDiscovery;
};

#endif // APPLICATION_H 