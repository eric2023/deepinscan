// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCANCONTROLWIDGET_H
#define SCANCONTROLWIDGET_H

#include <DWidget>
#include <DPushButton>
#include <DComboBox>
#include <DSlider>
#include <DSpinBox>
#include <DDoubleSpinBox>
#include <DCheckBox>
#include <DGroupBox>
#include <DLabel>
#include <DProgressBar>
#include <DFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include "Scanner/DScannerTypes.h"
#include "Scanner/DScannerDevice.h"

DWIDGET_USE_NAMESPACE
using namespace Dtk::Scanner;

/**
 * @brief 扫描控制界面组件
 * 
 * 提供扫描参数设置和扫描控制功能
 * 包括分辨率、颜色模式、扫描区域、图像增强等设置
 */
class ScanControlWidget : public DWidget
{
    Q_OBJECT

public:
    explicit ScanControlWidget(QWidget *parent = nullptr);
    ~ScanControlWidget();

    /**
     * @brief 设置当前扫描设备
     * @param device 扫描设备指针
     */
    void setCurrentDevice(DScannerDevice *device);

    /**
     * @brief 获取当前扫描配置
     * @return 扫描配置结构体
     */
    ScanParameters getScanParameters() const;

    /**
     * @brief 设置扫描配置
     * @param params 扫描配置结构体
     */
    void setScanParameters(const ScanParameters &params);

    /**
     * @brief 启用/禁用扫描控制
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled);

    /**
     * @brief 重置为默认配置
     */
    void resetToDefaults();

signals:
    /**
     * @brief 请求开始扫描
     * @param params 扫描参数
     */
    void scanRequested(const ScanParameters &params);

    /**
     * @brief 请求取消扫描
     */
    void cancelScanRequested();

    /**
     * @brief 请求预览扫描
     * @param params 扫描参数
     */
    void previewRequested(const ScanParameters &params);

    /**
     * @brief 扫描参数已更改
     * @param params 新的扫描参数
     */
    void parametersChanged(const ScanParameters &params);

private slots:
    /**
     * @brief 分辨率改变槽函数
     */
    void onResolutionChanged();

    /**
     * @brief 颜色模式改变槽函数
     */
    void onColorModeChanged();

    /**
     * @brief 扫描源改变槽函数
     */
    void onScanSourceChanged();

    /**
     * @brief 扫描区域改变槽函数
     */
    void onScanAreaChanged();

    /**
     * @brief 图像增强选项改变槽函数
     */
    void onImageEnhancementChanged();

    /**
     * @brief 开始扫描按钮点击槽函数
     */
    void onStartScanClicked();

    /**
     * @brief 取消扫描按钮点击槽函数
     */
    void onCancelScanClicked();

    /**
     * @brief 预览按钮点击槽函数
     */
    void onPreviewClicked();

    /**
     * @brief 重置按钮点击槽函数
     */
    void onResetClicked();

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
     * @brief 创建基本扫描设置组
     * @return 设置组widget
     */
    QWidget *createBasicSettingsGroup();

    /**
     * @brief 创建高级扫描设置组
     * @return 设置组widget
     */
    QWidget *createAdvancedSettingsGroup();

    /**
     * @brief 创建扫描区域设置组
     * @return 设置组widget
     */
    QWidget *createScanAreaGroup();

    /**
     * @brief 创建图像增强设置组
     * @return 设置组widget
     */
    QWidget *createImageEnhancementGroup();

    /**
     * @brief 创建控制按钮组
     * @return 按钮组widget
     */
    QWidget *createControlButtonsGroup();

    /**
     * @brief 更新设备能力相关的UI
     */
    void updateDeviceCapabilities();

    /**
     * @brief 验证扫描参数
     * @return 是否有效
     */
    bool validateScanParameters() const;

    /**
     * @brief 发射参数更改信号
     */
    void emitParametersChanged();

private:
    // 当前设备
    DScannerDevice *m_currentDevice;

    // 基本设置控件
    DComboBox *m_resolutionCombo;
    DComboBox *m_colorModeCombo;
    DComboBox *m_scanSourceCombo;
    DComboBox *m_formatCombo;

    // 扫描区域控件
    DDoubleSpinBox *m_xSpinBox;
    DDoubleSpinBox *m_ySpinBox;
    DDoubleSpinBox *m_widthSpinBox;
    DDoubleSpinBox *m_heightSpinBox;
    DPushButton *m_fullAreaButton;
    DPushButton *m_customAreaButton;

    // 图像增强控件
    DSlider *m_brightnessSlider;
    DSlider *m_contrastSlider;
    DSlider *m_gammaSlider;
    DCheckBox *m_autoColorCorrection;
    DCheckBox *m_dustRemoval;
    DCheckBox *m_noiseReduction;

    // 高级设置控件
    DSpinBox *m_qualitySpinBox;
    DCheckBox *m_multiSamplingCheck;
    DSpinBox *m_batchCountSpinBox;

    // 控制按钮
    DPushButton *m_scanButton;
    DPushButton *m_cancelButton;
    DPushButton *m_previewButton;
    DPushButton *m_resetButton;

    // 进度显示
    DProgressBar *m_progressBar;
    DLabel *m_statusLabel;

    // 布局
    QVBoxLayout *m_mainLayout;
};

#endif // SCANCONTROLWIDGET_H 