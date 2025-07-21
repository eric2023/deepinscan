// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ADVANCEDEXPORTDIALOG_H
#define ADVANCEDEXPORTDIALOG_H

#include "Scanner/DScannerGlobal.h"
#include <DDialog>
#include <DComboBox>
#include <DSpinBox>
#include <DDoubleSpinBox>
#include <DSlider>
#include <DCheckBox>
#include <DLineEdit>
#include <DPushButton>
#include <DLabel>
#include <DGroupBox>
#include <DProgressBar>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTimer>

DWIDGET_USE_NAMESPACE

DSCANNER_BEGIN_NAMESPACE

/**
 * @brief 高级导出对话框 - 现代化导出功能设计
 * 
 * 提供完整的图像导出功能，包括：
 * - 多种文件格式支持 (JPEG, PNG, TIFF, PDF, BMP)
 * - 图像质量和压缩设置
 * - 分辨率和尺寸调整
 * - 颜色空间转换
 * - 批量导出功能
 * - 文件命名模板
 * - 导出进度监控
 */
class DSCANNER_EXPORT AdvancedExportDialog : public DDialog
{
    Q_OBJECT
    
public:
    /**
     * @brief 支持的导出格式
     */
    enum class ExportFormat {
        JPEG,       ///< JPEG格式
        PNG,        ///< PNG格式
        TIFF,       ///< TIFF格式
        PDF,        ///< PDF格式
        BMP,        ///< BMP格式
        WebP        ///< WebP格式
    };
    
    /**
     * @brief 颜色空间类型
     */
    enum class ColorSpace {
        sRGB,       ///< 标准RGB
        AdobeRGB,   ///< Adobe RGB
        Grayscale,  ///< 灰度
        CMYK        ///< CMYK
    };
    
    /**
     * @brief 导出配置结构
     */
    struct ExportSettings {
        ExportFormat format = ExportFormat::JPEG;
        QString outputDirectory;
        QString filenameTemplate = "%Y%M%D_%H%m%S";
        
        // 图像质量设置
        int jpegQuality = 90;           // JPEG质量 (1-100)
        int pngCompression = 6;         // PNG压缩级别 (0-9)
        bool losslessCompression = false; // 无损压缩
        
        // 尺寸和分辨率
        int outputWidth = 0;            // 输出宽度 (0=保持原始)
        int outputHeight = 0;           // 输出高度 (0=保持原始)
        int dpi = 300;                  // 分辨率
        bool maintainAspectRatio = true; // 保持宽高比
        
        // 颜色设置
        ColorSpace colorSpace = ColorSpace::sRGB;
        bool embedColorProfile = true;  // 嵌入颜色配置文件
        
        // 高级选项
        bool applyImageEnhancement = false; // 应用图像增强
        bool removeBackground = false;   // 移除背景
        bool autoRotate = false;        // 自动旋转
        bool addWatermark = false;      // 添加水印
        QString watermarkText;          // 水印文本
        
        // 批量导出
        bool batchExport = false;       // 批量导出模式
        bool createSubfolders = false;  // 创建子文件夹
        QString subfolderPattern = "%Y-%M-%D"; // 子文件夹模式
    };

public:
    explicit AdvancedExportDialog(QWidget *parent = nullptr);
    ~AdvancedExportDialog();
    
    /**
     * @brief 设置要导出的图像
     * @param images 图像列表
     */
    void setImages(const QList<QImage> &images);
    
    /**
     * @brief 设置单个图像导出
     * @param image 图像
     */
    void setImage(const QImage &image);
    
    /**
     * @brief 获取导出设置
     * @return 导出配置
     */
    ExportSettings getExportSettings() const;
    
    /**
     * @brief 设置导出设置
     * @param settings 导出配置
     */
    void setExportSettings(const ExportSettings &settings);
    
    /**
     * @brief 显示对话框并执行导出
     * @return 用户是否确认导出
     */
    bool showAndExport();

public slots:
    /**
     * @brief 开始导出
     */
    void startExport();
    
    /**
     * @brief 取消导出
     */
    void cancelExport();
    
    /**
     * @brief 预览导出效果
     */
    void previewExport();

signals:
    /**
     * @brief 导出开始信号
     */
    void exportStarted();
    
    /**
     * @brief 导出进度更新
     * @param current 当前进度
     * @param total 总进度
     */
    void exportProgress(int current, int total);
    
    /**
     * @brief 导出完成信号
     * @param success 是否成功
     * @param outputFiles 输出文件列表
     */
    void exportFinished(bool success, const QStringList &outputFiles);
    
    /**
     * @brief 导出错误信号
     * @param error 错误信息
     */
    void exportError(const QString &error);

private slots:
    void onFormatChanged();
    void onOutputDirectoryChanged();
    void onQualityChanged();
    void onSizeChanged();
    void onColorSpaceChanged();
    void onBatchModeChanged();
    void onPreviewRequested();
    void onBrowseDirectory();
    void onResetToDefaults();
    void onSavePreset();
    void onLoadPreset();

private:
    /**
     * @brief 初始化用户界面
     */
    void initializeUI();
    
    /**
     * @brief 创建格式设置组
     */
    QWidget *createFormatGroup();
    
    /**
     * @brief 创建质量设置组
     */
    QWidget *createQualityGroup();
    
    /**
     * @brief 创建尺寸设置组
     */
    QWidget *createSizeGroup();
    
    /**
     * @brief 创建颜色设置组
     */
    QWidget *createColorGroup();
    
    /**
     * @brief 创建高级选项组
     */
    QWidget *createAdvancedGroup();
    
    /**
     * @brief 创建批量设置组
     */
    QWidget *createBatchGroup();
    
    /**
     * @brief 创建预览区域
     */
    QWidget *createPreviewArea();
    
    /**
     * @brief 创建底部按钮区
     */
    QWidget *createButtonArea();
    
    /**
     * @brief 更新预览
     */
    void updatePreview();
    
    /**
     * @brief 更新界面状态
     */
    void updateUIState();
    
    /**
     * @brief 验证输入
     * @return 是否有效
     */
    bool validateInput();
    
    /**
     * @brief 执行实际导出
     * @return 是否成功
     */
    bool performExport();
    
    /**
     * @brief 导出单个图像
     * @param image 图像
     * @param outputPath 输出路径
     * @return 是否成功
     */
    bool exportSingleImage(const QImage &image, const QString &outputPath);
    
    /**
     * @brief 应用图像处理
     * @param image 原始图像
     * @return 处理后的图像
     */
    QImage applyImageProcessing(const QImage &image);
    
    /**
     * @brief 生成输出文件名
     * @param index 图像索引
     * @return 文件名
     */
    QString generateOutputFilename(int index = 0);
    
    /**
     * @brief 获取格式扩展名
     * @param format 格式
     * @return 扩展名
     */
    QString getFormatExtension(ExportFormat format);
    
    /**
     * @brief 加载预设配置
     */
    void loadPresets();
    
    /**
     * @brief 保存当前配置为预设
     */
    void saveCurrentAsPreset();

private:
    // 核心数据
    QList<QImage> m_images;
    ExportSettings m_settings;
    bool m_isExporting;
    bool m_exportCancelled;
    
    // UI组件 - 格式设置
    DComboBox *m_formatCombo;
    DLineEdit *m_outputDirEdit;
    DPushButton *m_browseDirButton;
    DLineEdit *m_filenameTemplateEdit;
    
    // UI组件 - 质量设置
    DSlider *m_jpegQualitySlider;
    DSpinBox *m_jpegQualitySpin;
    DSlider *m_pngCompressionSlider;
    DSpinBox *m_pngCompressionSpin;
    DCheckBox *m_losslessCheck;
    
    // UI组件 - 尺寸设置
    DSpinBox *m_widthSpin;
    DSpinBox *m_heightSpin;
    DSpinBox *m_dpiSpin;
    DCheckBox *m_aspectRatioCheck;
    DComboBox *m_presetsCombo;
    
    // UI组件 - 颜色设置
    DComboBox *m_colorSpaceCombo;
    DCheckBox *m_embedProfileCheck;
    
    // UI组件 - 高级选项
    DCheckBox *m_enhanceCheck;
    DCheckBox *m_removeBackgroundCheck;
    DCheckBox *m_autoRotateCheck;
    DCheckBox *m_watermarkCheck;
    DLineEdit *m_watermarkEdit;
    
    // UI组件 - 批量设置
    DCheckBox *m_batchModeCheck;
    DCheckBox *m_subfoldersCheck;
    DLineEdit *m_subfolderPatternEdit;
    
    // UI组件 - 预览和控制
    DLabel *m_previewLabel;
    DProgressBar *m_progressBar;
    DLabel *m_statusLabel;
    DPushButton *m_previewButton;
    DPushButton *m_exportButton;
    DPushButton *m_cancelButton;
    DPushButton *m_resetButton;
    
    // 预设管理
    DComboBox *m_presetCombo;
    DPushButton *m_savePresetButton;
    DPushButton *m_loadPresetButton;
    
    // 布局
    QVBoxLayout *m_mainLayout;
    QGridLayout *m_contentLayout;
    
    // 导出进度跟踪
    QTimer *m_progressTimer;
    int m_currentExportIndex;
    QStringList m_outputFiles;
};

DSCANNER_END_NAMESPACE

#endif // ADVANCEDEXPORTDIALOG_H 