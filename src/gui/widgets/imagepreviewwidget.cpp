// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagepreviewwidget.h"
#include <DApplication>
#include <DMessageBox>
#include <DFrame>
#include <DFontSizeManager>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTransform>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>

DWIDGET_USE_NAMESPACE

ImagePreviewWidget::ImagePreviewWidget(QWidget *parent)
    : DWidget(parent)
    , m_scale(1.0)
    , m_rotation(0)
    , m_mainLayout(nullptr)
    , m_fitToWindowButton(nullptr)
    , m_actualSizeButton(nullptr)
    , m_zoomInButton(nullptr)
    , m_zoomOutButton(nullptr)
    , m_scaleSlider(nullptr)
    , m_scaleSpinBox(nullptr)
    , m_rotateLeftButton(nullptr)
    , m_rotateRightButton(nullptr)
    , m_saveButton(nullptr)
    , m_resetButton(nullptr)
    , m_graphicsView(nullptr)
    , m_graphicsScene(nullptr)
    , m_pixmapItem(nullptr)
    , m_imageSizeLabel(nullptr)
    , m_scaleLabel(nullptr)
    , m_positionLabel(nullptr)
{
    qDebug() << "ImagePreviewWidget: 初始化图像预览界面组件";
    initUI();
    initConnections();
}

ImagePreviewWidget::~ImagePreviewWidget()
{
    qDebug() << "ImagePreviewWidget: 销毁图像预览界面组件";
    
    if (m_pixmapItem) {
        delete m_pixmapItem;
        m_pixmapItem = nullptr;
    }
    
    if (m_graphicsScene) {
        delete m_graphicsScene;
        m_graphicsScene = nullptr;
    }
}

void ImagePreviewWidget::setPreviewImage(const QPixmap &image)
{
    qDebug() << "ImagePreviewWidget: 设置预览图像，尺寸:" << image.size();
    
    m_originalImage = image;
    m_currentImage = image;
    m_rotation = 0;
    
    updatePreview();
    updateStatusInfo();
    
    // 启用相关按钮
    m_saveButton->setEnabled(!image.isNull());
    m_resetButton->setEnabled(!image.isNull());
    m_fitToWindowButton->setEnabled(!image.isNull());
    m_actualSizeButton->setEnabled(!image.isNull());
    m_zoomInButton->setEnabled(!image.isNull());
    m_zoomOutButton->setEnabled(!image.isNull());
    m_rotateLeftButton->setEnabled(!image.isNull());
    m_rotateRightButton->setEnabled(!image.isNull());
    
    emit imageChanged(m_currentImage);
}

QPixmap ImagePreviewWidget::getPreviewImage() const
{
    return m_currentImage;
}

void ImagePreviewWidget::clearPreview()
{
    qDebug() << "ImagePreviewWidget: 清除预览图像";
    
    m_originalImage = QPixmap();
    m_currentImage = QPixmap();
    m_scale = 1.0;
    m_rotation = 0;
    
    if (m_pixmapItem) {
        m_pixmapItem->setPixmap(QPixmap());
    }
    
    updateStatusInfo();
    
    // 禁用相关按钮
    m_saveButton->setEnabled(false);
    m_resetButton->setEnabled(false);
    m_fitToWindowButton->setEnabled(false);
    m_actualSizeButton->setEnabled(false);
    m_zoomInButton->setEnabled(false);
    m_zoomOutButton->setEnabled(false);
    m_rotateLeftButton->setEnabled(false);
    m_rotateRightButton->setEnabled(false);
}

void ImagePreviewWidget::setScale(double scale)
{
    scale = qBound(0.1, scale, 5.0);
    if (qAbs(m_scale - scale) < 0.01) {
        return;
    }
    
    qDebug() << "ImagePreviewWidget: 设置缩放比例:" << scale;
    
    m_scale = scale;
    
    // 更新UI控件
    m_scaleSlider->blockSignals(true);
    m_scaleSpinBox->blockSignals(true);
    
    m_scaleSlider->setValue(static_cast<int>(scale * 100));
    m_scaleSpinBox->setValue(static_cast<int>(scale * 100));
    
    m_scaleSlider->blockSignals(false);
    m_scaleSpinBox->blockSignals(false);
    
    updatePreview();
    updateStatusInfo();
    
    emit scaleChanged(m_scale);
}

double ImagePreviewWidget::getScale() const
{
    return m_scale;
}

void ImagePreviewWidget::fitToWindow()
{
    if (m_originalImage.isNull() || !m_graphicsView) {
        return;
    }
    
    qDebug() << "ImagePreviewWidget: 适应窗口大小";
    
    double scale = calculateFitToWindowScale();
    setScale(scale);
}

void ImagePreviewWidget::actualSize()
{
    qDebug() << "ImagePreviewWidget: 实际大小显示";
    setScale(1.0);
}

void ImagePreviewWidget::rotateImage(int angle)
{
    if (m_originalImage.isNull()) {
        return;
    }
    
    qDebug() << "ImagePreviewWidget: 旋转图像:" << angle << "度";
    
    m_rotation = (m_rotation + angle) % 360;
    if (m_rotation < 0) {
        m_rotation += 360;
    }
    
    // 应用旋转到当前图像
    QTransform transform;
    transform.rotate(angle);
    m_currentImage = m_currentImage.transformed(transform, Qt::SmoothTransformation);
    
    updatePreview();
    updateStatusInfo();
    
    emit imageChanged(m_currentImage);
}

void ImagePreviewWidget::initUI()
{
    qDebug() << "ImagePreviewWidget: 初始化用户界面";
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    // 创建各个区域
    m_mainLayout->addWidget(createToolBar());
    m_mainLayout->addWidget(createPreviewArea(), 1); // 预览区域占据主要空间
    m_mainLayout->addWidget(createStatusBar());
    
    setLayout(m_mainLayout);
}

void ImagePreviewWidget::initConnections()
{
    qDebug() << "ImagePreviewWidget: 初始化信号连接";
    
    // 工具栏按钮信号连接
    connect(m_fitToWindowButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onFitToWindowClicked);
    connect(m_actualSizeButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onActualSizeClicked);
    connect(m_zoomInButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onZoomInClicked);
    connect(m_zoomOutButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onZoomOutClicked);
    connect(m_rotateLeftButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onRotateLeftClicked);
    connect(m_rotateRightButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onRotateRightClicked);
    connect(m_saveButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onSaveClicked);
    connect(m_resetButton, &DPushButton::clicked,
            this, &ImagePreviewWidget::onResetClicked);
    
    // 缩放控件信号连接
    connect(m_scaleSlider, &DSlider::valueChanged,
            this, &ImagePreviewWidget::onScaleChanged);
    connect(m_scaleSpinBox, QOverload<int>::of(&DSpinBox::valueChanged),
            this, &ImagePreviewWidget::onScaleChanged);
}

QWidget *ImagePreviewWidget::createToolBar()
{
    auto *toolBar = new DFrame(this);
    toolBar->setFrameStyle(DFrame::NoFrame);
    auto *layout = new QHBoxLayout(toolBar);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // 视图控制按钮
    m_fitToWindowButton = new DPushButton("适应窗口", this);
    m_fitToWindowButton->setIcon(QIcon::fromTheme("zoom-fit-best"));
    m_fitToWindowButton->setEnabled(false);
    
    m_actualSizeButton = new DPushButton("实际大小", this);
    m_actualSizeButton->setIcon(QIcon::fromTheme("zoom-original"));
    m_actualSizeButton->setEnabled(false);
    
    m_zoomInButton = new DPushButton("放大", this);
    m_zoomInButton->setIcon(QIcon::fromTheme("zoom-in"));
    m_zoomInButton->setEnabled(false);
    
    m_zoomOutButton = new DPushButton("缩小", this);
    m_zoomOutButton->setIcon(QIcon::fromTheme("zoom-out"));
    m_zoomOutButton->setEnabled(false);
    
    // 缩放滑块和数值框
    m_scaleSlider = new DSlider(Qt::Horizontal, this);
    m_scaleSlider->setMinimum(10);  // 10%
    m_scaleSlider->setMaximum(500); // 500%
    m_scaleSlider->setValue(100);
    m_scaleSlider->setFixedWidth(150);
    
    m_scaleSpinBox = new DSpinBox(this);
    m_scaleSpinBox->setRange(10, 500);
    m_scaleSpinBox->setValue(100);
    m_scaleSpinBox->setSuffix("%");
    m_scaleSpinBox->setFixedWidth(80);
    
    // 旋转按钮
    m_rotateLeftButton = new DPushButton("左转", this);
    m_rotateLeftButton->setIcon(QIcon::fromTheme("object-rotate-left"));
    m_rotateLeftButton->setEnabled(false);
    
    m_rotateRightButton = new DPushButton("右转", this);
    m_rotateRightButton->setIcon(QIcon::fromTheme("object-rotate-right"));
    m_rotateRightButton->setEnabled(false);
    
    // 操作按钮
    m_saveButton = new DPushButton("保存", this);
    m_saveButton->setIcon(QIcon::fromTheme("document-save"));
    m_saveButton->setEnabled(false);
    
    m_resetButton = new DPushButton("重置", this);
    m_resetButton->setIcon(QIcon::fromTheme("edit-undo"));
    m_resetButton->setEnabled(false);
    
    // 布局按钮
    layout->addWidget(m_fitToWindowButton);
    layout->addWidget(m_actualSizeButton);
    layout->addWidget(m_zoomInButton);
    layout->addWidget(m_zoomOutButton);
    layout->addSpacing(10);
    layout->addWidget(new DLabel("缩放:", this));
    layout->addWidget(m_scaleSlider);
    layout->addWidget(m_scaleSpinBox);
    layout->addSpacing(10);
    layout->addWidget(m_rotateLeftButton);
    layout->addWidget(m_rotateRightButton);
    layout->addStretch();
    layout->addWidget(m_saveButton);
    layout->addWidget(m_resetButton);
    
    return toolBar;
}

QWidget *ImagePreviewWidget::createPreviewArea()
{
    // 创建图形视图和场景
    m_graphicsScene = new QGraphicsScene(this);
    m_graphicsView = new QGraphicsView(m_graphicsScene, this);
    
    // 设置视图属性
    m_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_graphicsView->setRenderHint(QPainter::Antialiasing);
    m_graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_graphicsView->setBackgroundBrush(QBrush(Qt::lightGray, Qt::CrossPattern));
    
    // 创建图像项
    m_pixmapItem = new QGraphicsPixmapItem();
    m_graphicsScene->addItem(m_pixmapItem);
    
    return m_graphicsView;
}

QWidget *ImagePreviewWidget::createStatusBar()
{
    auto *statusBar = new DFrame(this);
    statusBar->setFrameStyle(DFrame::NoFrame);
    auto *layout = new QHBoxLayout(statusBar);
    layout->setContentsMargins(5, 2, 5, 2);
    
    // 状态标签
    m_imageSizeLabel = new DLabel("图像尺寸: 无", this);
    m_scaleLabel = new DLabel("缩放比例: 100%", this);
    m_positionLabel = new DLabel("坐标: (0, 0)", this);
    
    layout->addWidget(m_imageSizeLabel);
    layout->addSpacing(20);
    layout->addWidget(m_scaleLabel);
    layout->addSpacing(20);
    layout->addWidget(m_positionLabel);
    layout->addStretch();
    
    return statusBar;
}

void ImagePreviewWidget::updatePreview()
{
    if (!m_pixmapItem || m_currentImage.isNull()) {
        return;
    }
    
    // 设置图像到场景
    m_pixmapItem->setPixmap(m_currentImage);
    
    // 设置缩放
    m_pixmapItem->setScale(m_scale);
    
    // 调整场景大小
    QRectF imageRect = m_pixmapItem->boundingRect();
    imageRect = QRectF(imageRect.topLeft() * m_scale, imageRect.size() * m_scale);
    m_graphicsScene->setSceneRect(imageRect);
    
    // 居中显示
    m_graphicsView->centerOn(m_pixmapItem);
}

void ImagePreviewWidget::updateStatusInfo()
{
    if (m_originalImage.isNull()) {
        m_imageSizeLabel->setText("图像尺寸: 无");
        m_scaleLabel->setText("缩放比例: 100%");
        m_positionLabel->setText("坐标: (0, 0)");
    } else {
        QSize size = m_originalImage.size();
        m_imageSizeLabel->setText(QString("图像尺寸: %1 × %2").arg(size.width()).arg(size.height()));
        m_scaleLabel->setText(QString("缩放比例: %1%").arg(static_cast<int>(m_scale * 100)));
        
        // 获取当前视图中心位置
        if (m_graphicsView) {
            QPointF center = m_graphicsView->mapToScene(m_graphicsView->viewport()->rect().center());
            m_positionLabel->setText(QString("坐标: (%1, %2)")
                                   .arg(static_cast<int>(center.x()))
                                   .arg(static_cast<int>(center.y())));
        }
    }
}

double ImagePreviewWidget::calculateFitToWindowScale() const
{
    if (m_originalImage.isNull() || !m_graphicsView) {
        return 1.0;
    }
    
    QSize imageSize = m_originalImage.size();
    QSize viewSize = m_graphicsView->viewport()->size();
    
    // 计算水平和垂直方向的缩放比例
    double scaleX = static_cast<double>(viewSize.width() - 20) / imageSize.width();
    double scaleY = static_cast<double>(viewSize.height() - 20) / imageSize.height();
    
    // 取较小的缩放比例以确保图像完全显示
    return qMin(scaleX, scaleY);
}

// 槽函数实现
void ImagePreviewWidget::onScaleChanged()
{
    int value = 100;
    
    // 获取发送者
    QObject *sender = this->sender();
    if (sender == m_scaleSlider) {
        value = m_scaleSlider->value();
        m_scaleSpinBox->blockSignals(true);
        m_scaleSpinBox->setValue(value);
        m_scaleSpinBox->blockSignals(false);
    } else if (sender == m_scaleSpinBox) {
        value = m_scaleSpinBox->value();
        m_scaleSlider->blockSignals(true);
        m_scaleSlider->setValue(value);
        m_scaleSlider->blockSignals(false);
    }
    
    setScale(value / 100.0);
}

void ImagePreviewWidget::onFitToWindowClicked()
{
    qDebug() << "ImagePreviewWidget: 适应窗口按钮点击";
    fitToWindow();
}

void ImagePreviewWidget::onActualSizeClicked()
{
    qDebug() << "ImagePreviewWidget: 实际大小按钮点击";
    actualSize();
}

void ImagePreviewWidget::onZoomInClicked()
{
    qDebug() << "ImagePreviewWidget: 放大按钮点击";
    double newScale = m_scale * 1.25;
    setScale(newScale);
}

void ImagePreviewWidget::onZoomOutClicked()
{
    qDebug() << "ImagePreviewWidget: 缩小按钮点击";
    double newScale = m_scale / 1.25;
    setScale(newScale);
}

void ImagePreviewWidget::onRotateLeftClicked()
{
    qDebug() << "ImagePreviewWidget: 左转按钮点击";
    rotateImage(-90);
    emit imageProcessingRequested(m_currentImage);
}

void ImagePreviewWidget::onRotateRightClicked()
{
    qDebug() << "ImagePreviewWidget: 右转按钮点击";
    rotateImage(90);
    emit imageProcessingRequested(m_currentImage);
}

void ImagePreviewWidget::onSaveClicked()
{
    if (m_currentImage.isNull()) {
        return;
    }
    
    qDebug() << "ImagePreviewWidget: 保存按钮点击";
    
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "保存图像",
        QDir(defaultPath).filePath("扫描图像.png"),
        "PNG文件 (*.png);;JPEG文件 (*.jpg *.jpeg);;TIFF文件 (*.tiff *.tif);;所有文件 (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        if (m_currentImage.save(fileName)) {
            qDebug() << "ImagePreviewWidget: 图像保存成功:" << fileName;
            DMessageBox::information(this, "保存成功", QString("图像已保存到:\n%1").arg(fileName));
            emit saveRequested(m_currentImage);
            emit imageSaveRequested(m_currentImage);
        } else {
            qWarning() << "ImagePreviewWidget: 图像保存失败:" << fileName;
            DMessageBox::warning(this, "保存失败", "无法保存图像文件");
        }
    }
}

void ImagePreviewWidget::onResetClicked()
{
    qDebug() << "ImagePreviewWidget: 重置按钮点击";
    
    if (!m_originalImage.isNull()) {
        m_currentImage = m_originalImage;
        m_rotation = 0;
        setScale(1.0);
        updatePreview();
        emit imageChanged(m_currentImage);
    }
} 