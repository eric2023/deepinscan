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

#include "Scanner/DScannerTypes.h"

DWIDGET_USE_NAMESPACE
using namespace Dtk::Scanner;

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

    /**
     * @brief 保存任务到JSON文件
     * @param fileName 文件名
     * @return 是否成功
     */
    bool saveTasksToJson(const QString &fileName);

    /**
     * @brief 从JSON文件加载任务
     * @param fileName 文件名
     * @return 是否成功
     */
    bool loadTasksFromJson(const QString &fileName);

private:
    // 主布局
    QVBoxLayout *m_mainLayout;

    // 任务列表
    DListWidget *m_taskList;
    QList<BatchItem> m_batchItems;
    QQueue<QString> m_processingQueue;

    // 输出设置控件
    DComboBox *m_outputFormatCombo;
    DComboBox *m_outputQualityCombo;
    DCheckBox *m_autoNamingCheck;
    DCheckBox *m_createSubfoldersCheck;
    DComboBox *m_namingPatternCombo;

    // 控制按钮
    DPushButton *m_addItemButton;
    DPushButton *m_removeItemButton;
    DPushButton *m_clearItemsButton;
    DPushButton *m_startButton;
    DPushButton *m_stopButton;
    DPushButton *m_pauseButton;

    // 进度显示
    DProgressBar *m_totalProgressBar;
    DProgressBar *m_currentProgressBar;
    DLabel *m_statusLabel;
    DLabel *m_countLabel;

    // 处理状态
    bool m_isProcessing;
    bool m_isPaused;
    QString m_currentItemId;
    int m_completedCount;
    int m_totalCount;

    // 定时器
    QTimer *m_processTimer;
};

#endif // BATCHPROCESSWIDGET_H 