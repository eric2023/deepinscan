/*
 * SPDX-FileCopyrightText: 2024-2025 eric2023
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "imageprocessingwidget.h"
#include <DGroupBox>
#include <DSlider>
#include <DSpinBox>
#include <DLabel>
#include <DPushButton>
#include <DComboBox>
#include <DCheckBox>
#include <DFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QTimer>
#include <QDebug>

DWIDGET_USE_NAMESPACE

DImageProcessingWidget::DImageProcessingWidget(QWidget *parent)
    : DWidget(parent)
    , m_processing(false)
    , m_resetTimer(new QTimer(this))
{
    qDebug() << "初始化图像处理组件";
    setupUI();
    connectSignals();
    
    // 设置重置计时器
    m_resetTimer->setSingleShot(true);
    m_resetTimer->setInterval(100);
    connect(m_resetTimer, &QTimer::timeout, this, &DImageProcessingWidget::resetToDefaults);
    
    qDebug() << "图像处理组件初始化完成";
}

DImageProcessingWidget::~DImageProcessingWidget()
{
    qDebug() << "销毁图像处理组件";
}

void DImageProcessingWidget::setupUI()
{
    qDebug() << "设置图像处理界面布局";
    
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 色彩调整组
    setupColorAdjustmentGroup();
    mainLayout->addWidget(m_colorGroup);
    
    // 几何变换组
    setupGeometryGroup();
    mainLayout->addWidget(m_geometryGroup);
    
    // 滤镜效果组
    setupFilterGroup();
    mainLayout->addWidget(m_filterGroup);
    
    // 操作按钮组
    setupActionButtons();
    mainLayout->addWidget(m_actionFrame);
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    mainLayout->addWidget(m_progressBar);
    
    mainLayout->addStretch();
    
    qDebug() << "图像处理界面布局设置完成";
}

void DImageProcessingWidget::setupColorAdjustmentGroup()
{
    qDebug() << "设置色彩调整组件";
    
    m_colorGroup = new DGroupBox("色彩调整", this);
    auto *layout = new QGridLayout(m_colorGroup);
    layout->setSpacing(8);
    
    // 亮度控制
    layout->addWidget(new DLabel("亮度:", this), 0, 0);
    m_brightnessSlider = new DSlider(Qt::Horizontal, this);
    m_brightnessSlider->setMinimum(-100);
    m_brightnessSlider->setMaximum(100);
    m_brightnessSlider->setValue(0);
    m_brightnessSlider->setToolTip("调整图像亮度 (-100 到 100)");
    layout->addWidget(m_brightnessSlider, 0, 1);
    
    m_brightnessSpinBox = new DSpinBox(this);
    m_brightnessSpinBox->setRange(-100, 100);
    m_brightnessSpinBox->setValue(0);
    m_brightnessSpinBox->setSuffix("%");
    layout->addWidget(m_brightnessSpinBox, 0, 2);
    
    // 对比度控制
    layout->addWidget(new DLabel("对比度:", this), 1, 0);
    m_contrastSlider = new DSlider(Qt::Horizontal, this);
    m_contrastSlider->setMinimum(-100);
    m_contrastSlider->setMaximum(100);
    m_contrastSlider->setValue(0);
    m_contrastSlider->setToolTip("调整图像对比度 (-100 到 100)");
    layout->addWidget(m_contrastSlider, 1, 1);
    
    m_contrastSpinBox = new DSpinBox(this);
    m_contrastSpinBox->setRange(-100, 100);
    m_contrastSpinBox->setValue(0);
    m_contrastSpinBox->setSuffix("%");
    layout->addWidget(m_contrastSpinBox, 1, 2);
    
    // 饱和度控制
    layout->addWidget(new DLabel("饱和度:", this), 2, 0);
    m_saturationSlider = new DSlider(Qt::Horizontal, this);
    m_saturationSlider->setMinimum(-100);
    m_saturationSlider->setMaximum(100);
    m_saturationSlider->setValue(0);
    m_saturationSlider->setToolTip("调整图像饱和度 (-100 到 100)");
    layout->addWidget(m_saturationSlider, 2, 1);
    
    m_saturationSpinBox = new DSpinBox(this);
    m_saturationSpinBox->setRange(-100, 100);
    m_saturationSpinBox->setValue(0);
    m_saturationSpinBox->setSuffix("%");
    layout->addWidget(m_saturationSpinBox, 2, 2);
    
    // 伽马控制
    layout->addWidget(new DLabel("伽马:", this), 3, 0);
    m_gammaSlider = new DSlider(Qt::Horizontal, this);
    m_gammaSlider->setMinimum(50);
    m_gammaSlider->setMaximum(300);
    m_gammaSlider->setValue(100);
    m_gammaSlider->setToolTip("调整图像伽马值 (0.5 到 3.0)");
    layout->addWidget(m_gammaSlider, 3, 1);
    
    m_gammaSpinBox = new DSpinBox(this);
    m_gammaSpinBox->setRange(50, 300);
    m_gammaSpinBox->setValue(100);
    m_gammaSpinBox->setSuffix("%");
    layout->addWidget(m_gammaSpinBox, 3, 2);
    
    qDebug() << "色彩调整组件设置完成";
}

void DImageProcessingWidget::setupGeometryGroup()
{
    qDebug() << "设置几何变换组件";
    
    m_geometryGroup = new DGroupBox("几何变换", this);
    auto *layout = new QGridLayout(m_geometryGroup);
    layout->setSpacing(8);
    
    // 旋转角度
    layout->addWidget(new DLabel("旋转:", this), 0, 0);
    m_rotationSlider = new DSlider(Qt::Horizontal, this);
    m_rotationSlider->setMinimum(-180);
    m_rotationSlider->setMaximum(180);
    m_rotationSlider->setValue(0);
    m_rotationSlider->setToolTip("旋转图像角度 (-180° 到 180°)");
    layout->addWidget(m_rotationSlider, 0, 1);
    
    m_rotationSpinBox = new DSpinBox(this);
    m_rotationSpinBox->setRange(-180, 180);
    m_rotationSpinBox->setValue(0);
    m_rotationSpinBox->setSuffix("°");
    layout->addWidget(m_rotationSpinBox, 0, 2);
    
    // 水平缩放
    layout->addWidget(new DLabel("水平缩放:", this), 1, 0);
    m_scaleXSlider = new DSlider(Qt::Horizontal, this);
    m_scaleXSlider->setMinimum(10);
    m_scaleXSlider->setMaximum(500);
    m_scaleXSlider->setValue(100);
    m_scaleXSlider->setToolTip("水平方向缩放比例 (10% 到 500%)");
    layout->addWidget(m_scaleXSlider, 1, 1);
    
    m_scaleXSpinBox = new DSpinBox(this);
    m_scaleXSpinBox->setRange(10, 500);
    m_scaleXSpinBox->setValue(100);
    m_scaleXSpinBox->setSuffix("%");
    layout->addWidget(m_scaleXSpinBox, 1, 2);
    
    // 垂直缩放
    layout->addWidget(new DLabel("垂直缩放:", this), 2, 0);
    m_scaleYSlider = new DSlider(Qt::Horizontal, this);
    m_scaleYSlider->setMinimum(10);
    m_scaleYSlider->setMaximum(500);
    m_scaleYSlider->setValue(100);
    m_scaleYSlider->setToolTip("垂直方向缩放比例 (10% 到 500%)");
    layout->addWidget(m_scaleYSlider, 2, 1);
    
    m_scaleYSpinBox = new DSpinBox(this);
    m_scaleYSpinBox->setRange(10, 500);
    m_scaleYSpinBox->setValue(100);
    m_scaleYSpinBox->setSuffix("%");
    layout->addWidget(m_scaleYSpinBox, 2, 2);
    
    // 锁定比例复选框
    m_lockAspectRatio = new DCheckBox("锁定宽高比", this);
    m_lockAspectRatio->setChecked(true);
    m_lockAspectRatio->setToolTip("保持图像宽高比不变");
    layout->addWidget(m_lockAspectRatio, 3, 0, 1, 3);
    
    qDebug() << "几何变换组件设置完成";
}

void DImageProcessingWidget::setupFilterGroup()
{
    qDebug() << "设置滤镜效果组件";
    
    m_filterGroup = new DGroupBox("滤镜效果", this);
    auto *layout = new QVBoxLayout(m_filterGroup);
    layout->setSpacing(8);
    
    // 滤镜类型选择
    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new DLabel("滤镜类型:", this));
    
    m_filterComboBox = new DComboBox(this);
    m_filterComboBox->addItems({
        "无滤镜",
        "模糊",
        "锐化",
        "浮雕",
        "边缘检测",
        "噪点消除",
        "黑白",
        "深褐色",
        "反色"
    });
    m_filterComboBox->setCurrentText("无滤镜");
    m_filterComboBox->setToolTip("选择要应用的滤镜效果");
    filterLayout->addWidget(m_filterComboBox);
    filterLayout->addStretch();
    
    layout->addLayout(filterLayout);
    
    // 滤镜强度
    auto *intensityLayout = new QHBoxLayout();
    intensityLayout->addWidget(new DLabel("强度:", this));
    
    m_filterIntensity = new DSlider(Qt::Horizontal, this);
    m_filterIntensity->setMinimum(0);
    m_filterIntensity->setMaximum(100);
    m_filterIntensity->setValue(50);
    m_filterIntensity->setToolTip("调整滤镜效果强度 (0% 到 100%)");
    intensityLayout->addWidget(m_filterIntensity);
    
    m_filterIntensitySpinBox = new DSpinBox(this);
    m_filterIntensitySpinBox->setRange(0, 100);
    m_filterIntensitySpinBox->setValue(50);
    m_filterIntensitySpinBox->setSuffix("%");
    intensityLayout->addWidget(m_filterIntensitySpinBox);
    
    layout->addLayout(intensityLayout);
    
    qDebug() << "滤镜效果组件设置完成";
}

void DImageProcessingWidget::setupActionButtons()
{
    qDebug() << "设置操作按钮";
    
    m_actionFrame = new DFrame(this);
    auto *layout = new QHBoxLayout(m_actionFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 应用处理按钮
    m_applyButton = new DPushButton("应用处理", this);
    m_applyButton->setToolTip("应用当前所有图像处理设置");
    layout->addWidget(m_applyButton);
    
    // 重置按钮
    m_resetButton = new DPushButton("重置", this);
    m_resetButton->setToolTip("重置所有处理参数到默认值");
    layout->addWidget(m_resetButton);
    
    // 预览按钮
    m_previewButton = new DPushButton("实时预览", this);
    m_previewButton->setCheckable(true);
    m_previewButton->setToolTip("开启/关闭实时预览效果");
    layout->addWidget(m_previewButton);
    
    layout->addStretch();
    
    qDebug() << "操作按钮设置完成";
}

void DImageProcessingWidget::connectSignals()
{
    qDebug() << "连接图像处理信号";
    
    // 色彩调整信号连接
    connect(m_brightnessSlider, QOverload<int>::of(&DSlider::valueChanged),
            m_brightnessSpinBox, &DSpinBox::setValue);
    connect(m_brightnessSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_brightnessSlider, &DSlider::setValue);
    connect(m_brightnessSlider, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onColorAdjustmentChanged);
    
    connect(m_contrastSlider, QOverload<int>::of(&DSlider::valueChanged),
            m_contrastSpinBox, &DSpinBox::setValue);
    connect(m_contrastSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_contrastSlider, &DSlider::setValue);
    connect(m_contrastSlider, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onColorAdjustmentChanged);
    
    connect(m_saturationSlider, QOverload<int>::of(&DSlider::valueChanged),
            m_saturationSpinBox, &DSpinBox::setValue);
    connect(m_saturationSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_saturationSlider, &DSlider::setValue);
    connect(m_saturationSlider, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onColorAdjustmentChanged);
    
    connect(m_gammaSlider, QOverload<int>::of(&DSlider::valueChanged),
            m_gammaSpinBox, &DSpinBox::setValue);
    connect(m_gammaSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_gammaSlider, &DSlider::setValue);
    connect(m_gammaSlider, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onColorAdjustmentChanged);
    
    // 几何变换信号连接
    connect(m_rotationSlider, QOverload<int>::of(&DSlider::valueChanged),
            m_rotationSpinBox, &DSpinBox::setValue);
    connect(m_rotationSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_rotationSlider, &DSlider::setValue);
    connect(m_rotationSlider, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onGeometryChanged);
    
    connect(m_scaleXSlider, QOverload<int>::of(&DSlider::valueChanged),
            m_scaleXSpinBox, &DSpinBox::setValue);
    connect(m_scaleXSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_scaleXSlider, &DSlider::setValue);
    connect(m_scaleXSlider, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onScaleXChanged);
    
    connect(m_scaleYSlider, QOverload<int>::of(&DSlider::valueChanged),
            m_scaleYSpinBox, &DSpinBox::setValue);
    connect(m_scaleYSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_scaleYSlider, &DSlider::setValue);
    connect(m_scaleYSlider, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onScaleYChanged);
    
    connect(m_lockAspectRatio, &DCheckBox::toggled,
            this, &DImageProcessingWidget::onAspectRatioLockChanged);
    
    // 滤镜效果信号连接
    connect(m_filterComboBox, QOverload<const QString &>::of(&DComboBox::currentTextChanged),
            this, &DImageProcessingWidget::onFilterChanged);
    
    connect(m_filterIntensity, QOverload<int>::of(&DSlider::valueChanged),
            m_filterIntensitySpinBox, &DSpinBox::setValue);
    connect(m_filterIntensitySpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            m_filterIntensity, &DSlider::setValue);
    connect(m_filterIntensity, &DSlider::valueChanged,
            this, &DImageProcessingWidget::onFilterChanged);
    
    // 操作按钮信号连接
    connect(m_applyButton, &DPushButton::clicked,
            this, &DImageProcessingWidget::applyProcessing);
    connect(m_resetButton, &DPushButton::clicked,
            this, &DImageProcessingWidget::resetParameters);
    connect(m_previewButton, &DPushButton::toggled,
            this, &DImageProcessingWidget::onPreviewToggled);
    
    qDebug() << "图像处理信号连接完成";
}

void DImageProcessingWidget::onColorAdjustmentChanged()
{
    qDebug() << "色彩调整参数变化";
    
    ImageProcessingUIParameters params = getCurrentParameters();
    emit parametersChanged(params);
    
    if (m_previewButton->isChecked()) {
        emit previewRequested(params);
    }
}

void DImageProcessingWidget::onGeometryChanged()
{
    qDebug() << "几何变换参数变化";
    
    ImageProcessingUIParameters params = getCurrentParameters();
    emit parametersChanged(params);

    if (m_previewButton->isChecked()) {
        emit previewRequested(params);
    }
}

void DImageProcessingWidget::onScaleXChanged(int value)
{
    qDebug() << QString("水平缩放变化: %1%").arg(value);
    
    if (m_lockAspectRatio->isChecked()) {
        // 锁定宽高比时同步Y轴缩放
        m_scaleYSlider->blockSignals(true);
        m_scaleYSpinBox->blockSignals(true);
        m_scaleYSlider->setValue(value);
        m_scaleYSpinBox->setValue(value);
        m_scaleYSlider->blockSignals(false);
        m_scaleYSpinBox->blockSignals(false);
    }
    
    onGeometryChanged();
}

void DImageProcessingWidget::onScaleYChanged(int value)
{
    qDebug() << QString("垂直缩放变化: %1%").arg(value);
    
    if (m_lockAspectRatio->isChecked()) {
        // 锁定宽高比时同步X轴缩放
        m_scaleXSlider->blockSignals(true);
        m_scaleXSpinBox->blockSignals(true);
        m_scaleXSlider->setValue(value);
        m_scaleXSpinBox->setValue(value);
        m_scaleXSlider->blockSignals(false);
        m_scaleXSpinBox->blockSignals(false);
    }
    
    onGeometryChanged();
}

void DImageProcessingWidget::onAspectRatioLockChanged(bool locked)
{
    qDebug() << QString("宽高比锁定状态: %1").arg(locked ? "已锁定" : "已解锁");
    
    if (locked) {
        // 锁定时将Y轴缩放同步到X轴
        int xScale = m_scaleXSlider->value();
        m_scaleYSlider->blockSignals(true);
        m_scaleYSpinBox->blockSignals(true);
        m_scaleYSlider->setValue(xScale);
        m_scaleYSpinBox->setValue(xScale);
        m_scaleYSlider->blockSignals(false);
        m_scaleYSpinBox->blockSignals(false);
        
        onGeometryChanged();
    }
}

void DImageProcessingWidget::onFilterChanged()
{
    qDebug() << QString("滤镜效果变化: %1, 强度: %2%")
                .arg(m_filterComboBox->currentText())
                .arg(m_filterIntensity->value());
    
        ImageProcessingUIParameters params = getCurrentParameters();
    emit parametersChanged(params);

    if (m_previewButton->isChecked()) {
        emit previewRequested(params);
    }
}

void DImageProcessingWidget::onPreviewToggled(bool enabled)
{
    qDebug() << QString("实时预览状态: %1").arg(enabled ? "开启" : "关闭");
    
    if (enabled) {
        ImageProcessingUIParameters params = getCurrentParameters();
        emit previewRequested(params);
    } else {
        emit previewCancelled();
    }
}

void DImageProcessingWidget::applyProcessing()
{
    qDebug() << "开始应用图像处理";
    
    if (m_processing) {
        qWarning() << "正在处理中，忽略重复请求";
        return;
    }
    
    setProcessing(true);
    
    ImageProcessingUIParameters params = getCurrentParameters();
    emit processingRequested(params);
}

void DImageProcessingWidget::resetParameters()
{
    qDebug() << "重置图像处理参数";
    
    m_resetTimer->start();
}

void DImageProcessingWidget::resetToDefaults()
{
    qDebug() << "重置参数到默认值";
    
    // 阻塞所有信号以避免重复触发
    const QList<QWidget*> widgets = {
        m_brightnessSlider, m_brightnessSpinBox,
        m_contrastSlider, m_contrastSpinBox,
        m_saturationSlider, m_saturationSpinBox,
        m_gammaSlider, m_gammaSpinBox,
        m_rotationSlider, m_rotationSpinBox,
        m_scaleXSlider, m_scaleXSpinBox,
        m_scaleYSlider, m_scaleYSpinBox,
        m_filterIntensity, m_filterIntensitySpinBox
    };
    
    for (auto *widget : widgets) {
        widget->blockSignals(true);
    }
    
    // 重置色彩调整
    m_brightnessSlider->setValue(0);
    m_brightnessSpinBox->setValue(0);
    m_contrastSlider->setValue(0);
    m_contrastSpinBox->setValue(0);
    m_saturationSlider->setValue(0);
    m_saturationSpinBox->setValue(0);
    m_gammaSlider->setValue(100);
    m_gammaSpinBox->setValue(100);
    
    // 重置几何变换
    m_rotationSlider->setValue(0);
    m_rotationSpinBox->setValue(0);
    m_scaleXSlider->setValue(100);
    m_scaleXSpinBox->setValue(100);
    m_scaleYSlider->setValue(100);
    m_scaleYSpinBox->setValue(100);
    m_lockAspectRatio->setChecked(true);
    
    // 重置滤镜效果
    m_filterComboBox->setCurrentText("无滤镜");
    m_filterIntensity->setValue(50);
    m_filterIntensitySpinBox->setValue(50);
    
    // 恢复信号
    for (auto *widget : widgets) {
        widget->blockSignals(false);
    }
    
    qDebug() << "参数重置完成";
    
    ImageProcessingUIParameters params = getCurrentParameters();
    emit parametersChanged(params);
    emit parametersReset();
}

ImageProcessingUIParameters DImageProcessingWidget::getCurrentParameters() const
{
    ImageProcessingUIParameters params;
    
    // 色彩调整参数
    params.brightness = m_brightnessSlider->value();
    params.contrast = m_contrastSlider->value();
    params.saturation = m_saturationSlider->value();
    params.gamma = m_gammaSlider->value() / 100.0;
    
    // 几何变换参数
    params.rotation = m_rotationSlider->value();
    params.scaleX = m_scaleXSlider->value() / 100.0;
    params.scaleY = m_scaleYSlider->value() / 100.0;
    params.lockAspectRatio = m_lockAspectRatio->isChecked();
    
    // 滤镜参数
    params.filterType = m_filterComboBox->currentText();
    params.filterIntensity = m_filterIntensity->value() / 100.0;
    
    return params;
}

void DImageProcessingWidget::setParameters(const ImageProcessingUIParameters &params)
{
    qDebug() << "设置图像处理参数";
    
    // 阻塞信号避免循环触发
    const QList<QWidget*> widgets = {
        m_brightnessSlider, m_brightnessSpinBox,
        m_contrastSlider, m_contrastSpinBox,
        m_saturationSlider, m_saturationSpinBox,
        m_gammaSlider, m_gammaSpinBox,
        m_rotationSlider, m_rotationSpinBox,
        m_scaleXSlider, m_scaleXSpinBox,
        m_scaleYSlider, m_scaleYSpinBox,
        m_filterIntensity, m_filterIntensitySpinBox
    };
    
    for (auto *widget : widgets) {
        widget->blockSignals(true);
    }
    
    // 设置色彩调整参数
    m_brightnessSlider->setValue(params.brightness);
    m_brightnessSpinBox->setValue(params.brightness);
    m_contrastSlider->setValue(params.contrast);
    m_contrastSpinBox->setValue(params.contrast);
    m_saturationSlider->setValue(params.saturation);
    m_saturationSpinBox->setValue(params.saturation);
    m_gammaSlider->setValue(static_cast<int>(params.gamma * 100));
    m_gammaSpinBox->setValue(static_cast<int>(params.gamma * 100));
    
    // 设置几何变换参数
    m_rotationSlider->setValue(params.rotation);
    m_rotationSpinBox->setValue(params.rotation);
    m_scaleXSlider->setValue(static_cast<int>(params.scaleX * 100));
    m_scaleXSpinBox->setValue(static_cast<int>(params.scaleX * 100));
    m_scaleYSlider->setValue(static_cast<int>(params.scaleY * 100));
    m_scaleYSpinBox->setValue(static_cast<int>(params.scaleY * 100));
    m_lockAspectRatio->setChecked(params.lockAspectRatio);
    
    // 设置滤镜参数
    m_filterComboBox->setCurrentText(params.filterType);
    m_filterIntensity->setValue(static_cast<int>(params.filterIntensity * 100));
    m_filterIntensitySpinBox->setValue(static_cast<int>(params.filterIntensity * 100));
    
    // 恢复信号
    for (auto *widget : widgets) {
        widget->blockSignals(false);
    }
    
    qDebug() << "图像处理参数设置完成";
}

void DImageProcessingWidget::setProcessing(bool processing)
{
    qDebug() << QString("设置处理状态: %1").arg(processing ? "处理中" : "空闲");
    
    m_processing = processing;
    
    // 更新UI状态
    m_applyButton->setEnabled(!processing);
    m_resetButton->setEnabled(!processing);
    m_previewButton->setEnabled(!processing);
    
    // 显示/隐藏进度条
    m_progressBar->setVisible(processing);
    if (processing) {
        m_progressBar->setValue(0);
    }
}

void DImageProcessingWidget::updateProgress(int percentage)
{
    qDebug() << QString("更新处理进度: %1%").arg(percentage);
    
    m_progressBar->setValue(percentage);
    
    if (percentage >= 100) {
        QTimer::singleShot(1000, this, [this]() {
            setProcessing(false);
        });
    }
}

void DImageProcessingWidget::setPreviewEnabled(bool enabled)
{
    qDebug() << QString("设置预览状态: %1").arg(enabled ? "启用" : "禁用");
    
    m_previewButton->setChecked(enabled);
} 