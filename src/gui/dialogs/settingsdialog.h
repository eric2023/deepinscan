// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <DDialog>
#include <DTabWidget>
#include <DPushButton>
#include <DComboBox>
#include <DSpinBox>
#include <DCheckBox>
#include <DLineEdit>
#include <DSlider>
#include <DGroupBox>
#include <DLabel>
#include <DFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSettings>

DWIDGET_USE_NAMESPACE

/**
 * @brief 设置对话框
 * 
 * 提供应用程序的各种设置选项
 * 包括界面、扫描、输出、高级等设置页面
 */
class SettingsDialog : public DDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    /**
     * @brief 设置配置对象
     * @param settings 配置对象指针
     */
    void setSettings(QSettings *settings);

    /**
     * @brief 加载设置
     */
    void loadSettings();

    /**
     * @brief 保存设置
     */
    void saveSettings();

    /**
     * @brief 重置为默认设置
     */
    void resetToDefaults();

signals:
    /**
     * @brief 设置已更改
     */
    void settingsChanged();

    /**
     * @brief 主题已更改
     * @param themeName 主题名称
     */
    void themeChanged(const QString &themeName);

    /**
     * @brief 语言已更改
     * @param language 语言代码
     */
    void languageChanged(const QString &language);

private slots:
    /**
     * @brief 确定按钮点击槽函数
     */
    void onAccepted();

    /**
     * @brief 取消按钮点击槽函数
     */
    void onRejected();

    /**
     * @brief 应用按钮点击槽函数
     */
    void onApplyClicked();

    /**
     * @brief 重置按钮点击槽函数
     */
    void onResetClicked();

    /**
     * @brief 浏览按钮点击槽函数
     */
    void onBrowseClicked();

    /**
     * @brief 测试按钮点击槽函数
     */
    void onTestClicked();

    /**
     * @brief 设置值改变槽函数
     */
    void onSettingChanged();

private:
    /**
     * @brief 初始化用户界面
     */
    void initUI();

    /**
     * @brief 初始化信号连接
     */
    void initConnections();

    /**
     * @brief 创建界面设置页面
     * @return 界面设置页面widget
     */
    QWidget *createAppearancePage();

    /**
     * @brief 创建扫描设置页面
     * @return 扫描设置页面widget
     */
    QWidget *createScanPage();

    /**
     * @brief 创建输出设置页面
     * @return 输出设置页面widget
     */
    QWidget *createOutputPage();

    /**
     * @brief 创建高级设置页面
     * @return 高级设置页面widget
     */
    QWidget *createAdvancedPage();

    /**
     * @brief 验证设置
     * @return 是否有效
     */
    bool validateSettings();

    /**
     * @brief 应用设置到界面
     */
    void applySettingsToUI();

private:
    // 主要组件
    DTabWidget *m_tabWidget;
    QSettings *m_settings;
    bool m_settingsChanged;

    // 界面设置控件
    DComboBox *m_themeCombo;
    DComboBox *m_languageCombo;
    DCheckBox *m_autoStartCheck;
    DCheckBox *m_minimizeToTrayCheck;
    DCheckBox *m_showThumbnailsCheck;
    DSpinBox *m_thumbnailSizeSpinBox;

    // 扫描设置控件
    DComboBox *m_defaultResolutionCombo;
    DComboBox *m_defaultColorModeCombo;
    DComboBox *m_defaultFormatCombo;
    DCheckBox *m_autoDetectDevicesCheck;
    DCheckBox *m_rememberLastSettingsCheck;
    DSpinBox *m_scanTimeoutSpinBox;
    DCheckBox *m_enablePreviewCheck;

    // 输出设置控件
    DLineEdit *m_defaultSavePathEdit;
    DPushButton *m_browseSavePathButton;
    DComboBox *m_fileNamingCombo;
    DCheckBox *m_createDateFoldersCheck;
    DCheckBox *m_autoOpenOutputCheck;
    DSlider *m_jpegQualitySlider;
    DSpinBox *m_jpegQualitySpinBox;

    // 高级设置控件
    DCheckBox *m_enableLoggingCheck;
    DComboBox *m_logLevelCombo;
    DCheckBox *m_enableCacheCheck;
    DSpinBox *m_cacheMaxSizeSpinBox;
    DCheckBox *m_enableHardwareAccelCheck;
    DSpinBox *m_maxThreadsSpinBox;
    DCheckBox *m_enableAutoUpdateCheck;

    // 按钮
    DPushButton *m_applyButton;
    DPushButton *m_resetButton;
    DPushButton *m_testButton;
};

#endif // SETTINGSDIALOG_H 