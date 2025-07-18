// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file minimal_main.cpp
 * @brief 最小化图像处理GUI应用
 * 
 * 专注于图像处理功能，不依赖扫描设备管理模块
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QSplitter>
#include <QScrollArea>
#include <QGridLayout>
#include <QDebug>
#include <QFileInfo>

#include "Scanner/DScannerImageProcessor.h"

using namespace Dtk::Scanner;

class ImageProcessingWindow : public QMainWindow
{
    Q_OBJECT

public:
    ImageProcessingWindow(QWidget *parent = nullptr);
    ~ImageProcessingWindow();

private slots:
    void openImage();
    void saveImage();
    void resetImage();
    void applyBrightness();
    void applyContrast();
    void applyGamma();
    void applyDenoise();
    void applySharpen();
    void applyAutoLevel();
    void applyColorCorrection();
    void applyDeskew();
    void showAbout();

private:
    void setupUI();
    void updateImageDisplay();
    void updateControls(bool enabled);
    
    // UI组件
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QScrollArea *m_imageScrollArea;
    QLabel *m_imageLabel;
    QWidget *m_controlsWidget;
    QStatusBar *m_statusBar;
    QProgressBar *m_progressBar;
    
    // 控制面板
    QGroupBox *m_basicGroup;
    QSlider *m_brightnessSlider;
    QSpinBox *m_brightnessSpinBox;
    QSlider *m_contrastSlider;
    QSpinBox *m_contrastSpinBox;
    QSlider *m_gammaSlider;
    QSpinBox *m_gammaSpinBox;
    
    QGroupBox *m_filterGroup;
    QPushButton *m_denoiseButton;
    QPushButton *m_sharpenButton;
    
    QGroupBox *m_advancedGroup;
    QPushButton *m_autoLevelButton;
    QPushButton *m_colorCorrectionButton;
    QPushButton *m_deskewButton;
    
    QGroupBox *m_actionGroup;
    QPushButton *m_openButton;
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
    
    // 核心功能
    DScannerImageProcessor *m_processor;
    QImage m_originalImage;
    QImage m_currentImage;
};

ImageProcessingWindow::ImageProcessingWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_processor(new DScannerImageProcessor(this))
{
    setWindowTitle("DeepinScan - Image Processor");
    setMinimumSize(800, 600);
    
    setupUI();
    updateControls(false);
    
    // Initialize processor
    qDebug() << "Image processor initialized";
    m_statusBar->showMessage("Ready", 2000);
}

ImageProcessingWindow::~ImageProcessingWindow()
{
}

void ImageProcessingWindow::setupUI()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // 主分割器
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // 图像显示区域
    m_imageScrollArea = new QScrollArea;
    m_imageScrollArea->setWidgetResizable(true);
    m_imageScrollArea->setAlignment(Qt::AlignCenter);
    m_imageScrollArea->setMinimumWidth(400);
    
    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText("Click 'Open Image' to load and process images");
    m_imageLabel->setStyleSheet("color: gray; font-size: 14px;");
    m_imageScrollArea->setWidget(m_imageLabel);
    
    // 控制面板
    m_controlsWidget = new QWidget;
    m_controlsWidget->setFixedWidth(300);
    QVBoxLayout *controlsLayout = new QVBoxLayout(m_controlsWidget);
    
    // 操作按钮组
    m_actionGroup = new QGroupBox("File Operations");
    QGridLayout *actionLayout = new QGridLayout(m_actionGroup);
    
    m_openButton = new QPushButton("Open Image");
    m_saveButton = new QPushButton("Save Image");
    m_resetButton = new QPushButton("Reset Image");
    
    actionLayout->addWidget(m_openButton, 0, 0);
    actionLayout->addWidget(m_saveButton, 0, 1);
    actionLayout->addWidget(m_resetButton, 1, 0, 1, 2);
    
    // Basic Adjustments组
    m_basicGroup = new QGroupBox("Basic Adjustments");
    QGridLayout *basicLayout = new QGridLayout(m_basicGroup);
    
    // Brightness
    basicLayout->addWidget(new QLabel("Brightness:"), 0, 0);
    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setRange(-100, 100);
    m_brightnessSlider->setValue(0);
    m_brightnessSpinBox = new QSpinBox;
    m_brightnessSpinBox->setRange(-100, 100);
    m_brightnessSpinBox->setValue(0);
    basicLayout->addWidget(m_brightnessSlider, 0, 1);
    basicLayout->addWidget(m_brightnessSpinBox, 0, 2);
    
    // Contrast
    basicLayout->addWidget(new QLabel("Contrast:"), 1, 0);
    m_contrastSlider = new QSlider(Qt::Horizontal);
    m_contrastSlider->setRange(-100, 100);
    m_contrastSlider->setValue(0);
    m_contrastSpinBox = new QSpinBox;
    m_contrastSpinBox->setRange(-100, 100);
    m_contrastSpinBox->setValue(0);
    basicLayout->addWidget(m_contrastSlider, 1, 1);
    basicLayout->addWidget(m_contrastSpinBox, 1, 2);
    
    // Gamma
    basicLayout->addWidget(new QLabel("Gamma:"), 2, 0);
    m_gammaSlider = new QSlider(Qt::Horizontal);
    m_gammaSlider->setRange(10, 300);
    m_gammaSlider->setValue(100);
    m_gammaSpinBox = new QSpinBox;
    m_gammaSpinBox->setRange(10, 300);
    m_gammaSpinBox->setValue(100);
    basicLayout->addWidget(m_gammaSlider, 2, 1);
    basicLayout->addWidget(m_gammaSpinBox, 2, 2);
    
    // 滤波组
    m_filterGroup = new QGroupBox("Filters");
    QVBoxLayout *filterLayout = new QVBoxLayout(m_filterGroup);
    
    m_denoiseButton = new QPushButton("Denoise");
    m_sharpenButton = new QPushButton("Sharpen");
    
    filterLayout->addWidget(m_denoiseButton);
    filterLayout->addWidget(m_sharpenButton);
    
    // Advanced组
    m_advancedGroup = new QGroupBox("Advanced");
    QVBoxLayout *advancedLayout = new QVBoxLayout(m_advancedGroup);
    
    m_autoLevelButton = new QPushButton("Auto Level");
    m_colorCorrectionButton = new QPushButton("Color Correction");
    m_deskewButton = new QPushButton("Deskew");
    
    advancedLayout->addWidget(m_autoLevelButton);
    advancedLayout->addWidget(m_colorCorrectionButton);
    advancedLayout->addWidget(m_deskewButton);
    
    // 添加到控制面板
    controlsLayout->addWidget(m_actionGroup);
    controlsLayout->addWidget(m_basicGroup);
    controlsLayout->addWidget(m_filterGroup);
    controlsLayout->addWidget(m_advancedGroup);
    controlsLayout->addStretch();
    
    // 添加到主分割器
    m_mainSplitter->addWidget(m_imageScrollArea);
    m_mainSplitter->addWidget(m_controlsWidget);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 0);
    
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);
    
    // 状态栏
    m_statusBar = statusBar();
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    m_statusBar->addPermanentWidget(m_progressBar);
    
    // 连接信号
    connect(m_openButton, &QPushButton::clicked, this, &ImageProcessingWindow::openImage);
    connect(m_saveButton, &QPushButton::clicked, this, &ImageProcessingWindow::saveImage);
    connect(m_resetButton, &QPushButton::clicked, this, &ImageProcessingWindow::resetImage);
    
    connect(m_brightnessSlider, &QSlider::valueChanged, m_brightnessSpinBox, &QSpinBox::setValue);
    connect(m_brightnessSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_brightnessSlider, &QSlider::setValue);
    connect(m_brightnessSlider, &QSlider::sliderReleased, this, &ImageProcessingWindow::applyBrightness);
    
    connect(m_contrastSlider, &QSlider::valueChanged, m_contrastSpinBox, &QSpinBox::setValue);
    connect(m_contrastSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_contrastSlider, &QSlider::setValue);
    connect(m_contrastSlider, &QSlider::sliderReleased, this, &ImageProcessingWindow::applyContrast);
    
    connect(m_gammaSlider, &QSlider::valueChanged, m_gammaSpinBox, &QSpinBox::setValue);
    connect(m_gammaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_gammaSlider, &QSlider::setValue);
    connect(m_gammaSlider, &QSlider::sliderReleased, this, &ImageProcessingWindow::applyGamma);
    
    connect(m_denoiseButton, &QPushButton::clicked, this, &ImageProcessingWindow::applyDenoise);
    connect(m_sharpenButton, &QPushButton::clicked, this, &ImageProcessingWindow::applySharpen);
    connect(m_autoLevelButton, &QPushButton::clicked, this, &ImageProcessingWindow::applyAutoLevel);
    connect(m_colorCorrectionButton, &QPushButton::clicked, this, &ImageProcessingWindow::applyColorCorrection);
    connect(m_deskewButton, &QPushButton::clicked, this, &ImageProcessingWindow::applyDeskew);
}

void ImageProcessingWindow::openImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Open Image", "",
        "Image files (*.png *.jpg *.jpeg *.bmp *.tiff)");
    
    if (!fileName.isEmpty()) {
        QImage image(fileName);
        if (!image.isNull()) {
            m_originalImage = image;
            m_currentImage = image;
            updateImageDisplay();
            updateControls(true);
            m_statusBar->showMessage("Image loaded: " + QFileInfo(fileName).fileName(), 3000);
        } else {
            QMessageBox::warning(this, "Error", "Unable to load image file");
        }
    }
}

void ImageProcessingWindow::saveImage()
{
    if (m_currentImage.isNull()) {
        QMessageBox::information(this, "Info", "No image to save");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Image", "",
        "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");
    
    if (!fileName.isEmpty()) {
        if (m_currentImage.save(fileName)) {
            m_statusBar->showMessage("Image saved successfully", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Unable to save image file");
        }
    }
}

void ImageProcessingWindow::resetImage()
{
    if (!m_originalImage.isNull()) {
        m_currentImage = m_originalImage;
        updateImageDisplay();
        
        // 重置控件
        m_brightnessSlider->setValue(0);
        m_contrastSlider->setValue(0);
        m_gammaSlider->setValue(100);
        
        m_statusBar->showMessage("Image reset", 2000);
    }
}

void ImageProcessingWindow::applyBrightness()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    m_currentImage = m_processor->adjustBrightness(m_originalImage, m_brightnessSlider->value());
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Brightness adjusted", 2000);
}

void ImageProcessingWindow::applyContrast()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    m_currentImage = m_processor->adjustContrast(m_originalImage, m_contrastSlider->value());
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Contrast adjusted", 2000);
}

void ImageProcessingWindow::applyGamma()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    double gamma = m_gammaSlider->value() / 100.0;
    m_currentImage = m_processor->adjustGamma(m_originalImage, gamma);
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Gamma corrected", 2000);
}

void ImageProcessingWindow::applyDenoise()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    m_currentImage = m_processor->denoise(m_currentImage, 50);
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Denoise applied", 2000);
}

void ImageProcessingWindow::applySharpen()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    m_currentImage = m_processor->sharpen(m_currentImage, 50);
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Sharpen applied", 2000);
}

void ImageProcessingWindow::applyAutoLevel()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    m_currentImage = m_processor->autoLevel(m_currentImage);
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Auto level applied", 2000);
}

void ImageProcessingWindow::applyColorCorrection()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    m_currentImage = m_processor->colorCorrection(m_currentImage);
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Color correction applied", 2000);
}

void ImageProcessingWindow::applyDeskew()
{
    if (m_originalImage.isNull()) return;
    
    m_progressBar->setVisible(true);
    m_currentImage = m_processor->deskew(m_currentImage);
    updateImageDisplay();
    m_progressBar->setVisible(false);
    m_statusBar->showMessage("Deskew applied", 2000);
}

void ImageProcessingWindow::showAbout()
{
    QMessageBox::about(this, "About", 
        "DeepinScan Image Processor\n\n"
        "A modern scanner image processing application\n"
        "With SIMD-optimized high-performance image processing algorithms");
}

void ImageProcessingWindow::updateImageDisplay()
{
    if (!m_currentImage.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(m_currentImage);
        
        // 缩放图像以适应显示
        if (pixmap.width() > 800 || pixmap.height() > 600) {
            pixmap = pixmap.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        
        m_imageLabel->setPixmap(pixmap);
        m_imageLabel->resize(pixmap.size());
    }
}

void ImageProcessingWindow::updateControls(bool enabled)
{
    m_basicGroup->setEnabled(enabled);
    m_filterGroup->setEnabled(enabled);
    m_advancedGroup->setEnabled(enabled);
    m_saveButton->setEnabled(enabled);
    m_resetButton->setEnabled(enabled);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("DeepinScan");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Deepin Technology Co., Ltd.");
    
    ImageProcessingWindow window;
    window.show();
    
    return app.exec();
}

#include "minimal_main.moc" 