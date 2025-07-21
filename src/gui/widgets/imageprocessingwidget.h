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
#include <QTimer>
#include <QString>

#include "Scanner/DScannerImageProcessor.h"

DWIDGET_USE_NAMESPACE
using namespace Dtk::Scanner;

/**
 * @brief 图像处理UI参数结构体
 * 专门用于GUI组件的参数管理
 */
struct ImageProcessingUIParameters {
    // 色彩调整参数
    int brightness = 0;           ///< 亮度 (-100 到 100)
    int contrast = 0;             ///< 对比度 (-100 到 100)
    int saturation = 0;           ///< 饱和度 (-100 到 100)
    double gamma = 1.0;           ///< 伽马值 (0.5 到 3.0)
    
    // 几何变换参数
    int rotation = 0;             ///< 旋转角度 (-180 到 180)
    double scaleX = 1.0;          ///< X轴缩放比例 (0.1 到 5.0)
    double scaleY = 1.0;          ///< Y轴缩放比例 (0.1 到 5.0)
    bool lockAspectRatio = true;  ///< 锁定宽高比
    
    // 滤镜参数
    QString filterType = "无滤镜"; ///< 滤镜类型
    double filterIntensity = 1.0; ///< 滤镜强度 (0.0 到 1.0)
    
    ImageProcessingUIParameters() = default;
    
    bool isValid() const {
        return brightness >= -100 && brightness <= 100 &&
               contrast >= -100 && contrast <= 100 &&
               saturation >= -100 && saturation <= 100 &&
               gamma >= 0.5 && gamma <= 3.0 &&
               rotation >= -180 && rotation <= 180 &&
               scaleX >= 0.1 && scaleX <= 5.0 &&
               scaleY >= 0.1 && scaleY <= 5.0 &&
               filterIntensity >= 0.0 && filterIntensity <= 1.0;
    }
};

/**
 * @brief 图像处理组件
 * 
 * 提供扫描图像的后处理功能
 * 包括色彩调整、裁剪、旋转、滤镜等处理选项
 */
class DImageProcessingWidget : public DWidget
{
    Q_OBJECT

public:
    explicit DImageProcessingWidget(QWidget *parent = nullptr);
    ~DImageProcessingWidget();

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

    /**
     * @brief 获取当前处理参数
     * @return 处理参数结构体
     */
    ImageProcessingUIParameters getCurrentParameters() const;

    /**
     * @brief 设置处理参数
     * @param params 处理参数结构体
     */
    void setParameters(const ImageProcessingUIParameters &params);

    /**
     * @brief 设置处理状态
     * @param processing 是否正在处理
     */
    void setProcessing(bool processing);

    /**
     * @brief 更新进度
     * @param percentage 进度百分比
     */
    void updateProgress(int percentage);

    /**
     * @brief 设置预览是否启用
     * @param enabled 是否启用预览
     */
    void setPreviewEnabled(bool enabled);

signals:
    /**
     * @brief 图像处理完成
     * @param processedImage 处理后的图像
     */
    void imageProcessed(const QPixmap &processedImage);

    /**
     * @brief 处理参数已更改
     */
    void parametersChanged(const ImageProcessingUIParameters &params);

    /**
     * @brief 处理进度更新
     * @param progress 进度值 (0-100)
     */
    void progressUpdated(int progress);

    /**
     * @brief 请求处理图像
     * @param params 处理参数
     */
    void processingRequested(const ImageProcessingUIParameters &params);

    /**
     * @brief 请求预览
     * @param params 处理参数  
     */
    void previewRequested(const ImageProcessingUIParameters &params);

    /**
     * @brief 取消预览
     */
    void previewCancelled();

    /**
     * @brief 参数已重置
     */
    void parametersReset();

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
     * @brief X轴缩放改变槽函数
     * @param value 缩放值
     */
    void onScaleXChanged(int value);

    /**
     * @brief Y轴缩放改变槽函数  
     * @param value 缩放值
     */
    void onScaleYChanged(int value);

    /**
     * @brief 比例锁定状态改变槽函数
     * @param locked 是否锁定
     */
    void onAspectRatioLockChanged(bool locked);

    /**
     * @brief 滤镜参数改变槽函数
     */
    void onFilterChanged();

    /**
     * @brief 预览切换槽函数
     * @param enabled 是否启用
     */
    void onPreviewToggled(bool enabled);

    /**
     * @brief 应用处理按钮点击槽函数
     */
    void applyProcessing();

    /**
     * @brief 重置到默认值
     */
    void resetToDefaults();

private:
    /**
     * @brief 初始化用户界面
     */
    void setupUI();

    /**
     * @brief 连接信号
     */
    void connectSignals();

    /**
     * @brief 设置色彩调整组
     */
    void setupColorAdjustmentGroup();

    /**
     * @brief 设置几何变换组
     */
    void setupGeometryGroup();

    /**
     * @brief 设置滤镜效果组
     */
    void setupFilterGroup();

    /**
     * @brief 设置操作按钮组
     */
    void setupActionButtons();

private:
    // 图像处理器和图像数据
    DScannerImageProcessor *m_imageProcessor;
    QPixmap m_sourceImage;
    QPixmap m_processedImage;

    // 主要组件
    DGroupBox *m_colorGroup;
    DGroupBox *m_geometryGroup;
    DGroupBox *m_filterGroup;
    DFrame *m_actionFrame;
    QProgressBar *m_progressBar;
    QTimer *m_resetTimer;

    // 色彩调整控件
    DSlider *m_brightnessSlider;
    DSpinBox *m_brightnessSpinBox;
    DSlider *m_contrastSlider;
    DSpinBox *m_contrastSpinBox;
    DSlider *m_saturationSlider;
    DSpinBox *m_saturationSpinBox;
    DSlider *m_gammaSlider;
    DSpinBox *m_gammaSpinBox;

    // 几何变换控件
    DSlider *m_rotationSlider;
    DSpinBox *m_rotationSpinBox;
    DSlider *m_scaleXSlider;
    DSpinBox *m_scaleXSpinBox;
    DSlider *m_scaleYSlider;
    DSpinBox *m_scaleYSpinBox;
    DCheckBox *m_lockAspectRatio;

    // 滤镜效果控件
    DComboBox *m_filterComboBox;
    DSlider *m_filterIntensity;
    DSpinBox *m_filterIntensitySpinBox;

    // 控制按钮
    DPushButton *m_previewButton;
    DPushButton *m_applyButton;
    DPushButton *m_resetButton;

    // 处理状态
    bool m_processing;
};

#endif // IMAGEPROCESSINGWIDGET_H 