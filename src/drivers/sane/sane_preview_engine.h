#ifndef SANE_PREVIEW_ENGINE_H
#define SANE_PREVIEW_ENGINE_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <sane/sane.h>

DSCANNER_BEGIN_NAMESPACE

class SANEOptionManager;

/**
 * @brief SANE预览扫描引擎
 * 
 * 基于先进技术的预览扫描机制，提供高效的预览功能
 * 主要特性：
 * - 快速预览扫描
 * - 智能分辨率调整
 * - 异步处理支持
 * - 实时进度反馈
 */
class SANEPreviewEngine : public QObject
{
    Q_OBJECT

public:
    explicit SANEPreviewEngine(SANE_Handle handle, SANEOptionManager *optionManager, 
                               QObject *parent = nullptr);
    ~SANEPreviewEngine();

    /**
     * @brief 执行预览扫描
     * @param region 扫描区域，空表示全区域
     * @return 预览图像，失败返回空图像
     */
    QImage performPreviewScan(const QRect &region = QRect());

    /**
     * @brief 异步执行预览扫描
     * @param region 扫描区域
     */
    void performPreviewScanAsync(const QRect &region = QRect());

    /**
     * @brief 取消当前预览扫描
     */
    void cancelPreviewScan();

    /**
     * @brief 检查是否正在扫描
     * @return 正在扫描返回true
     */
    bool isScanning() const;

    /**
     * @brief 获取扫描进度
     * @return 进度百分比(0-100)
     */
    int getScanProgress() const;

    /**
     * @brief 设置预览质量
     * @param quality 质量级别(1-5)
     */
    void setPreviewQuality(int quality);

    /**
     * @brief 获取预览质量
     * @return 质量级别
     */
    int getPreviewQuality() const;

signals:
    /**
     * @brief 预览扫描开始信号
     */
    void previewScanStarted();

    /**
     * @brief 预览扫描完成信号
     * @param image 预览图像
     */
    void previewScanFinished(const QImage &image);

    /**
     * @brief 预览扫描失败信号
     * @param error 错误信息
     */
    void previewScanFailed(const QString &error);

    /**
     * @brief 扫描进度更新信号
     * @param progress 进度百分比
     */
    void scanProgressUpdated(int progress);

    /**
     * @brief 扫描已取消信号
     */
    void scanCancelled();

private slots:
    void onScanTimeout();
    void updateScanProgress();

private:
    struct PreviewParameters {
        int resolution;           ///< 预览分辨率
        QRect scanArea;          ///< 扫描区域
        QString colorMode;       ///< 色彩模式
        int depth;              ///< 色彩深度
        bool autoExposure;      ///< 自动曝光
        
        PreviewParameters() 
            : resolution(75), depth(8), autoExposure(true) {}
    };

    /**
     * @brief 初始化预览参数
     */
    void initializePreviewParameters();

    /**
     * @brief 设置预览扫描参数
     * @param params 预览参数
     * @return 成功返回true
     */
    bool setupPreviewParameters(const PreviewParameters &params);

    /**
     * @brief 执行实际的扫描操作
     * @return 扫描数据
     */
    QByteArray performActualScan();

    /**
     * @brief 将扫描数据转换为图像
     * @param data 扫描数据
     * @param params 扫描参数
     * @return 转换后的图像
     */
    QImage convertScanDataToImage(const QByteArray &data, const PreviewParameters &params);

    /**
     * @brief 获取最佳预览分辨率
     * @param targetSize 目标尺寸
     * @return 最佳分辨率
     */
    int getOptimalPreviewResolution(const QSize &targetSize) const;

    /**
     * @brief 应用图像增强
     * @param image 原始图像
     * @return 增强后的图像
     */
    QImage enhancePreviewImage(const QImage &image) const;

    /**
     * @brief 检查SANE选项值
     * @param optionName 选项名称
     * @param value 要设置的值
     * @return 成功返回true
     */
    bool setSANEOption(const QString &optionName, const QVariant &value);

    /**
     * @brief 备份当前SANE设置
     */
    void backupCurrentSettings();

    /**
     * @brief 恢复SANE设置
     */
    void restoreSettings();

    /**
     * @brief 重置扫描状态
     */
    void resetScanState();

    SANE_Handle m_handle;                   ///< SANE设备句柄
    SANEOptionManager *m_optionManager;     ///< 选项管理器
    
    mutable QMutex m_scanMutex;             ///< 扫描操作互斥锁
    bool m_isScanning;                      ///< 是否正在扫描
    bool m_cancelRequested;                 ///< 是否请求取消
    int m_scanProgress;                     ///< 扫描进度
    int m_previewQuality;                   ///< 预览质量
    
    PreviewParameters m_currentParams;      ///< 当前预览参数
    QHash<QString, QVariant> m_backupSettings; ///< 备份的设置
    
    QTimer *m_scanTimer;                    ///< 扫描超时定时器
    QTimer *m_progressTimer;                ///< 进度更新定时器
    
    static const int DEFAULT_PREVIEW_RESOLUTION;  ///< 默认预览分辨率
    static const int SCAN_TIMEOUT_MS;              ///< 扫描超时时间
    static const int PROGRESS_UPDATE_INTERVAL_MS;  ///< 进度更新间隔
};

DSCANNER_END_NAMESPACE

#endif // SANE_PREVIEW_ENGINE_H 