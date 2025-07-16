// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <DWidget>
#include <DStackedWidget>
#include <DToolBar>
#include <DStatusBar>
#include <DMenuBar>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerDevice.h"
#include "Scanner/DScannerTypes.h"

DWIDGET_USE_NAMESPACE
using namespace Dtk::Scanner;

class DeviceListWidget;
class ScanControlWidget;
class ImagePreviewWidget;
class ImageProcessingWidget;
class BatchProcessWidget;
class AdvancedExportDialog;

/**
 * @brief 主窗口类
 * 
 * 基于DTK的现代化扫描仪应用程序主窗口，提供完整的用户界面
 * 包括设备管理、扫描控制、图像预览和处理功能
 */
class MainWindow : public DMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * @brief 初始化应用程序
     * @return 是否成功初始化
     */
    bool initialize();

    /**
     * @brief 关闭应用程序前的清理工作
     */
    void cleanup();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    // 设备管理相关槽函数
    void onDeviceSelected(const DeviceInfo &device);
    void onDeviceConnected(const QString &deviceId);
    void onDeviceDisconnected(const QString &deviceId);
    void onDeviceRefreshRequested();

    // 扫描控制相关槽函数
    void onScanRequested(const ScanParameters &params);
    void onPreviewRequested(const ScanParameters &params);
    void onScanCancelled();
    void onScanCompleted(const QImage &image);
    void onScanProgress(int percentage);

    // 图像处理相关槽函数
    void onImageProcessingRequested();
    void onImageSaveRequested();
    void onImageExportRequested();

    // 批量处理相关槽函数
    void onBatchScanRequested();
    void onBatchProcessingRequested();

    // 菜单和工具栏操作
    void onSettingsRequested();
    void onAboutRequested();
    void onHelpRequested();
    void onExitRequested();

    // 视图切换
    void onViewModeChanged(int mode);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupConnections();
    
    void loadSettings();
    void saveSettings();
    
    void updateDeviceStatus();
    void updateScanStatus();
    void updateBatchProcessingStatus();
    void showMessage(const QString &message, int timeout = 3000);
    void showErrorMessage(const QString &error);

private:
    // 核心组件
    DScannerManager *m_scannerManager;
    DScannerDevice *m_currentDevice;

    // 主界面组件
    DWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QSplitter *m_rightSplitter;

    // 功能组件
    DeviceListWidget *m_deviceList;
    ScanControlWidget *m_scanControl;
    ImagePreviewWidget *m_imagePreview;
    ImageProcessingWidget *m_imageProcessing;
    BatchProcessWidget *m_batchProcess;

    // 界面控制
    DStackedWidget *m_rightPanel;
    DToolBar *m_toolBar;
    DStatusBar *m_statusBar;

    // 状态变量
    bool m_isInitialized;
    bool m_isScanInProgress;
    QString m_currentDeviceId;
    QImage m_currentImage;

    // 界面状态
    enum ViewMode {
        SingleScanMode = 0,     // 单张扫描模式
        BatchScanMode = 1,      // 批量扫描模式
        ProcessingMode = 2      // 图像处理模式
    };
    ViewMode m_currentViewMode;
};

#endif // MAINWINDOW_H 