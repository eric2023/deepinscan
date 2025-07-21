// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scancontrolwidget.h"
#include <DApplication>
#include <DMessageBox>
#include <DGroupBox>
#include <DFontSizeManager>
#include <QGridLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QDebug>

DWIDGET_USE_NAMESPACE

ScanControlWidget::ScanControlWidget(QWidget *parent)
    : DWidget(parent)
    , m_currentDevice(nullptr)
    , m_resolutionCombo(nullptr)
    , m_colorModeCombo(nullptr)
    , m_scanSourceCombo(nullptr)
    , m_formatCombo(nullptr)
    , m_xSpinBox(nullptr)
    , m_ySpinBox(nullptr)
    , m_widthSpinBox(nullptr)
    , m_heightSpinBox(nullptr)
    , m_fullAreaButton(nullptr)
    , m_customAreaButton(nullptr)
    , m_brightnessSlider(nullptr)
    , m_contrastSlider(nullptr)
    , m_gammaSlider(nullptr)
    , m_autoColorCorrection(nullptr)
    , m_dustRemoval(nullptr)
    , m_noiseReduction(nullptr)
    , m_qualitySpinBox(nullptr)
    , m_multiSamplingCheck(nullptr)
    , m_batchCountSpinBox(nullptr)
    , m_scanButton(nullptr)
    , m_cancelButton(nullptr)
    , m_previewButton(nullptr)
    , m_resetButton(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_mainLayout(nullptr)
{
    qDebug() << "ScanControlWidget: 初始化扫描控制界面组件";
    initUI();
    initConnections();
    resetToDefaults();
}

ScanControlWidget::~ScanControlWidget()
{
    qDebug() << "ScanControlWidget: 销毁扫描控制界面组件";
}

void ScanControlWidget::setCurrentDevice(DScannerDevice *device)
{
    if (m_currentDevice == device) {
        return;
    }
    
    qDebug() << "ScanControlWidget: 设置当前扫描设备";
    m_currentDevice = device;
    
    // 更新设备能力相关的UI
    updateDeviceCapabilities();
    
    // 启用或禁用控件
    setEnabled(device != nullptr);
}

ScanParameters ScanControlWidget::getScanParameters() const
{
    ScanParameters params;
    
    // 基本参数
    params.resolution = m_resolutionCombo->currentData().toInt();
    params.colorMode = static_cast<ColorMode>(m_colorModeCombo->currentData().toInt());
    // ScanParameters中没有scanSource字段，暂时注释掉
    // params.scanSource = static_cast<ScanSource>(m_scanSourceCombo->currentData().toInt());
    params.format = static_cast<ImageFormat>(m_formatCombo->currentData().toInt());
    
    // 扫描区域
    params.area.x = m_xSpinBox->value();
    params.area.y = m_ySpinBox->value();
    params.area.width = m_widthSpinBox->value();
    params.area.height = m_heightSpinBox->value();
    
    // 图像增强
    params.brightness = m_brightnessSlider->value();
    params.contrast = m_contrastSlider->value();
    params.gamma = m_gammaSlider->value() / 100.0;
    // 使用实际存在的字段
    params.denoiseEnabled = m_noiseReduction->isChecked();
    
    // 高级设置
    params.quality = m_qualitySpinBox->value();
    
    return params;
}

void ScanControlWidget::setScanParameters(const ScanParameters &params)
{
    qDebug() << "ScanControlWidget: 设置扫描参数";
    
    // 基本参数
    for (int i = 0; i < m_resolutionCombo->count(); ++i) {
        if (m_resolutionCombo->itemData(i).toInt() == params.resolution) {
            m_resolutionCombo->setCurrentIndex(i);
            break;
        }
    }
    
    for (int i = 0; i < m_colorModeCombo->count(); ++i) {
        if (m_colorModeCombo->itemData(i).toInt() == static_cast<int>(params.colorMode)) {
            m_colorModeCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // ScanParameters中没有scanSource字段，暂时注释掉
    /*
    for (int i = 0; i < m_scanSourceCombo->count(); ++i) {
        if (m_scanSourceCombo->itemData(i).toInt() == static_cast<int>(params.scanSource)) {
            m_scanSourceCombo->setCurrentIndex(i);
            break;
        }
    }
    */
    
    for (int i = 0; i < m_formatCombo->count(); ++i) {
        if (m_formatCombo->itemData(i).toInt() == static_cast<int>(params.format)) {
            m_formatCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // 扫描区域
    m_xSpinBox->setValue(params.area.x);
    m_ySpinBox->setValue(params.area.y);
    m_widthSpinBox->setValue(params.area.width);
    m_heightSpinBox->setValue(params.area.height);
    
    // 图像增强
    m_brightnessSlider->setValue(params.brightness);
    m_contrastSlider->setValue(params.contrast);
    m_gammaSlider->setValue(static_cast<int>(params.gamma * 100));
    // 使用实际存在的字段
    m_noiseReduction->setChecked(params.denoiseEnabled);
    
    // 高级设置
    m_qualitySpinBox->setValue(params.quality);
}

void ScanControlWidget::setEnabled(bool enabled)
{
    DWidget::setEnabled(enabled);
    
    // 更新状态标签
    if (!enabled) {
        m_statusLabel->setText("请先连接扫描设备");
        m_progressBar->hide();
    } else {
        m_statusLabel->setText("准备就绪");
        m_progressBar->hide();
    }
}

void ScanControlWidget::resetToDefaults()
{
    qDebug() << "ScanControlWidget: 重置为默认设置";
    
    // 设置默认参数
    ScanParameters defaultParams;
    defaultParams.resolution = 300;
    defaultParams.colorMode = ColorMode::Color;
    // ScanParameters中没有scanSource字段，暂时注释掉
    // defaultParams.scanSource = ScanSource::Flatbed;
    defaultParams.format = ImageFormat::JPEG;
    defaultParams.area = {0.0, 0.0, 210.0, 297.0}; // A4尺寸
    defaultParams.brightness = 0;
    defaultParams.contrast = 0;
    defaultParams.gamma = 1.0;
    defaultParams.denoiseEnabled = true;
    defaultParams.quality = 80;
    
    setScanParameters(defaultParams);
}

void ScanControlWidget::initUI()
{
    qDebug() << "ScanControlWidget: 初始化用户界面";
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // 创建各个设置组
    m_mainLayout->addWidget(createBasicSettingsGroup());
    m_mainLayout->addWidget(createScanAreaGroup());
    m_mainLayout->addWidget(createImageEnhancementGroup());
    m_mainLayout->addWidget(createAdvancedSettingsGroup());
    m_mainLayout->addWidget(createControlButtonsGroup());
    
    // 添加弹性空间
    m_mainLayout->addStretch();
    
    setLayout(m_mainLayout);
}

void ScanControlWidget::initConnections()
{
    qDebug() << "ScanControlWidget: 初始化信号连接";
    
    // 基本设置信号连接
    connect(m_resolutionCombo, QOverload<int>::of(&DComboBox::currentIndexChanged),
            this, &ScanControlWidget::onResolutionChanged);
    connect(m_colorModeCombo, QOverload<int>::of(&DComboBox::currentIndexChanged),
            this, &ScanControlWidget::onColorModeChanged);
    connect(m_scanSourceCombo, QOverload<int>::of(&DComboBox::currentIndexChanged),
            this, &ScanControlWidget::onScanSourceChanged);
    
    // 扫描区域信号连接
    connect(m_xSpinBox, QOverload<double>::of(&DDoubleSpinBox::valueChanged),
            this, &ScanControlWidget::onScanAreaChanged);
    connect(m_ySpinBox, QOverload<double>::of(&DDoubleSpinBox::valueChanged),
            this, &ScanControlWidget::onScanAreaChanged);
    connect(m_widthSpinBox, QOverload<double>::of(&DDoubleSpinBox::valueChanged),
            this, &ScanControlWidget::onScanAreaChanged);
    connect(m_heightSpinBox, QOverload<double>::of(&DDoubleSpinBox::valueChanged),
            this, &ScanControlWidget::onScanAreaChanged);
    connect(m_fullAreaButton, &DPushButton::clicked,
            this, [this]() {
                m_xSpinBox->setValue(0.0);
                m_ySpinBox->setValue(0.0);
                m_widthSpinBox->setValue(210.0);
                m_heightSpinBox->setValue(297.0);
            });
    
    // 图像增强信号连接
    connect(m_brightnessSlider, &DSlider::valueChanged,
            this, &ScanControlWidget::onImageEnhancementChanged);
    connect(m_contrastSlider, &DSlider::valueChanged,
            this, &ScanControlWidget::onImageEnhancementChanged);
    connect(m_gammaSlider, &DSlider::valueChanged,
            this, &ScanControlWidget::onImageEnhancementChanged);
    connect(m_autoColorCorrection, &DCheckBox::toggled,
            this, &ScanControlWidget::onImageEnhancementChanged);
    connect(m_dustRemoval, &DCheckBox::toggled,
            this, &ScanControlWidget::onImageEnhancementChanged);
    connect(m_noiseReduction, &DCheckBox::toggled,
            this, &ScanControlWidget::onImageEnhancementChanged);
    
    // 控制按钮信号连接
    connect(m_scanButton, &DPushButton::clicked,
            this, &ScanControlWidget::onStartScanClicked);
    connect(m_cancelButton, &DPushButton::clicked,
            this, &ScanControlWidget::onCancelScanClicked);
    connect(m_previewButton, &DPushButton::clicked,
            this, &ScanControlWidget::onPreviewClicked);
    connect(m_resetButton, &DPushButton::clicked,
            this, &ScanControlWidget::onResetClicked);
}

QWidget *ScanControlWidget::createBasicSettingsGroup()
{
    auto *group = new DGroupBox("基本设置", this);
    auto *layout = new QFormLayout(group);
    
    // 分辨率设置
    m_resolutionCombo = new DComboBox(this);
    m_resolutionCombo->addItem("75 DPI", 75);
    m_resolutionCombo->addItem("150 DPI", 150);
    m_resolutionCombo->addItem("300 DPI", 300);
    m_resolutionCombo->addItem("600 DPI", 600);
    m_resolutionCombo->addItem("1200 DPI", 1200);
    m_resolutionCombo->setCurrentIndex(2); // 默认300 DPI
    layout->addRow("分辨率:", m_resolutionCombo);
    
    // 颜色模式设置
    m_colorModeCombo = new DComboBox(this);
    m_colorModeCombo->addItem("黑白", static_cast<int>(ColorMode::Monochrome));
    m_colorModeCombo->addItem("灰度", static_cast<int>(ColorMode::Grayscale));
    m_colorModeCombo->addItem("彩色", static_cast<int>(ColorMode::Color));
    m_colorModeCombo->setCurrentIndex(2); // 默认彩色
    layout->addRow("颜色模式:", m_colorModeCombo);
    
    // 扫描源设置
    m_scanSourceCombo = new DComboBox(this);
    // 暂时使用整数值，因为ScanSource类型不存在
    m_scanSourceCombo->addItem("平板", 0);  // ScanSource::Flatbed
    m_scanSourceCombo->addItem("自动进纸器", 1);  // ScanSource::ADF  
    m_scanSourceCombo->addItem("透明材料适配器", 2);  // ScanSource::Transparency
    m_scanSourceCombo->setCurrentIndex(0); // 默认平板
    layout->addRow("扫描源:", m_scanSourceCombo);
    
    // 文件格式设置
    m_formatCombo = new DComboBox(this);
    m_formatCombo->addItem("JPEG", static_cast<int>(ImageFormat::JPEG));
    m_formatCombo->addItem("PNG", static_cast<int>(ImageFormat::PNG));
    m_formatCombo->addItem("TIFF", static_cast<int>(ImageFormat::TIFF));
    m_formatCombo->addItem("PDF", static_cast<int>(ImageFormat::PDF));
    m_formatCombo->setCurrentIndex(0); // 默认JPEG
    layout->addRow("输出格式:", m_formatCombo);
    
    return group;
}

QWidget *ScanControlWidget::createAdvancedSettingsGroup()
{
    auto *group = new DGroupBox("高级设置", this);
    auto *layout = new QFormLayout(group);
    
    // 图像质量设置
    m_qualitySpinBox = new DSpinBox(this);
    m_qualitySpinBox->setRange(1, 100);
    m_qualitySpinBox->setValue(80);
    m_qualitySpinBox->setSuffix("%");
    layout->addRow("图像质量:", m_qualitySpinBox);
    
    // 多重采样
    m_multiSamplingCheck = new DCheckBox("启用多重采样", this);
    layout->addRow("", m_multiSamplingCheck);
    
    // 批量扫描数量
    m_batchCountSpinBox = new DSpinBox(this);
    m_batchCountSpinBox->setRange(1, 100);
    m_batchCountSpinBox->setValue(1);
    layout->addRow("批量数量:", m_batchCountSpinBox);
    
    return group;
}

QWidget *ScanControlWidget::createScanAreaGroup()
{
    auto *group = new DGroupBox("扫描区域", this);
    auto *layout = new QGridLayout(group);
    
    // 区域坐标设置
    layout->addWidget(new DLabel("X (mm):", this), 0, 0);
    m_xSpinBox = new DDoubleSpinBox(this);
    m_xSpinBox->setRange(0.0, 300.0);
    m_xSpinBox->setDecimals(1);
    m_xSpinBox->setValue(0.0);
    layout->addWidget(m_xSpinBox, 0, 1);
    
    layout->addWidget(new DLabel("Y (mm):", this), 0, 2);
    m_ySpinBox = new DDoubleSpinBox(this);
    m_ySpinBox->setRange(0.0, 400.0);
    m_ySpinBox->setDecimals(1);
    m_ySpinBox->setValue(0.0);
    layout->addWidget(m_ySpinBox, 0, 3);
    
    layout->addWidget(new DLabel("宽度 (mm):", this), 1, 0);
    m_widthSpinBox = new DDoubleSpinBox(this);
    m_widthSpinBox->setRange(1.0, 300.0);
    m_widthSpinBox->setDecimals(1);
    m_widthSpinBox->setValue(210.0);
    layout->addWidget(m_widthSpinBox, 1, 1);
    
    layout->addWidget(new DLabel("高度 (mm):", this), 1, 2);
    m_heightSpinBox = new DDoubleSpinBox(this);
    m_heightSpinBox->setRange(1.0, 400.0);
    m_heightSpinBox->setDecimals(1);
    m_heightSpinBox->setValue(297.0);
    layout->addWidget(m_heightSpinBox, 1, 3);
    
    // 快捷按钮
    auto *buttonLayout = new QHBoxLayout();
    m_fullAreaButton = new DPushButton("整个平板", this);
    m_customAreaButton = new DPushButton("自定义区域", this);
    
    buttonLayout->addWidget(m_fullAreaButton);
    buttonLayout->addWidget(m_customAreaButton);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout, 2, 0, 1, 4);
    
    return group;
}

QWidget *ScanControlWidget::createImageEnhancementGroup()
{
    auto *group = new DGroupBox("图像增强", this);
    auto *layout = new QGridLayout(group);
    
    // 亮度调节
    layout->addWidget(new DLabel("亮度:", this), 0, 0);
    m_brightnessSlider = new DSlider(Qt::Horizontal, this);
    m_brightnessSlider->setMinimum(-100);
    m_brightnessSlider->setMaximum(100);
    m_brightnessSlider->setValue(0);
    layout->addWidget(m_brightnessSlider, 0, 1, 1, 2);
    
    // 对比度调节
    layout->addWidget(new DLabel("对比度:", this), 1, 0);
    m_contrastSlider = new DSlider(Qt::Horizontal, this);
    m_contrastSlider->setMinimum(-100);
    m_contrastSlider->setMaximum(100);
    m_contrastSlider->setValue(0);
    layout->addWidget(m_contrastSlider, 1, 1, 1, 2);
    
    // 伽马调节
    layout->addWidget(new DLabel("伽马:", this), 2, 0);
    m_gammaSlider = new DSlider(Qt::Horizontal, this);
    m_gammaSlider->setMinimum(10);
    m_gammaSlider->setMaximum(300);
    m_gammaSlider->setValue(100);
    layout->addWidget(m_gammaSlider, 2, 1, 1, 2);
    
    // 自动增强选项
    m_autoColorCorrection = new DCheckBox("自动色彩校正", this);
    m_autoColorCorrection->setChecked(true);
    layout->addWidget(m_autoColorCorrection, 3, 0, 1, 2);
    
    m_dustRemoval = new DCheckBox("除尘", this);
    layout->addWidget(m_dustRemoval, 3, 2);
    
    m_noiseReduction = new DCheckBox("降噪", this);
    m_noiseReduction->setChecked(true);
    layout->addWidget(m_noiseReduction, 4, 0, 1, 2);
    
    return group;
}

QWidget *ScanControlWidget::createControlButtonsGroup()
{
    auto *group = new DFrame(this);
    group->setFrameStyle(DFrame::NoFrame);
    auto *layout = new QVBoxLayout(group);
    
    // 按钮区域
    auto *buttonLayout = new QHBoxLayout();
    
    m_scanButton = new DPushButton("开始扫描", this);
    m_scanButton->setIcon(QIcon::fromTheme("document-scan"));
    
    m_previewButton = new DPushButton("预览", this);
    m_previewButton->setIcon(QIcon::fromTheme("document-preview"));
    
    m_cancelButton = new DPushButton("取消", this);
    m_cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    m_cancelButton->setEnabled(false);
    
    m_resetButton = new DPushButton("重置", this);
    m_resetButton->setIcon(QIcon::fromTheme("edit-undo"));
    
    buttonLayout->addWidget(m_scanButton);
    buttonLayout->addWidget(m_previewButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    // 进度和状态区域
    auto *statusLayout = new QHBoxLayout();
    
    m_progressBar = new DProgressBar(this);
    m_progressBar->hide();
    
    m_statusLabel = new DLabel("准备就绪", this);
    
    statusLayout->addWidget(m_progressBar);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    
    layout->addLayout(statusLayout);
    
    return group;
}

void ScanControlWidget::updateDeviceCapabilities()
{
    if (!m_currentDevice) {
        qDebug() << "ScanControlWidget: 无当前设备，禁用所有控件";
        setEnabled(false);
        return;
    }
    
    qDebug() << "ScanControlWidget: 更新设备能力相关UI";
    
    // 根据设备能力更新可用选项
    // 这里可以根据具体设备的能力来动态调整UI选项
    setEnabled(true);
}

bool ScanControlWidget::validateScanParameters() const
{
    // 验证扫描区域
    if (m_widthSpinBox->value() <= 0 || m_heightSpinBox->value() <= 0) {
        return false;
    }
    
    // 验证分辨率
    if (m_resolutionCombo->currentData().toInt() <= 0) {
        return false;
    }
    
    return true;
}

void ScanControlWidget::emitParametersChanged()
{
    if (validateScanParameters()) {
        emit parametersChanged(getScanParameters());
    }
}

// 槽函数实现
void ScanControlWidget::onResolutionChanged()
{
    qDebug() << "ScanControlWidget: 分辨率已更改";
    emitParametersChanged();
}

void ScanControlWidget::onColorModeChanged()
{
    qDebug() << "ScanControlWidget: 颜色模式已更改";
    emitParametersChanged();
}

void ScanControlWidget::onScanSourceChanged()
{
    qDebug() << "ScanControlWidget: 扫描源已更改";
    emitParametersChanged();
}

void ScanControlWidget::onScanAreaChanged()
{
    qDebug() << "ScanControlWidget: 扫描区域已更改";
    emitParametersChanged();
}

void ScanControlWidget::onImageEnhancementChanged()
{
    qDebug() << "ScanControlWidget: 图像增强设置已更改";
    emitParametersChanged();
}

void ScanControlWidget::onStartScanClicked()
{
    if (!validateScanParameters()) {
        DMessageBox::warning(this, "参数错误", "请检查扫描参数设置");
        return;
    }
    
    qDebug() << "ScanControlWidget: 开始扫描";
    
    m_statusLabel->setText("正在扫描...");
    m_progressBar->show();
    m_progressBar->setValue(0);
    
    m_scanButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    
    emit scanRequested(getScanParameters());
}

void ScanControlWidget::onCancelScanClicked()
{
    qDebug() << "ScanControlWidget: 取消扫描";
    
    m_statusLabel->setText("已取消扫描");
    m_progressBar->hide();
    
    m_scanButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    
    emit cancelScanRequested();
}

void ScanControlWidget::onPreviewClicked()
{
    if (!validateScanParameters()) {
        DMessageBox::warning(this, "参数错误", "请检查扫描参数设置");
        return;
    }
    
    qDebug() << "ScanControlWidget: 请求预览";
    
    m_statusLabel->setText("正在预览...");
    emit previewRequested(getScanParameters());
}

void ScanControlWidget::onResetClicked()
{
    qDebug() << "ScanControlWidget: 重置设置";
    resetToDefaults();
} 