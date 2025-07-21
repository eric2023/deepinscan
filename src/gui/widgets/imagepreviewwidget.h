// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEPREVIEWWIDGET_H
#define IMAGEPREVIEWWIDGET_H

#include <DWidget>
#include <DLabel>
#include <DPushButton>
#include <DSlider>
#include <DSpinBox>
#include <DScrollArea>
#include <DFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

DWIDGET_USE_NAMESPACE

/**
 * @brief 图像预览组件
 * 
 * 提供扫描图像的预览、缩放、旋转等功能
 * 支持多种图像格式的显示和基本编辑操作
 */
class ImagePreviewWidget : public DWidget
{
    Q_OBJECT

public:
    explicit ImagePreviewWidget(QWidget *parent = nullptr);
    ~ImagePreviewWidget();

    /**
     * @brief 设置预览图像
     * @param image 要预览的图像
     */
    void setPreviewImage(const QPixmap &image);

    /**
     * @brief 获取当前预览图像
     * @return 当前预览的图像
     */
    QPixmap getPreviewImage() const;

    /**
     * @brief 清除预览图像
     */
    void clearPreview();

    /**
     * @brief 设置缩放比例
     * @param scale 缩放比例 (0.1 - 5.0)
     */
    void setScale(double scale);

    /**
     * @brief 获取当前缩放比例
     * @return 当前缩放比例
     */
    double getScale() const;

    /**
     * @brief 适应窗口大小
     */
    void fitToWindow();

    /**
     * @brief 实际大小显示
     */
    void actualSize();

    /**
     * @brief 旋转图像
     * @param angle 旋转角度（度）
     */
    void rotateImage(int angle);

signals:
    /**
     * @brief 图像已更改
     * @param image 更改后的图像
     */
    void imageChanged(const QPixmap &image);

    /**
     * @brief 缩放比例已更改
     * @param scale 新的缩放比例
     */
    void scaleChanged(double scale);

    /**
     * @brief 请求保存图像
     * @param image 要保存的图像
     */
    void saveRequested(const QPixmap &image);
    
    /**
     * @brief 请求保存图像（别名）
     * @param image 要保存的图像
     */
    void imageSaveRequested(const QPixmap &image);
    
    /**
     * @brief 请求图像处理
     * @param image 要处理的图像
     */
    void imageProcessingRequested(const QPixmap &image);

private slots:
    /**
     * @brief 缩放比例改变槽函数
     */
    void onScaleChanged();

    /**
     * @brief 适应窗口按钮点击槽函数
     */
    void onFitToWindowClicked();

    /**
     * @brief 实际大小按钮点击槽函数
     */
    void onActualSizeClicked();

    /**
     * @brief 放大按钮点击槽函数
     */
    void onZoomInClicked();

    /**
     * @brief 缩小按钮点击槽函数
     */
    void onZoomOutClicked();

    /**
     * @brief 向左旋转按钮点击槽函数
     */
    void onRotateLeftClicked();

    /**
     * @brief 向右旋转按钮点击槽函数
     */
    void onRotateRightClicked();

    /**
     * @brief 保存按钮点击槽函数
     */
    void onSaveClicked();

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
     * @brief 创建工具栏
     * @return 工具栏widget
     */
    QWidget *createToolBar();

    /**
     * @brief 创建预览区域
     * @return 预览区域widget
     */
    QWidget *createPreviewArea();

    /**
     * @brief 创建状态栏
     * @return 状态栏widget
     */
    QWidget *createStatusBar();

    /**
     * @brief 更新预览显示
     */
    void updatePreview();

    /**
     * @brief 更新状态信息
     */
    void updateStatusInfo();

    /**
     * @brief 计算适应窗口的缩放比例
     * @return 缩放比例
     */
    double calculateFitToWindowScale() const;

private:
    // 原始图像和当前显示图像
    QPixmap m_originalImage;
    QPixmap m_currentImage;

    // 图像变换参数
    double m_scale;
    int m_rotation;

    // UI组件
    QVBoxLayout *m_mainLayout;

    // 工具栏组件
    DPushButton *m_fitToWindowButton;
    DPushButton *m_actualSizeButton;
    DPushButton *m_zoomInButton;
    DPushButton *m_zoomOutButton;
    DSlider *m_scaleSlider;
    DSpinBox *m_scaleSpinBox;
    DPushButton *m_rotateLeftButton;
    DPushButton *m_rotateRightButton;
    DPushButton *m_saveButton;
    DPushButton *m_resetButton;

    // 预览区域组件
    QGraphicsView *m_graphicsView;
    QGraphicsScene *m_graphicsScene;
    QGraphicsPixmapItem *m_pixmapItem;

    // 状态栏组件
    DLabel *m_imageSizeLabel;
    DLabel *m_scaleLabel;
    DLabel *m_positionLabel;
};

#endif // IMAGEPREVIEWWIDGET_H 