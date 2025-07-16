// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEPROCESSINGWIDGET_H
#define IMAGEPROCESSINGWIDGET_H

#include <DWidget>
#include <DPushButton>
#include <DSlider>
#include <DSpinBox>
#include <DCheckBox>
#include <DComboBox>
#include <DGroupBox>
#include <DLabel>
#include <DProgressBar>
#include <DFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>

#include "Scanner/DScannerImageProcessor.h"

DWIDGET_USE_NAMESPACE
using namespace Dtk::Scanner;

/**
 * @brief 图像处理组件
 * 
 * 提供扫描图像的后处理功能
 * 包括色彩调整、裁剪、旋转、滤镜等处理选项
 */
class ImageProcessingWidget : public DWidget
{
    Q_OBJECT

public:
    explicit ImageProcessingWidget(QWidget *parent = nullptr);
    ~ImageProcessingWidget();

    /**
     * @brief 设置图像处理器
     * @param processor 图像处理器实例
     */
    void setImageProcessor(DScannerImageProcessor *processor);

    /**
     * @brief 设置源图像
     * @param image 要处理的源图像
     */
    void setSourceImage(const QPixmap &image);

    /**
     * @brief 获取处理后的图像
     * @return 处理后的图像
     */
    QPixmap getProcessedImage() const;

    /**
     * @brief 重置所有处理参数
     */
    void resetParameters();

    /**
     * @brief 应用所有处理效果
     */
    void applyAllEffects();

signals:
    /**
     * @brief 图像处理完成
     * @param processedImage 处理后的图像
     */
    void imageProcessed(const QPixmap &processedImage);

    /**
     * @brief 处理参数已更改
     */
    void parametersChanged();

    /**
     * @brief 处理进度更新
     * @param progress 进度值 (0-100)
     */
    void progressUpdated(int progress);

private slots:
    /**
     * @brief 色彩调整参数改变槽函数
     */
    void onColorAdjustmentChanged();

    /**
     * @brief 几何变换参数改变槽函数
     */
    void onGeometryChanged();

    /**
     * @brief 滤镜参数改变槽函数
     */
    void onFilterChanged();

    /**
     * @brief 应用处理按钮点击槽函数
     */
    void onApplyClicked();

    /**
     * @brief 重置按钮点击槽函数
     */
    void onResetClicked();

    /**
     * @brief 预览按钮点击槽函数
     */
    void onPreviewClicked();

    /**
     * @brief 保存预设按钮点击槽函数
     */
    void onSavePresetClicked();

    /**
     * @brief 加载预设按钮点击槽函数
     */
    void onLoadPresetClicked();

    /**
     * @brief 处理完成槽函数
     */
    void onProcessingFinished(const QPixmap &result);

    /**
     * @brief 处理进度更新槽函数
     */
    void onProcessingProgress(int progress);

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
     * @brief 创建色彩调整组
     * @return 色彩调整组widget
     */
    QWidget *createColorAdjustmentGroup();

    /**
     * @brief 创建几何变换组
     * @return 几何变换组widget
     */
    QWidget *createGeometryGroup();

    /**
     * @brief 创建滤镜效果组
     * @return 滤镜效果组widget
     */
    QWidget *createFilterGroup();

    /**
     * @brief 创建预设管理组
     * @return 预设管理组widget
     */
    QWidget *createPresetGroup();

    /**
     * @brief 创建控制按钮组
     * @return 控制按钮组widget
     */
    QWidget *createControlGroup();

    /**
     * @brief 获取当前处理参数
     * @return 处理参数结构体
     */
    ImageProcessingParameters getProcessingParameters() const;

    /**
     * @brief 设置处理参数
     * @param params 处理参数结构体
     */
    void setProcessingParameters(const ImageProcessingParameters &params);

    /**
     * @brief 启动异步处理
     */
    void startAsyncProcessing();

    /**
     * @brief 更新预览
     */
    void updatePreview();

private:
    // 图像处理器和图像数据
    DScannerImageProcessor *m_imageProcessor;
    QPixmap m_sourceImage;
    QPixmap m_processedImage;

    // 主布局
    QVBoxLayout *m_mainLayout;

    // 色彩调整控件
    DSlider *m_brightnessSlider;
    DSlider *m_contrastSlider;
    DSlider *m_saturationSlider;
    DSlider *m_hueSlider;
    DSlider *m_gammaSlider;
    DCheckBox *m_autoLevelsCheck;
    DCheckBox *m_autoColorCheck;

    // 几何变换控件
    DSlider *m_rotationSlider;
    DSpinBox *m_cropXSpinBox;
    DSpinBox *m_cropYSpinBox;
    DSpinBox *m_cropWidthSpinBox;
    DSpinBox *m_cropHeightSpinBox;
    DCheckBox *m_flipHorizontalCheck;
    DCheckBox *m_flipVerticalCheck;

    // 滤镜效果控件
    DCheckBox *m_sharpenCheck;
    DSlider *m_sharpenSlider;
    DCheckBox *m_blurCheck;
    DSlider *m_blurSlider;
    DCheckBox *m_noiseReductionCheck;
    DSlider *m_noiseReductionSlider;
    DCheckBox *m_edgeEnhanceCheck;
    DComboBox *m_filterPresetCombo;

    // 预设管理控件
    DComboBox *m_presetCombo;
    DPushButton *m_savePresetButton;
    DPushButton *m_loadPresetButton;
    DPushButton *m_deletePresetButton;

    // 控制按钮
    DPushButton *m_previewButton;
    DPushButton *m_applyButton;
    DPushButton *m_resetButton;

    // 进度和状态
    DProgressBar *m_progressBar;
    DLabel *m_statusLabel;

    // 处理状态
    bool m_isProcessing;
    bool m_previewMode;
};

#endif // IMAGEPROCESSINGWIDGET_H 