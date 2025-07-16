// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPLICATION_H
#define APPLICATION_H

#include <DApplication>
#include <QTranslator>
#include <QSettings>

DWIDGET_USE_NAMESPACE

class MainWindow;
class DScannerManager;

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
     * @brief 获取应用程序设置
     * @return 设置对象指针
     */
    QSettings *settings() const { return m_settings; }

    /**
     * @brief 获取扫描仪管理器
     * @return 扫描仪管理器指针
     */
    DScannerManager *scannerManager() const { return m_scannerManager; }

public slots:
    /**
     * @brief 显示关于对话框
     */
    void showAbout();

    /**
     * @brief 显示设置对话框
     */
    void showSettings();

    /**
     * @brief 切换主题
     */
    void toggleTheme();

private slots:
    /**
     * @brief 应用程序即将退出
     */
    void onAboutToQuit();

private:
    /**
     * @brief 初始化应用程序属性
     */
    void initializeApplication();

    /**
     * @brief 初始化国际化支持
     */
    void initializeTranslations();

    /**
     * @brief 初始化主题
     */
    void initializeTheme();

    /**
     * @brief 初始化扫描仪管理器
     */
    void initializeScannerManager();

    /**
     * @brief 加载应用程序设置
     */
    void loadSettings();

    /**
     * @brief 保存应用程序设置
     */
    void saveSettings();

private:
    MainWindow *m_mainWindow;
    DScannerManager *m_scannerManager;
    QSettings *m_settings;
    QTranslator *m_translator;
    QTranslator *m_qtTranslator;
};

#endif // APPLICATION_H 