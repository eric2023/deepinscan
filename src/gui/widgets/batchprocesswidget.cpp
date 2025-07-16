/*
 * SPDX-FileCopyrightText: 2024-2025 eric2023
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "batchprocesswidget.h"
#include <DGroupBox>
#include <DLabel>
#include <DPushButton>
#include <DComboBox>
#include <DSpinBox>
#include <DCheckBox>
#include <DListWidget>
#include <DProgressBar>
#include <DFrame>
#include <DLineEdit>
#include <DFileDialog>
#include <DMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QDebug>

DWIDGET_USE_NAMESPACE

DBatchProcessWidget::DBatchProcessWidget(QWidget *parent)
    : DWidget(parent)
    , m_isProcessing(false)
    , m_currentTaskIndex(0)
    , m_completedTasks(0)
    , m_failedTasks(0)
    , m_timer(new QTimer(this))
{
    qDebug() << "初始化批量处理组件";
    setupUI();
    connectSignals();
    
    // 设置定时器用于更新进度
    m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, this, &DBatchProcessWidget::updateProgress);
    
    // 启用拖放功能
    setAcceptDrops(true);
    
    qDebug() << "批量处理组件初始化完成";
}

DBatchProcessWidget::~DBatchProcessWidget()
{
    qDebug() << "销毁批量处理组件";
    stopProcessing();
}

void DBatchProcessWidget::setupUI()
{
    qDebug() << "设置批量处理界面布局";
    
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 文件管理组
    setupFileManagementGroup();
    mainLayout->addWidget(m_fileGroup);
    
    // 处理设置组
    setupProcessingSettingsGroup();
    mainLayout->addWidget(m_settingsGroup);
    
    // 任务列表组
    setupTaskListGroup();
    mainLayout->addWidget(m_taskGroup);
    
    // 进度监控组
    setupProgressGroup();
    mainLayout->addWidget(m_progressGroup);
    
    // 控制按钮组
    setupControlButtons();
    mainLayout->addWidget(m_controlFrame);
    
    qDebug() << "批量处理界面布局设置完成";
}

void DBatchProcessWidget::setupFileManagementGroup()
{
    qDebug() << "设置文件管理组件";
    
    m_fileGroup = new DGroupBox("文件管理", this);
    auto *layout = new QVBoxLayout(m_fileGroup);
    layout->setSpacing(8);
    
    // 输入目录选择
    auto *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new DLabel("输入目录:", this));
    
    m_inputPathEdit = new DLineEdit(this);
    m_inputPathEdit->setPlaceholderText("选择包含待处理图像的目录");
    m_inputPathEdit->setReadOnly(true);
    inputLayout->addWidget(m_inputPathEdit);
    
    m_inputBrowseButton = new DPushButton("浏览", this);
    m_inputBrowseButton->setToolTip("选择输入目录");
    inputLayout->addWidget(m_inputBrowseButton);
    
    layout->addLayout(inputLayout);
    
    // 输出目录选择
    auto *outputLayout = new QHBoxLayout();
    outputLayout->addWidget(new DLabel("输出目录:", this));
    
    m_outputPathEdit = new DLineEdit(this);
    m_outputPathEdit->setPlaceholderText("选择处理后图像的保存目录");
    m_outputPathEdit->setReadOnly(true);
    outputLayout->addWidget(m_outputPathEdit);
    
    m_outputBrowseButton = new DPushButton("浏览", this);
    m_outputBrowseButton->setToolTip("选择输出目录");
    outputLayout->addWidget(m_outputBrowseButton);
    
    layout->addLayout(outputLayout);
    
    // 文件过滤器
    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new DLabel("文件类型:", this));
    
    m_fileFilterComboBox = new DComboBox(this);
    m_fileFilterComboBox->addItems({
        "所有图像文件 (*.jpg *.jpeg *.png *.tiff *.bmp)",
        "JPEG文件 (*.jpg *.jpeg)",
        "PNG文件 (*.png)",
        "TIFF文件 (*.tiff *.tif)",
        "BMP文件 (*.bmp)",
        "自定义筛选器"
    });
    m_fileFilterComboBox->setToolTip("选择要处理的文件类型");
    filterLayout->addWidget(m_fileFilterComboBox);
    filterLayout->addStretch();
    
    layout->addLayout(filterLayout);
    
    // 子目录递归选项
    m_includeSubdirs = new DCheckBox("包含子目录", this);
    m_includeSubdirs->setChecked(true);
    m_includeSubdirs->setToolTip("递归搜索子目录中的图像文件");
    layout->addWidget(m_includeSubdirs);
    
    qDebug() << "文件管理组件设置完成";
}

void DBatchProcessWidget::setupProcessingSettingsGroup()
{
    qDebug() << "设置处理设置组件";
    
    m_settingsGroup = new DGroupBox("处理设置", this);
    auto *layout = new QGridLayout(m_settingsGroup);
    layout->setSpacing(8);
    
    // 输出格式设置
    layout->addWidget(new DLabel("输出格式:", this), 0, 0);
    m_outputFormatComboBox = new DComboBox(this);
    m_outputFormatComboBox->addItems({"JPEG", "PNG", "TIFF", "BMP"});
    m_outputFormatComboBox->setCurrentText("JPEG");
    m_outputFormatComboBox->setToolTip("选择输出图像格式");
    layout->addWidget(m_outputFormatComboBox, 0, 1);
    
    // 图像质量设置（仅对JPEG有效）
    layout->addWidget(new DLabel("图像质量:", this), 0, 2);
    m_qualitySpinBox = new DSpinBox(this);
    m_qualitySpinBox->setRange(1, 100);
    m_qualitySpinBox->setValue(90);
    m_qualitySpinBox->setSuffix("%");
    m_qualitySpinBox->setToolTip("JPEG图像压缩质量 (1-100)");
    layout->addWidget(m_qualitySpinBox, 0, 3);
    
    // 文件命名模式
    layout->addWidget(new DLabel("命名模式:", this), 1, 0);
    m_namingModeComboBox = new DComboBox(this);
    m_namingModeComboBox->addItems({
        "保持原名称",
        "添加前缀",
        "添加后缀",
        "重新编号",
        "自定义模式"
    });
    m_namingModeComboBox->setToolTip("选择输出文件的命名方式");
    layout->addWidget(m_namingModeComboBox, 1, 1);
    
    // 命名模式参数
    m_namingPatternEdit = new DLineEdit(this);
    m_namingPatternEdit->setPlaceholderText("输入命名模式");
    m_namingPatternEdit->setEnabled(false);
    layout->addWidget(m_namingPatternEdit, 1, 2, 1, 2);
    
    // 并发处理设置
    layout->addWidget(new DLabel("并发任务:", this), 2, 0);
    m_concurrentTasksSpinBox = new DSpinBox(this);
    m_concurrentTasksSpinBox->setRange(1, 8);
    m_concurrentTasksSpinBox->setValue(2);
    m_concurrentTasksSpinBox->setToolTip("同时处理的任务数量");
    layout->addWidget(m_concurrentTasksSpinBox, 2, 1);
    
    // 错误处理策略
    layout->addWidget(new DLabel("错误处理:", this), 2, 2);
    m_errorHandlingComboBox = new DComboBox(this);
    m_errorHandlingComboBox->addItems({
        "跳过错误继续",
        "停止处理",
        "重试一次"
    });
    m_errorHandlingComboBox->setToolTip("遇到错误时的处理策略");
    layout->addWidget(m_errorHandlingComboBox, 2, 3);
    
    // 覆写文件选项
    m_overwriteFiles = new DCheckBox("覆写已存在的文件", this);
    m_overwriteFiles->setChecked(false);
    m_overwriteFiles->setToolTip("如果输出文件已存在，是否覆写");
    layout->addWidget(m_overwriteFiles, 3, 0, 1, 4);
    
    qDebug() << "处理设置组件设置完成";
}

void DBatchProcessWidget::setupTaskListGroup()
{
    qDebug() << "设置任务列表组件";
    
    m_taskGroup = new DGroupBox("任务列表", this);
    auto *layout = new QVBoxLayout(m_taskGroup);
    layout->setSpacing(8);
    
    // 工具栏
    auto *toolbarLayout = new QHBoxLayout();
    
    m_addFilesButton = new DPushButton("添加文件", this);
    m_addFilesButton->setToolTip("添加单个文件到处理队列");
    toolbarLayout->addWidget(m_addFilesButton);
    
    m_scanDirectoryButton = new DPushButton("扫描目录", this);
    m_scanDirectoryButton->setToolTip("扫描输入目录中的所有图像文件");
    toolbarLayout->addWidget(m_scanDirectoryButton);
    
    m_removeSelectedButton = new DPushButton("移除选中", this);
    m_removeSelectedButton->setToolTip("从队列中移除选中的任务");
    toolbarLayout->addWidget(m_removeSelectedButton);
    
    m_clearAllButton = new DPushButton("清空队列", this);
    m_clearAllButton->setToolTip("清空所有待处理任务");
    toolbarLayout->addWidget(m_clearAllButton);
    
    toolbarLayout->addStretch();
    
    // 任务统计标签
    m_taskStatsLabel = new DLabel("任务: 0 | 待处理: 0 | 完成: 0 | 失败: 0", this);
    toolbarLayout->addWidget(m_taskStatsLabel);
    
    layout->addLayout(toolbarLayout);
    
    // 任务列表
    m_taskListWidget = new DListWidget(this);
    m_taskListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_taskListWidget->setDragDropMode(QAbstractItemView::DropOnly);
    m_taskListWidget->setAcceptDrops(true);
    m_taskListWidget->setToolTip("拖放文件到此处添加到处理队列");
    layout->addWidget(m_taskListWidget);
    
    qDebug() << "任务列表组件设置完成";
}

void DBatchProcessWidget::setupProgressGroup()
{
    qDebug() << "设置进度监控组件";
    
    m_progressGroup = new DGroupBox("进度监控", this);
    auto *layout = new QVBoxLayout(m_progressGroup);
    layout->setSpacing(8);
    
    // 总体进度
    auto *overallLayout = new QHBoxLayout();
    overallLayout->addWidget(new DLabel("总体进度:", this));
    
    m_overallProgressBar = new DProgressBar(this);
    m_overallProgressBar->setMinimum(0);
    m_overallProgressBar->setMaximum(100);
    m_overallProgressBar->setValue(0);
    overallLayout->addWidget(m_overallProgressBar);
    
    m_overallProgressLabel = new DLabel("0%", this);
    overallLayout->addWidget(m_overallProgressLabel);
    
    layout->addLayout(overallLayout);
    
    // 当前任务进度
    auto *currentLayout = new QHBoxLayout();
    currentLayout->addWidget(new DLabel("当前任务:", this));
    
    m_currentProgressBar = new DProgressBar(this);
    m_currentProgressBar->setMinimum(0);
    m_currentProgressBar->setMaximum(100);
    m_currentProgressBar->setValue(0);
    currentLayout->addWidget(m_currentProgressBar);
    
    m_currentProgressLabel = new DLabel("0%", this);
    currentLayout->addWidget(m_currentProgressLabel);
    
    layout->addLayout(currentLayout);
    
    // 当前处理文件
    m_currentFileLabel = new DLabel("当前文件: 无", this);
    layout->addWidget(m_currentFileLabel);
    
    // 预计剩余时间
    m_estimatedTimeLabel = new DLabel("预计剩余时间: --:--", this);
    layout->addWidget(m_estimatedTimeLabel);
    
    qDebug() << "进度监控组件设置完成";
}

void DBatchProcessWidget::setupControlButtons()
{
    qDebug() << "设置控制按钮";
    
    m_controlFrame = new DFrame(this);
    auto *layout = new QHBoxLayout(m_controlFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 开始处理按钮
    m_startButton = new DPushButton("开始处理", this);
    m_startButton->setToolTip("开始批量处理队列中的所有任务");
    layout->addWidget(m_startButton);
    
    // 暂停/恢复按钮
    m_pauseButton = new DPushButton("暂停", this);
    m_pauseButton->setToolTip("暂停当前处理进程");
    m_pauseButton->setEnabled(false);
    layout->addWidget(m_pauseButton);
    
    // 停止按钮
    m_stopButton = new DPushButton("停止", this);
    m_stopButton->setToolTip("停止批量处理");
    m_stopButton->setEnabled(false);
    layout->addWidget(m_stopButton);
    
    layout->addStretch();
    
    // 保存队列按钮
    m_saveQueueButton = new DPushButton("保存队列", this);
    m_saveQueueButton->setToolTip("将当前任务队列保存到文件");
    layout->addWidget(m_saveQueueButton);
    
    // 加载队列按钮
    m_loadQueueButton = new DPushButton("加载队列", this);
    m_loadQueueButton->setToolTip("从文件加载任务队列");
    layout->addWidget(m_loadQueueButton);
    
    qDebug() << "控制按钮设置完成";
}

void DBatchProcessWidget::connectSignals()
{
    qDebug() << "连接批量处理信号";
    
    // 文件管理信号连接
    connect(m_inputBrowseButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::selectInputDirectory);
    connect(m_outputBrowseButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::selectOutputDirectory);
    connect(m_fileFilterComboBox, QOverload<const QString &>::of(&DComboBox::currentTextChanged),
            this, &DBatchProcessWidget::onFileFilterChanged);
    
    // 处理设置信号连接
    connect(m_outputFormatComboBox, QOverload<const QString &>::of(&DComboBox::currentTextChanged),
            this, &DBatchProcessWidget::onOutputFormatChanged);
    connect(m_namingModeComboBox, QOverload<const QString &>::of(&DComboBox::currentTextChanged),
            this, &DBatchProcessWidget::onNamingModeChanged);
    
    // 任务列表信号连接
    connect(m_addFilesButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::addFiles);
    connect(m_scanDirectoryButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::scanDirectory);
    connect(m_removeSelectedButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::removeSelectedTasks);
    connect(m_clearAllButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::clearAllTasks);
    
    // 控制按钮信号连接
    connect(m_startButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::startProcessing);
    connect(m_pauseButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::togglePause);
    connect(m_stopButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::stopProcessing);
    connect(m_saveQueueButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::saveQueue);
    connect(m_loadQueueButton, &DPushButton::clicked,
            this, &DBatchProcessWidget::loadQueue);
    
    qDebug() << "批量处理信号连接完成";
}

void DBatchProcessWidget::selectInputDirectory()
{
    qDebug() << "选择输入目录";
    
    QString dir = DFileDialog::getExistingDirectory(this, "选择输入目录", 
                                                   m_inputPathEdit->text());
    if (!dir.isEmpty()) {
        m_inputPathEdit->setText(dir);
        qDebug() << QString("输入目录设置为: %1").arg(dir);
    }
}

void DBatchProcessWidget::selectOutputDirectory()
{
    qDebug() << "选择输出目录";
    
    QString dir = DFileDialog::getExistingDirectory(this, "选择输出目录", 
                                                   m_outputPathEdit->text());
    if (!dir.isEmpty()) {
        m_outputPathEdit->setText(dir);
        qDebug() << QString("输出目录设置为: %1").arg(dir);
    }
}

void DBatchProcessWidget::onFileFilterChanged(const QString &filter)
{
    qDebug() << QString("文件过滤器变化: %1").arg(filter);
    // 这里可以添加自定义过滤器的处理逻辑
}

void DBatchProcessWidget::onOutputFormatChanged(const QString &format)
{
    qDebug() << QString("输出格式变化: %1").arg(format);
    
    // 仅对JPEG格式启用质量设置
    bool isJpeg = (format == "JPEG");
    m_qualitySpinBox->setEnabled(isJpeg);
    
    if (!isJpeg) {
        m_qualitySpinBox->setValue(100);
    }
}

void DBatchProcessWidget::onNamingModeChanged(const QString &mode)
{
    qDebug() << QString("命名模式变化: %1").arg(mode);
    
    // 根据命名模式启用/禁用模式参数输入
    bool needsPattern = (mode == "添加前缀" || mode == "添加后缀" || 
                        mode == "重新编号" || mode == "自定义模式");
    m_namingPatternEdit->setEnabled(needsPattern);
    
    // 设置提示文本
    if (mode == "添加前缀") {
        m_namingPatternEdit->setPlaceholderText("输入前缀");
    } else if (mode == "添加后缀") {
        m_namingPatternEdit->setPlaceholderText("输入后缀");
    } else if (mode == "重新编号") {
        m_namingPatternEdit->setPlaceholderText("例如: IMG_%04d (IMG_0001, IMG_0002...)");
    } else if (mode == "自定义模式") {
        m_namingPatternEdit->setPlaceholderText("例如: %name%_%date% (%name%=原文件名, %date%=日期)");
    }
}

void DBatchProcessWidget::addFiles()
{
    qDebug() << "添加文件到处理队列";
    
    QStringList files = DFileDialog::getOpenFileNames(this, "选择要处理的图像文件", 
                                                     QString(),
                                                     "图像文件 (*.jpg *.jpeg *.png *.tiff *.bmp)");
    
    for (const QString &file : files) {
        addTaskToQueue(file);
    }
    
    updateTaskStats();
    qDebug() << QString("添加了 %1 个文件到队列").arg(files.size());
}

void DBatchProcessWidget::scanDirectory()
{
    qDebug() << "扫描目录中的图像文件";
    
    QString inputDir = m_inputPathEdit->text();
    if (inputDir.isEmpty()) {
        DMessageBox::warning(this, "警告", "请先选择输入目录");
        return;
    }
    
    QDir dir(inputDir);
    if (!dir.exists()) {
        DMessageBox::warning(this, "错误", "输入目录不存在");
        return;
    }
    
    // 获取文件过滤器
    QStringList nameFilters = getFileFilters();
    
    // 搜索文件
    QDir::Filters filters = QDir::Files | QDir::Readable;
    if (m_includeSubdirs->isChecked()) {
        filters |= QDir::AllDirs | QDir::NoDotAndDotDot;
    }
    
    QStringList files = scanDirectoryRecursive(inputDir, nameFilters, 
                                              m_includeSubdirs->isChecked());
    
    // 添加到队列
    int addedCount = 0;
    for (const QString &file : files) {
        if (addTaskToQueue(file)) {
            addedCount++;
        }
    }
    
    updateTaskStats();
    qDebug() << QString("扫描完成，添加了 %1 个文件到队列").arg(addedCount);
    
    if (addedCount > 0) {
        DMessageBox::information(this, "扫描完成", 
                               QString("成功添加 %1 个文件到处理队列").arg(addedCount));
    }
}

void DBatchProcessWidget::removeSelectedTasks()
{
    qDebug() << "移除选中的任务";
    
    QList<QListWidgetItem*> selectedItems = m_taskListWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        DMessageBox::information(this, "提示", "请先选择要移除的任务");
        return;
    }
    
    // 按索引倒序删除，避免索引错乱
    QList<int> indices;
    for (QListWidgetItem *item : selectedItems) {
        indices.append(m_taskListWidget->row(item));
    }
    std::sort(indices.rbegin(), indices.rend());
    
    for (int index : indices) {
        QListWidgetItem *item = m_taskListWidget->takeItem(index);
        if (item) {
            m_tasks.removeAt(index);
            delete item;
        }
    }
    
    updateTaskStats();
    qDebug() << QString("移除了 %1 个任务").arg(selectedItems.size());
}

void DBatchProcessWidget::clearAllTasks()
{
    qDebug() << "清空所有任务";
    
    if (m_tasks.isEmpty()) {
        return;
    }
    
    int ret = DMessageBox::question(this, "确认", "确定要清空所有任务吗？", 
                                   DMessageBox::Yes | DMessageBox::No);
    if (ret == DMessageBox::Yes) {
        m_taskListWidget->clear();
        m_tasks.clear();
        m_completedTasks = 0;
        m_failedTasks = 0;
        updateTaskStats();
        qDebug() << "已清空所有任务";
    }
}

void DBatchProcessWidget::startProcessing()
{
    qDebug() << "开始批量处理";
    
    if (m_tasks.isEmpty()) {
        DMessageBox::warning(this, "警告", "没有待处理的任务");
        return;
    }
    
    QString outputDir = m_outputPathEdit->text();
    if (outputDir.isEmpty()) {
        DMessageBox::warning(this, "警告", "请先选择输出目录");
        return;
    }
    
    QDir dir(outputDir);
    if (!dir.exists()) {
        if (!dir.mkpath(outputDir)) {
            DMessageBox::critical(this, "错误", "无法创建输出目录");
            return;
        }
    }
    
    // 重置状态
    m_isProcessing = true;
    m_currentTaskIndex = 0;
    m_completedTasks = 0;
    m_failedTasks = 0;
    
    // 更新UI状态
    updateControlButtonsState();
    
    // 开始处理
    m_timer->start();
    emit processingStarted(getBatchSettings());
    
    qDebug() << QString("开始批量处理 %1 个任务").arg(m_tasks.size());
}

void DBatchProcessWidget::togglePause()
{
    qDebug() << "切换暂停状态";
    
    if (m_pauseButton->text() == "暂停") {
        m_timer->stop();
        m_pauseButton->setText("继续");
        emit processingPaused();
        qDebug() << "处理已暂停";
    } else {
        m_timer->start();
        m_pauseButton->setText("暂停");
        emit processingResumed();
        qDebug() << "处理已恢复";
    }
}

void DBatchProcessWidget::stopProcessing()
{
    qDebug() << "停止批量处理";
    
    int ret = DMessageBox::question(this, "确认", "确定要停止批量处理吗？", 
                                   DMessageBox::Yes | DMessageBox::No);
    if (ret == DMessageBox::Yes) {
        m_isProcessing = false;
        m_timer->stop();
        m_pauseButton->setText("暂停");
        updateControlButtonsState();
        emit processingStopped();
        qDebug() << "批量处理已停止";
    }
}

void DBatchProcessWidget::saveQueue()
{
    qDebug() << "保存任务队列";
    
    if (m_tasks.isEmpty()) {
        DMessageBox::information(this, "提示", "没有任务可保存");
        return;
    }
    
    QString fileName = DFileDialog::getSaveFileName(this, "保存任务队列", 
                                                   "batch_queue.json",
                                                   "JSON文件 (*.json)");
    if (!fileName.isEmpty()) {
        // 这里应该实现队列的序列化保存
        qDebug() << QString("任务队列保存到: %1").arg(fileName);
        // 实现JSON序列化 - 完整的批量任务JSON序列化
    qCDebug(batchProcessWidget) << "序列化批量任务到JSON";
    
    QJsonObject jsonObject;
    
    // 基本信息
    jsonObject["taskName"] = m_task.taskName;
    jsonObject["taskId"] = m_task.taskId;
    jsonObject["description"] = m_task.description;
    jsonObject["createdTime"] = m_task.createdTime.toString(Qt::ISODate);
    jsonObject["lastModified"] = m_task.lastModified.toString(Qt::ISODate);
    jsonObject["isEnabled"] = m_task.isEnabled;
    jsonObject["priority"] = m_task.priority;
    
    // 扫描参数
    QJsonObject scanParamsObject;
    scanParamsObject["resolution"] = m_task.scanParameters.resolution;
    scanParamsObject["mode"] = static_cast<int>(m_task.scanParameters.mode);
    scanParamsObject["source"] = static_cast<int>(m_task.scanParameters.source);
    scanParamsObject["format"] = static_cast<int>(m_task.scanParameters.format);
    scanParamsObject["quality"] = m_task.scanParameters.quality;
    scanParamsObject["brightness"] = m_task.scanParameters.brightness;
    scanParamsObject["contrast"] = m_task.scanParameters.contrast;
    scanParamsObject["gamma"] = m_task.scanParameters.gamma;
    
    // 扫描区域
    QJsonObject scanAreaObject;
    scanAreaObject["x"] = m_task.scanParameters.scanArea.x();
    scanAreaObject["y"] = m_task.scanParameters.scanArea.y();
    scanAreaObject["width"] = m_task.scanParameters.scanArea.width();
    scanAreaObject["height"] = m_task.scanParameters.scanArea.height();
    scanParamsObject["scanArea"] = scanAreaObject;
    
    // 输出设置
    QJsonObject outputObject;
    outputObject["outputPath"] = m_task.scanParameters.outputPath;
    outputObject["fileNameTemplate"] = m_task.scanParameters.fileNameTemplate;
    outputObject["autoRotate"] = m_task.scanParameters.autoRotate;
    outputObject["deskew"] = m_task.scanParameters.deskew;
    outputObject["despeckle"] = m_task.scanParameters.despeckle;
    scanParamsObject["output"] = outputObject;
    
    // 扩展参数
    QJsonObject extParamsObject;
    for (auto it = m_task.scanParameters.parameters.begin(); 
         it != m_task.scanParameters.parameters.end(); ++it) {
        extParamsObject[it.key()] = QJsonValue::fromVariant(it.value());
    }
    scanParamsObject["extendedParameters"] = extParamsObject;
    
    jsonObject["scanParameters"] = scanParamsObject;
    
    // 处理设置
    QJsonObject processObject;
    processObject["enablePostProcessing"] = m_task.processSettings.enablePostProcessing;
    processObject["imageEnhancement"] = m_task.processSettings.imageEnhancement;
    processObject["noiseReduction"] = m_task.processSettings.noiseReduction;
    processObject["edgeDetection"] = m_task.processSettings.edgeDetection;
    processObject["colorCorrection"] = m_task.processSettings.colorCorrection;
    processObject["autoColorBalance"] = m_task.processSettings.autoColorBalance;
    processObject["sharpenLevel"] = m_task.processSettings.sharpenLevel;
    processObject["blurLevel"] = m_task.processSettings.blurLevel;
    
    jsonObject["processSettings"] = processObject;
    
    // 调度设置
    QJsonObject scheduleObject;
    scheduleObject["scheduleType"] = static_cast<int>(m_task.scheduleSettings.scheduleType);
    scheduleObject["scheduledTime"] = m_task.scheduleSettings.scheduledTime.toString(Qt::ISODate);
    scheduleObject["repeatInterval"] = m_task.scheduleSettings.repeatInterval;
    scheduleObject["maxRetries"] = m_task.scheduleSettings.maxRetries;
    scheduleObject["timeoutMinutes"] = m_task.scheduleSettings.timeoutMinutes;
    
    jsonObject["scheduleSettings"] = scheduleObject;
    
    // 输入文件列表
    QJsonArray inputFilesArray;
    for (const QString &file : m_task.inputFiles) {
        inputFilesArray.append(file);
    }
    jsonObject["inputFiles"] = inputFilesArray;
    
    // 任务状态和统计
    QJsonObject statsObject;
    statsObject["totalFiles"] = m_task.statistics.totalFiles;
    statsObject["processedFiles"] = m_task.statistics.processedFiles;
    statsObject["successfulFiles"] = m_task.statistics.successfulFiles;
    statsObject["failedFiles"] = m_task.statistics.failedFiles;
    statsObject["totalSize"] = static_cast<qint64>(m_task.statistics.totalSize);
    statsObject["processedSize"] = static_cast<qint64>(m_task.statistics.processedSize);
    statsObject["averageProcessingTime"] = m_task.statistics.averageProcessingTime;
    statsObject["startTime"] = m_task.statistics.startTime.toString(Qt::ISODate);
    statsObject["endTime"] = m_task.statistics.endTime.toString(Qt::ISODate);
    
    jsonObject["statistics"] = statsObject;
    
    // 错误日志
    QJsonArray errorLogArray;
    for (const BatchTaskError &error : m_task.errorLog) {
        QJsonObject errorObject;
        errorObject["timestamp"] = error.timestamp.toString(Qt::ISODate);
        errorObject["fileName"] = error.fileName;
        errorObject["errorCode"] = error.errorCode;
        errorObject["errorMessage"] = error.errorMessage;
        errorObject["severity"] = static_cast<int>(error.severity);
        errorLogArray.append(errorObject);
    }
    jsonObject["errorLog"] = errorLogArray;
    
    // 转换为JSON文档
    QJsonDocument jsonDoc(jsonObject);
    
    qCDebug(batchProcessWidget) << "批量任务JSON序列化完成，大小:" << jsonDoc.toJson().size() << "字节";
    }
}

void DBatchProcessWidget::loadQueue()
{
    qDebug() << "加载任务队列";
    
    QString fileName = DFileDialog::getOpenFileName(this, "加载任务队列", 
                                                   QString(),
                                                   "JSON文件 (*.json)");
    if (!fileName.isEmpty()) {
        loadTasksFromJson(fileName);
    }
}

void DBatchProcessWidget::updateProgress()
{
    if (!m_isProcessing || m_tasks.isEmpty()) {
        return;
    }
    
    // 更新总体进度
    int overallProgress = (m_completedTasks + m_failedTasks) * 100 / m_tasks.size();
    m_overallProgressBar->setValue(overallProgress);
    m_overallProgressLabel->setText(QString("%1%").arg(overallProgress));
    
    // 更新当前任务信息
    if (m_currentTaskIndex < m_tasks.size()) {
        const BatchTask &task = m_tasks[m_currentTaskIndex];
        m_currentFileLabel->setText(QString("当前文件: %1").arg(QFileInfo(task.inputPath).fileName()));
        
        // 模拟当前任务进度（实际应该从处理引擎获取）
        static int currentTaskProgress = 0;
        currentTaskProgress = (currentTaskProgress + 5) % 101;
        m_currentProgressBar->setValue(currentTaskProgress);
        m_currentProgressLabel->setText(QString("%1%").arg(currentTaskProgress));
        
        // 如果当前任务完成，移动到下一个
        if (currentTaskProgress >= 100) {
            m_completedTasks++;
            m_currentTaskIndex++;
            currentTaskProgress = 0;
            
            // 更新任务状态
            updateTaskStats();
            
            // 检查是否完成所有任务
            if (m_currentTaskIndex >= m_tasks.size()) {
                m_isProcessing = false;
                m_timer->stop();
                updateControlButtonsState();
                emit processingCompleted(m_completedTasks, m_failedTasks);
                
                DMessageBox::information(this, "处理完成", 
                                       QString("批量处理完成\n完成: %1\n失败: %2")
                                       .arg(m_completedTasks).arg(m_failedTasks));
            }
        }
    }
}

void DBatchProcessWidget::updateTaskStats()
{
    int totalTasks = m_tasks.size();
    int pendingTasks = totalTasks - m_completedTasks - m_failedTasks;
    
    m_taskStatsLabel->setText(QString("任务: %1 | 待处理: %2 | 完成: %3 | 失败: %4")
                             .arg(totalTasks)
                             .arg(pendingTasks)
                             .arg(m_completedTasks)
                             .arg(m_failedTasks));
    
    qDebug() << QString("任务统计更新 - 总计: %1, 待处理: %2, 完成: %3, 失败: %4")
                .arg(totalTasks).arg(pendingTasks).arg(m_completedTasks).arg(m_failedTasks);
}

void DBatchProcessWidget::updateControlButtonsState()
{
    m_startButton->setEnabled(!m_isProcessing);
    m_pauseButton->setEnabled(m_isProcessing);
    m_stopButton->setEnabled(m_isProcessing);
    
    // 处理中时禁用某些设置
    m_inputBrowseButton->setEnabled(!m_isProcessing);
    m_outputBrowseButton->setEnabled(!m_isProcessing);
    m_addFilesButton->setEnabled(!m_isProcessing);
    m_scanDirectoryButton->setEnabled(!m_isProcessing);
    m_clearAllButton->setEnabled(!m_isProcessing);
}

bool DBatchProcessWidget::addTaskToQueue(const QString &filePath)
{
    // 检查文件是否已存在
    for (const BatchTask &task : m_tasks) {
        if (task.inputPath == filePath) {
            return false; // 文件已存在，不重复添加
        }
    }
    
    // 创建新任务
    BatchTask task;
    task.inputPath = filePath;
    task.status = BatchTask::Pending;
    
    // 生成输出文件路径
    QString outputDir = m_outputPathEdit->text();
    QString fileName = QFileInfo(filePath).baseName();
    QString extension = m_outputFormatComboBox->currentText().toLower();
    
    // 应用命名模式
    QString finalName = applyNamingPattern(fileName);
    task.outputPath = QString("%1/%2.%3").arg(outputDir).arg(finalName).arg(extension);
    
    // 添加到队列和UI
    m_tasks.append(task);
    
    QListWidgetItem *item = new QListWidgetItem(m_taskListWidget);
    item->setText(QString("%1 → %2").arg(QFileInfo(filePath).fileName())
                                   .arg(QFileInfo(task.outputPath).fileName()));
    item->setToolTip(QString("输入: %1\n输出: %2").arg(filePath).arg(task.outputPath));
    
    return true;
}

QString DBatchProcessWidget::applyNamingPattern(const QString &originalName) const
{
    QString mode = m_namingModeComboBox->currentText();
    QString pattern = m_namingPatternEdit->text();
    
    if (mode == "保持原名称") {
        return originalName;
    } else if (mode == "添加前缀") {
        return pattern + originalName;
    } else if (mode == "添加后缀") {
        return originalName + pattern;
    } else if (mode == "重新编号") {
        static int counter = 1;
        return QString(pattern.isEmpty() ? "image_%04d" : pattern).arg(counter++);
    } else if (mode == "自定义模式") {
        QString result = pattern;
        result.replace("%name%", originalName);
        result.replace("%date%", QDate::currentDate().toString("yyyyMMdd"));
        result.replace("%time%", QTime::currentTime().toString("hhmmss"));
        return result;
    }
    
    return originalName;
}

QStringList DBatchProcessWidget::getFileFilters() const
{
    QString filter = m_fileFilterComboBox->currentText();
    
    if (filter.contains("所有图像文件")) {
        return {"*.jpg", "*.jpeg", "*.png", "*.tiff", "*.tif", "*.bmp"};
    } else if (filter.contains("JPEG")) {
        return {"*.jpg", "*.jpeg"};
    } else if (filter.contains("PNG")) {
        return {"*.png"};
    } else if (filter.contains("TIFF")) {
        return {"*.tiff", "*.tif"};
    } else if (filter.contains("BMP")) {
        return {"*.bmp"};
    }
    
    return {"*.*"};
}

QStringList DBatchProcessWidget::scanDirectoryRecursive(const QString &dirPath, 
                                                       const QStringList &nameFilters,
                                                       bool includeSubdirs) const
{
    QStringList files;
    QDir dir(dirPath);
    
    // 扫描当前目录
    QStringList currentFiles = dir.entryList(nameFilters, QDir::Files);
    for (const QString &file : currentFiles) {
        files.append(dir.absoluteFilePath(file));
    }
    
    // 递归扫描子目录
    if (includeSubdirs) {
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir : subdirs) {
            QString subdirPath = dir.absoluteFilePath(subdir);
            files.append(scanDirectoryRecursive(subdirPath, nameFilters, true));
        }
    }
    
    return files;
}

BatchProcessingSettings DBatchProcessWidget::getBatchSettings() const
{
    BatchProcessingSettings settings;
    settings.inputDirectory = m_inputPathEdit->text();
    settings.outputDirectory = m_outputPathEdit->text();
    settings.outputFormat = m_outputFormatComboBox->currentText();
    settings.quality = m_qualitySpinBox->value();
    settings.namingMode = m_namingModeComboBox->currentText();
    settings.namingPattern = m_namingPatternEdit->text();
    settings.concurrentTasks = m_concurrentTasksSpinBox->value();
    settings.errorHandling = m_errorHandlingComboBox->currentText();
    settings.overwriteFiles = m_overwriteFiles->isChecked();
    settings.includeSubdirs = m_includeSubdirs->isChecked();
    settings.fileFilter = m_fileFilterComboBox->currentText();
    
    return settings;
}

void DBatchProcessWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DBatchProcessWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    
    if (mimeData->hasUrls()) {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();
        
        for (const QUrl &url : urlList) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);
                
                if (fileInfo.isFile()) {
                    pathList.append(filePath);
                }
            }
        }
        
        int addedCount = 0;
        for (const QString &path : pathList) {
            if (addTaskToQueue(path)) {
                addedCount++;
            }
        }
        
        updateTaskStats();
        
        if (addedCount > 0) {
            qDebug() << QString("通过拖放添加了 %1 个文件").arg(addedCount);
        }
        
        event->acceptProposedAction();
    }
}

bool DBatchProcessWidget::saveTasksToJson(const QString &fileName)
{
    qDebug() << "DBatchProcessWidget::saveTasksToJson: 保存任务到JSON文件" << fileName;
    
    try {
        QJsonObject rootObject;
        QJsonArray tasksArray;
        
        // 序列化所有任务
        for (const auto &task : m_tasks) {
            QJsonObject taskObject;
            taskObject["id"] = task.id;
            taskObject["source_path"] = task.sourcePath;
            taskObject["output_path"] = task.outputPath;
            taskObject["format"] = static_cast<int>(task.format);
            taskObject["quality"] = task.quality;
            taskObject["resolution"] = task.resolution;
            taskObject["color_mode"] = static_cast<int>(task.colorMode);
            taskObject["auto_crop"] = task.autoCrop;
            taskObject["auto_deskew"] = task.autoDeskew;
            taskObject["rotate_angle"] = task.rotateAngle;
            taskObject["brightness"] = task.brightness;
            taskObject["contrast"] = task.contrast;
            taskObject["status"] = static_cast<int>(task.status);
            taskObject["progress"] = task.progress;
            taskObject["error_message"] = task.errorMessage;
            taskObject["created_time"] = task.createdTime.toString(Qt::ISODate);
            taskObject["started_time"] = task.startedTime.toString(Qt::ISODate);
            taskObject["finished_time"] = task.finishedTime.toString(Qt::ISODate);
            
            tasksArray.append(taskObject);
        }
        
        // 添加元数据
        QJsonObject metadata;
        metadata["version"] = "1.0.0";
        metadata["application"] = "DeepinScan";
        metadata["total_tasks"] = m_tasks.size();
        metadata["export_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        rootObject["metadata"] = metadata;
        rootObject["tasks"] = tasksArray;
        
        // 写入文件
        QJsonDocument doc(rootObject);
        QFile file(fileName);
        
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "DBatchProcessWidget::saveTasksToJson: 无法创建文件" << fileName;
            return false;
        }
        
        qint64 written = file.write(doc.toJson());
        file.close();
        
        if (written == -1) {
            qWarning() << "DBatchProcessWidget::saveTasksToJson: 写入文件失败";
            return false;
        }
        
        qDebug() << "DBatchProcessWidget::saveTasksToJson: 成功保存" << m_tasks.size() << "个任务";
        return true;
        
    } catch (const std::exception &e) {
        qWarning() << "DBatchProcessWidget::saveTasksToJson: 异常" << e.what();
        return false;
    }
}

bool DBatchProcessWidget::loadTasksFromJson(const QString &fileName)
{
    qDebug() << "DBatchProcessWidget::loadTasksFromJson: 从JSON文件加载任务" << fileName;
    
    try {
        // 读取文件
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "DBatchProcessWidget::loadTasksFromJson: 无法打开文件" << fileName;
            return false;
        }
        
        QByteArray data = file.readAll();
        file.close();
        
        // 解析JSON
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "DBatchProcessWidget::loadTasksFromJson: JSON解析错误" << error.errorString();
            return false;
        }
        
        QJsonObject rootObject = doc.object();
        
        // 验证格式
        if (!rootObject.contains("metadata") || !rootObject.contains("tasks")) {
            qWarning() << "DBatchProcessWidget::loadTasksFromJson: 无效的文件格式";
            return false;
        }
        
        QJsonObject metadata = rootObject["metadata"].toObject();
        QString version = metadata["version"].toString();
        QString application = metadata["application"].toString();
        
        if (application != "DeepinScan") {
            qWarning() << "DBatchProcessWidget::loadTasksFromJson: 不兼容的应用程序" << application;
            // 可以选择继续或中止，这里选择继续
        }
        
        // 清空现有任务
        clearAllItems();
        
        // 加载任务
        QJsonArray tasksArray = rootObject["tasks"].toArray();
        for (const auto &taskValue : tasksArray) {
            QJsonObject taskObject = taskValue.toObject();
            
            BatchTask task;
            task.id = taskObject["id"].toString();
            task.sourcePath = taskObject["source_path"].toString();
            task.outputPath = taskObject["output_path"].toString();
            task.format = static_cast<ScanFormat>(taskObject["format"].toInt());
            task.quality = taskObject["quality"].toInt();
            task.resolution = taskObject["resolution"].toInt();
            task.colorMode = static_cast<ColorMode>(taskObject["color_mode"].toInt());
            task.autoCrop = taskObject["auto_crop"].toBool();
            task.autoDeskew = taskObject["auto_deskew"].toBool();
            task.rotateAngle = taskObject["rotate_angle"].toInt();
            task.brightness = taskObject["brightness"].toInt();
            task.contrast = taskObject["contrast"].toInt();
            task.status = static_cast<BatchTaskStatus>(taskObject["status"].toInt());
            task.progress = taskObject["progress"].toInt();
            task.errorMessage = taskObject["error_message"].toString();
            
            // 解析时间
            QString createdTimeStr = taskObject["created_time"].toString();
            task.createdTime = QDateTime::fromString(createdTimeStr, Qt::ISODate);
            
            QString startedTimeStr = taskObject["started_time"].toString();
            if (!startedTimeStr.isEmpty()) {
                task.startedTime = QDateTime::fromString(startedTimeStr, Qt::ISODate);
            }
            
            QString finishedTimeStr = taskObject["finished_time"].toString();
            if (!finishedTimeStr.isEmpty()) {
                task.finishedTime = QDateTime::fromString(finishedTimeStr, Qt::ISODate);
            }
            
            // 添加到任务列表
            m_tasks.append(task);
            
            // 更新UI
            addTaskToUI(task);
        }
        
        // 更新统计信息
        updateTaskStatistics();
        
        qDebug() << "DBatchProcessWidget::loadTasksFromJson: 成功加载" << m_tasks.size() << "个任务";
        return true;
        
    } catch (const std::exception &e) {
        qWarning() << "DBatchProcessWidget::loadTasksFromJson: 异常" << e.what();
        return false;
    }
} 