// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BATCHPROCESSWIDGET_H
#define BATCHPROCESSWIDGET_H

#include <DWidget>
#include <DListWidget>
#include <DPushButton>
#include <DComboBox>
#include <DSpinBox>
#include <DCheckBox>
#include <DGroupBox>
#include <DLabel>
#include <DProgressBar>
#include <DFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QQueue>
#include <DLineEdit>
#include <QFileInfo>
#include <QLoggingCategory>

#include "Scanner/DScannerTypes.h"

DWIDGET_USE_NAMESPACE
using namespace Dtk::Scanner;

Q_DECLARE_LOGGING_CATEGORY(batchProcessWidget)

/**
 * @brief 批量处理设置结构
 */
struct BatchProcessingSettings {
    QString inputDirectory;       // 输入目录
    QString outputDirectory;      // 输出目录
    QString outputFormat;         // 输出格式
    int quality;                  // 图像质量
    QString namingMode;           // 命名模式
    QString namingPattern;        // 命名模式
    int concurrentTasks;          // 并发任务数
    QString errorHandling;        // 错误处理方式
    bool overwriteFiles;          // 覆盖文件
    bool includeSubdirs;          // 包含子目录
    QString fileFilter;           // 文件过滤器
};

/**
 * @brief 批量扫描处理组件
 * 
 * 提供批量扫描和处理功能
 * 支持多文档扫描、自动处理和批量输出
 */
class BatchProcessWidget : public DWidget
{
    Q_OBJECT

public:
    /**
     * @brief 批量任务项结构
     */
    struct BatchItem {
        QString id;              // 任务ID
        QString name;            // 任务名称
        ScanParameters scanParams; // 扫描参数
        QString outputPath;      // 输出路径
        QString status;          // 状态
        int progress;            // 进度
        QPixmap thumbnail;       // 缩略图
        
        // 扩展字段以兼容实现文件（按构造函数初始化顺序排列）
        QString sourcePath;      // 源文件路径
        QString inputPath;       // 输入路径
        int quality;             // 质量
        int resolution;          // 分辨率
        ColorMode colorMode;     // 颜色模式
        ImageFormat format;      // 图像格式
        bool autoCrop;           // 自动裁剪
        bool autoDeskew;         // 自动倾斜校正
        double rotateAngle;      // 旋转角度
        double brightness;       // 亮度
        double contrast;         // 对比度
        QString errorMessage;    // 错误信息
        QDateTime createdTime;   // 创建时间
        QDateTime startedTime;   // 开始时间
        QDateTime finishedTime;  // 完成时间
        
        // 构造函数
        BatchItem() : progress(0), quality(85), resolution(300), 
                     colorMode(ColorMode::Color), format(ImageFormat::JPEG),
                     autoCrop(false), autoDeskew(false), rotateAngle(0.0),
                     brightness(0.0), contrast(0.0) {}
    };

    explicit BatchProcessWidget(QWidget *parent = nullptr);
    ~BatchProcessWidget();

    /**
     * @brief 添加批量任务
     * @param item 任务项
     */
    void addBatchItem(const BatchItem &item);

    /**
     * @brief 移除批量任务
     * @param itemId 任务ID
     */
    void removeBatchItem(const QString &itemId);

    /**
     * @brief 清空所有任务
     */
    void clearAllItems();

    /**
     * @brief 开始批量处理
     */
    void startBatchProcessing();

    /**
     * @brief 停止批量处理
     */
    void stopBatchProcessing();

    /**
     * @brief 暂停批量处理
     */
    void pauseBatchProcessing();

    /**
     * @brief 恢复批量处理
     */
    void resumeBatchProcessing();

    /**
     * @brief 获取任务总数
     * @return 任务总数
     */
    int getTotalItems() const;

    /**
     * @brief 获取已完成任务数
     * @return 已完成任务数
     */
    int getCompletedItems() const;

    /**
     * @brief 获取批量处理设置
     * @return 批量处理设置
     */
    BatchProcessingSettings getBatchSettings() const;

signals:
    /**
     * @brief 批量处理开始
     */
    void batchProcessingStarted();

    /**
     * @brief 批量处理完成
     */
    void batchProcessingFinished();

    /**
     * @brief 批量处理暂停
     */
    void batchProcessingPaused();

    /**
     * @brief 批量处理开始信号
     */
    void processingStarted();

    /**
     * @brief 批量处理暂停信号
     */
    void processingPaused();

    /**
     * @brief 批量处理恢复信号
     */
    void processingResumed();

    /**
     * @brief 批量处理停止信号
     */
    void processingStopped();

    /**
     * @brief 批量处理完成信号
     * @param completed 完成数量
     * @param failed 失败数量
     */
    void processingCompleted(int completed, int failed);

    /**
     * @brief 单个任务完成
     * @param itemId 任务ID
     * @param success 是否成功
     */
    void itemProcessed(const QString &itemId, bool success);

    /**
     * @brief 进度更新
     * @param current 当前进度
     * @param total 总进度
     */
    void progressUpdated(int current, int total);

    /**
     * @brief 请求扫描
     * @param params 扫描参数
     */
    void scanRequested(const ScanParameters &params);

private slots:
    /**
     * @brief 添加任务按钮点击槽函数
     */
    void onAddItemClicked();

    /**
     * @brief 移除任务按钮点击槽函数
     */
    void onRemoveItemClicked();

    /**
     * @brief 清空任务按钮点击槽函数
     */
    void onClearItemsClicked();

    /**
     * @brief 开始处理按钮点击槽函数
     */
    void onStartProcessingClicked();

    /**
     * @brief 停止处理按钮点击槽函数
     */
    void onStopProcessingClicked();

    /**
     * @brief 暂停处理按钮点击槽函数
     */
    void onPauseProcessingClicked();

    /**
     * @brief 任务列表选择改变槽函数
     */
    void onItemSelectionChanged();

    /**
     * @brief 任务列表双击槽函数
     */
    void onItemDoubleClicked();

    /**
     * @brief 输出设置改变槽函数
     */
    void onOutputSettingsChanged();

    /**
     * @brief 处理下一个任务
     */
    void processNextItem();

    /**
     * @brief 任务处理完成槽函数
     */
    void onItemProcessingFinished(bool success);

    /**
     * @brief 更新UI状态
     */
    void updateUIState();

    // 添加缺失的槽函数
    void onFileFilterChanged(const QString &filter);
    void onOutputFormatChanged(const QString &format);
    void onNamingModeChanged(const QString &mode);

private:
    // UI设置方法
    void setupUI();
    void setupFileManagementGroup();
    void setupProcessingSettingsGroup();
    void setupTaskListGroup();
    void setupProgressGroup();
    void setupControlButtons();
    void connectSignals();
    
    // 文件操作方法
    void selectInputDirectory();
    void selectOutputDirectory();
    void addFiles();
    void scanDirectory();
    
    // 任务管理方法
    void removeSelectedTasks();
    void clearAllTasks();
    void startProcessing();
    void togglePause();
    void stopProcessing();
    void saveQueue();
    void loadQueue();
    
    // 状态更新方法
    void updateProgress();
    void updateTaskStats();
    void updateControlButtonsState();
    
    // 辅助方法
    bool addTaskToQueue(const QString &filePath);
    QString applyNamingPattern(const QString &originalName) const;
    QStringList getFileFilters() const;
    QStringList scanDirectoryRecursive(const QString &dirPath, const QStringList &filters, bool recursive) const;
    
    // 文件操作
    bool saveTasksToJson(const QString &filePath);
    bool loadTasksFromJson(const QString &filePath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

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
     * @brief 创建任务列表区域
     * @return 任务列表区域widget
     */
    QWidget *createTaskListArea();

    /**
     * @brief 创建输出设置区域
     * @return 输出设置区域widget
     */
    QWidget *createOutputSettingsArea();

    /**
     * @brief 创建控制按钮区域
     * @return 控制按钮区域widget
     */
    QWidget *createControlArea();

    /**
     * @brief 创建进度显示区域
     * @return 进度显示区域widget
     */
    QWidget *createProgressArea();

    /**
     * @brief 更新任务列表显示
     */
    void updateTaskList();

    /**
     * @brief 更新任务项显示
     * @param item 任务项
     */
    void updateTaskItem(const BatchItem &item);

    /**
     * @brief 获取选中的任务项
     * @return 选中的任务项，如果没有选中则返回空指针
     */
    BatchItem *getSelectedItem();

    /**
     * @brief 根据ID查找任务项
     * @param itemId 任务ID
     * @return 任务项指针，如果未找到则返回空指针
     */
    BatchItem *findItemById(const QString &itemId);

    /**
     * @brief 生成唯一的任务ID
     * @return 唯一ID
     */
    QString generateItemId();

    /**
     * @brief 验证输出设置
     * @return 是否有效
     */
    bool validateOutputSettings();

    // 重复声明已删除，使用private区域的声明

private:
    // 主布局
    QVBoxLayout *m_mainLayout;

    // 任务列表
    DListWidget *m_taskList;
    QList<BatchItem> m_batchItems;
    QList<BatchItem> m_tasks;  // 兼容实现文件中的命名
    QQueue<QString> m_processingQueue;

    // 输出设置控件
    DComboBox *m_outputFormatCombo;
    DComboBox *m_outputFormatComboBox;  // 兼容命名
    DComboBox *m_outputQualityCombo;
    DCheckBox *m_autoNamingCheck;
    DCheckBox *m_createSubfoldersCheck;
    DComboBox *m_namingPatternCombo;
    DComboBox *m_namingModeComboBox;    // 兼容命名
    DLineEdit *m_namingPatternEdit;     // 兼容命名
    DLineEdit *m_outputPathEdit;        // 兼容命名
    DComboBox *m_fileFilterComboBox;    // 兼容命名
    DCheckBox *m_overwriteFiles;        // 覆盖文件选项
    DCheckBox *m_includeSubdirs;        // 包含子目录选项
    
    // 输入设置控件
    DLineEdit *m_inputPathEdit;         // 输入路径编辑框
    DSpinBox *m_qualitySpinBox;         // 质量设置
    DSpinBox *m_concurrentTasksSpinBox; // 并发任务数设置
    DComboBox *m_errorHandlingComboBox; // 错误处理方式

    // 控制按钮
    DPushButton *m_addItemButton;
    DPushButton *m_addFilesButton;      // 兼容命名
    DPushButton *m_removeItemButton;
    DPushButton *m_clearItemsButton;
    DPushButton *m_clearAllButton;      // 兼容命名
    DPushButton *m_startButton;
    DPushButton *m_stopButton;
    DPushButton *m_pauseButton;
    DPushButton *m_inputBrowseButton;   // 兼容命名
    DPushButton *m_outputBrowseButton;  // 兼容命名
    DPushButton *m_scanDirectoryButton; // 兼容命名
    DPushButton *m_removeSelectedButton; // 移除选中按钮
    DPushButton *m_saveQueueButton;     // 保存队列按钮
    DPushButton *m_loadQueueButton;     // 加载队列按钮

    // 进度显示
    DProgressBar *m_totalProgressBar;
    DProgressBar *m_overallProgressBar;   // 兼容命名
    DProgressBar *m_currentProgressBar;
    DLabel *m_statusLabel;
    DLabel *m_countLabel;
    DLabel *m_overallProgressLabel;       // 兼容命名
    DLabel *m_currentFileLabel;           // 兼容命名
    DLabel *m_currentProgressLabel;       // 兼容命名
    DLabel *m_taskStatsLabel;             // 兼容命名
    DListWidget *m_taskListWidget;        // 兼容命名
    DLabel *m_estimatedTimeLabel;         // 预计时间标签
    
    // 分组控件
    DGroupBox *m_fileGroup;               // 文件管理分组
    DGroupBox *m_settingsGroup;           // 设置分组
    DGroupBox *m_taskGroup;               // 任务分组
    DGroupBox *m_progressGroup;           // 进度分组
    DFrame *m_controlFrame;               // 控制按钮框架

    // 处理状态
    bool m_isProcessing;
    bool m_isPaused;
    QString m_currentItemId;
    int m_completedCount;
    int m_totalCount;
    int m_currentTaskIndex;
    int m_completedTasks;
    int m_failedTasks;

    // 定时器
    QTimer *m_processTimer;
    QTimer *m_timer;
};

#endif // BATCHPROCESSWIDGET_H 