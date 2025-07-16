// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ADVANCED_IMAGE_PROCESSOR_H
#define ADVANCED_IMAGE_PROCESSOR_H

#include "Scanner/DScannerGlobal.h"
#include "Scanner/DScannerTypes.h"
#include <QObject>
#include <QImage>
#include <QMutex>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QFuture>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <memory>
#include <vector>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(advancedImageProcessor)

// 像素格式定义
enum class PixelFormat {
    Unknown,
    Format1,        // 单色/灰度格式
    Format3,        // RGB格式
    Format4,        // RGBA格式  
    Format5,        // 高精度格式
    Format6,        // 专用格式
    Format8,        // 扩展格式
    Raw16Bit,       // 16位原始格式
    Raw12Bit,       // 12位原始格式
    YUV422,         // YUV 4:2:2格式
    LAB             // CIE LAB色彩空间
};

// 图像处理节点类型
enum class ProcessingNodeType {
    Source,                 // 数据源节点
    FormatConvert,         // 格式转换节点
    PixelShiftColumns,     // 像素列移位节点
    PixelShiftLines,       // 像素行移位节点
    ScaleRows,             // 行缩放节点
    Swap16BitEndian,       // 16位字节序交换节点
    Desegment,             // 图像分割节点
    DeinterleaveLines,     // 行去交错节点
    Calibrate,             // 图像校准节点
    ComponentShiftLines,   // 组件行移位节点
    ColorCorrection,       // 颜色校正节点
    NoiseReduction,        // 降噪处理节点
    Sharpening,            // 锐化处理节点
    Sink                   // 输出节点
};

// 像素数据结构
struct RawPixel {
    quint16 channels[4];    // 支持最多4个通道
    
    RawPixel() { 
        channels[0] = channels[1] = channels[2] = channels[3] = 0; 
    }
    
    RawPixel(quint16 r, quint16 g = 0, quint16 b = 0, quint16 a = 0) {
        channels[0] = r; channels[1] = g; channels[2] = b; channels[3] = a;
    }
};

struct Pixel {
    quint8 r, g, b, a;      // 标准RGBA格式
    
    Pixel() : r(0), g(0), b(0), a(255) {}
    Pixel(quint8 red, quint8 green, quint8 blue, quint8 alpha = 255) 
        : r(red), g(green), b(blue), a(alpha) {}
};

// 图像缓冲区类
class ImageBuffer {
public:
    ImageBuffer();
    ImageBuffer(int width, int height, PixelFormat format);
    ImageBuffer(const ImageBuffer &other);
    ImageBuffer &operator=(const ImageBuffer &other);
    ~ImageBuffer();
    
    // 基础属性
    int width() const { return m_width; }
    int height() const { return m_height; }
    PixelFormat format() const { return m_format; }
    int bytesPerPixel() const;
    int bytesPerLine() const;
    int totalBytes() const;
    
    // 数据访问
    quint8 *data() { return m_data.get(); }
    const quint8 *constData() const { return m_data.get(); }
    quint8 *scanLine(int y);
    const quint8 *constScanLine(int y) const;
    
    // 像素操作 - 模板实现
    template<PixelFormat format>
    Pixel getPixel(int x, int y) const;
    
    template<PixelFormat format>
    RawPixel getRawPixel(int x, int y) const;
    
    template<PixelFormat format>
    void setPixel(int x, int y, const Pixel &pixel);
    
    template<PixelFormat format>
    void setRawPixel(int x, int y, const RawPixel &pixel);
    
    // 转换方法
    QImage toQImage() const;
    bool fromQImage(const QImage &image, PixelFormat targetFormat = PixelFormat::Format3);
    
    // 复制和克隆
    ImageBuffer copy() const;
    ImageBuffer copy(const QRect &rect) const;
    
private:
    int m_width;
    int m_height;
    PixelFormat m_format;
    std::shared_ptr<quint8> m_data;
    
    void allocateData();
    void copyData(const ImageBuffer &other);
};

// 图像处理节点基类
class DSCANNER_EXPORT ImageProcessingNode : public QObject
{
    Q_OBJECT
    
public:
    explicit ImageProcessingNode(ProcessingNodeType type, QObject *parent = nullptr);
    virtual ~ImageProcessingNode();
    
    // 基础属性
    ProcessingNodeType nodeType() const { return m_nodeType; }
    QString nodeName() const { return m_nodeName; }
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    // 处理接口
    virtual bool process(const ImageBuffer &input, ImageBuffer &output) = 0;
    virtual bool canProcess(const ImageBuffer &input) const;
    virtual QVariantMap getParameters() const;
    virtual bool setParameters(const QVariantMap &params);
    
    // 连接管理
    void setNextNode(ImageProcessingNode *next) { m_nextNode = next; }
    ImageProcessingNode *nextNode() const { return m_nextNode; }
    
protected:
    ProcessingNodeType m_nodeType;
    QString m_nodeName;
    bool m_enabled;
    ImageProcessingNode *m_nextNode;
    
    // 线程安全的参数访问
    mutable QMutex m_parametersMutex;
    QVariantMap m_parameters;
};

// 数据源节点
class DSCANNER_EXPORT SourceNode : public ImageProcessingNode
{
    Q_OBJECT
    
public:
    explicit SourceNode(QObject *parent = nullptr);
    
    bool process(const ImageBuffer &input, ImageBuffer &output) override;
    
    // 数据源设置
    void setImageData(const ImageBuffer &data);
    void setImageData(const QImage &image);
    
    // 回调函数支持
    typedef std::function<bool(int row, quint8 *data)> RowCallback;
    void setRowCallback(const RowCallback &callback);
    
private:
    ImageBuffer m_sourceData;
    RowCallback m_rowCallback;
    mutable QMutex m_dataMutex;
};

// 格式转换节点
class DSCANNER_EXPORT FormatConvertNode : public ImageProcessingNode
{
    Q_OBJECT
    
public:
    explicit FormatConvertNode(QObject *parent = nullptr);
    
    bool process(const ImageBuffer &input, ImageBuffer &output) override;
    bool canProcess(const ImageBuffer &input) const override;
    
    // 转换参数
    void setTargetFormat(PixelFormat format);
    PixelFormat targetFormat() const { return m_targetFormat; }
    
    // 模板特化的转换实现
    template<PixelFormat InputFormat, PixelFormat OutputFormat>
    static bool convertPixelRow(const quint8 *input, quint8 *output, 
                               int width, const quint8 *params = nullptr);
    
    // 新增优化方法
    void enableVectorizedConversion(bool enabled) { m_vectorizedEnabled = enabled; }
    void optimizeMemoryPattern() { m_memoryOptimized = true; }

private:
    PixelFormat m_targetFormat;
    
    // 转换实现函数
    bool convertRGBToGrayscale(const ImageBuffer &input, ImageBuffer &output);
    bool convertGrayscaleToRGB(const ImageBuffer &input, ImageBuffer &output);
    bool convertRaw16ToRGB(const ImageBuffer &input, ImageBuffer &output);
    bool convertRGBToLAB(const ImageBuffer &input, ImageBuffer &output);
    
    bool m_vectorizedEnabled = false;
    bool m_memoryOptimized = false;
};

// 像素移位节点
class DSCANNER_EXPORT PixelShiftNode : public ImageProcessingNode
{
    Q_OBJECT
    
public:
    explicit PixelShiftNode(QObject *parent = nullptr);
    
    bool process(const ImageBuffer &input, ImageBuffer &output) override;
    
    // 移位参数
    void setColumnShift(int shift) { m_columnShift = shift; }
    void setLineShift(int shift) { m_lineShift = shift; }
    int columnShift() const { return m_columnShift; }
    int lineShift() const { return m_lineShift; }
    
    // 亚像素精度支持
    void setSubPixelShift(double columnShift, double lineShift);
    
private:
    int m_columnShift;
    int m_lineShift;
    double m_subPixelColumnShift;
    double m_subPixelLineShift;
    bool m_useSubPixel;
    
    bool performColumnShift(const ImageBuffer &input, ImageBuffer &output);
    bool performLineShift(const ImageBuffer &input, ImageBuffer &output);
    bool performSubPixelShift(const ImageBuffer &input, ImageBuffer &output);
};

// 颜色校正节点
class DSCANNER_EXPORT ColorCorrectionNode : public ImageProcessingNode
{
    Q_OBJECT
    
public:
    explicit ColorCorrectionNode(QObject *parent = nullptr);
    
    bool process(const ImageBuffer &input, ImageBuffer &output) override;
    
    // 校正参数
    struct ColorMatrix {
        double r[3];    // R通道的RGB系数
        double g[3];    // G通道的RGB系数  
        double b[3];    // B通道的RGB系数
        
        ColorMatrix() {
            // 默认单位矩阵
            r[0] = g[1] = b[2] = 1.0;
            r[1] = r[2] = g[0] = g[2] = b[0] = b[1] = 0.0;
        }
    };
    
    void setColorMatrix(const ColorMatrix &matrix);
    void setBrightness(double brightness);  // -100 到 100
    void setContrast(double contrast);      // -100 到 100
    void setGamma(double gamma);            // 0.1 到 3.0
    void setSaturation(double saturation);  // -100 到 100
    
    // 自动校正功能
    void setAutoWhiteBalance(bool enabled);
    void setAutoColorRestoration(bool enabled);
    
    // 使用QMatrix3x3替代自定义ColorMatrix
    void setColorMatrix(const QMatrix3x3 &matrix);
    void setWhitePoint(const QColor &whitePoint);
    void setBrightness(int brightness);    // -100 到 100
    void setContrast(int contrast);        // 0 到 200
    void setSaturation(int saturation);    // 0 到 200
    void setGamma(double gamma);           // 0.1 到 3.0
    
    // SIMD优化支持
    void enableSIMDProcessing(bool enabled) { m_simdEnabled = enabled; }
    void optimizeLookupTables() { m_optimizedLUT = true; }

private:
    ColorMatrix m_colorMatrix;
    double m_brightness;
    double m_contrast;
    double m_gamma;
    double m_saturation;
    bool m_autoWhiteBalance;
    bool m_autoColorRestoration;
    
    // 校正算法实现
    bool applyColorMatrix(const ImageBuffer &input, ImageBuffer &output);
    bool applyBrightnessContrast(ImageBuffer &buffer);
    bool applyGammaCorrection(ImageBuffer &buffer);
    bool applySaturationAdjustment(ImageBuffer &buffer);
    bool performAutoWhiteBalance(ImageBuffer &buffer);
    bool performAutoColorRestoration(ImageBuffer &buffer);
    
    // SIMD优化的处理函数
    void applyColorMatrix_SSE2(quint8 *data, int pixels, const ColorMatrix &matrix);
    void applyGamma_AVX2(quint8 *data, int pixels, double gamma);
    
    bool m_simdEnabled = false;
    bool m_optimizedLUT = false;
};

// 降噪处理节点
class DSCANNER_EXPORT NoiseReductionNode : public ImageProcessingNode
{
    Q_OBJECT
    
public:
    explicit NoiseReductionNode(QObject *parent = nullptr);
    
    bool process(const ImageBuffer &input, ImageBuffer &output) override;
    
    // 降噪算法类型
    enum class NoiseReductionType {
        None,
        Gaussian,       // 高斯降噪
        Bilateral,      // 双边滤波
        NonLocal,       // 非局部均值
        Wavelet,        // 小波降噪
        MedianFilter    // 中值滤波
    };
    
    void setNoiseReductionType(NoiseReductionType type);
    void setStrength(double strength);     // 0.0 到 1.0
    void setPreserveDetails(bool preserve);
    
private:
    NoiseReductionType m_type;
    double m_strength;
    bool m_preserveDetails;
    
    // 不同降噪算法的实现
    bool applyGaussianNoise(const ImageBuffer &input, ImageBuffer &output);
    bool applyBilateralFilter(const ImageBuffer &input, ImageBuffer &output);
    bool applyNonLocalMeans(const ImageBuffer &input, ImageBuffer &output);
    bool applyWaveletDenoising(const ImageBuffer &input, ImageBuffer &output);
    bool applyMedianFilter(const ImageBuffer &input, ImageBuffer &output);
};

// 图像处理管道类
class DSCANNER_EXPORT AdvancedImageProcessor : public QObject
{
    Q_OBJECT
    
public:
    explicit AdvancedImageProcessor(QObject *parent = nullptr);
    ~AdvancedImageProcessor() override;
    
    // 管道管理
    void addNode(ImageProcessingNode *node);
    void insertNode(int index, ImageProcessingNode *node);
    void removeNode(ImageProcessingNode *node);
    void removeNode(int index);
    void clearNodes();
    
    QList<ImageProcessingNode*> nodes() const { return m_nodes; }
    int nodeCount() const { return m_nodes.size(); }
    
    // 处理控制
    bool processImage(const ImageBuffer &input, ImageBuffer &output);
    bool processImage(const QImage &input, QImage &output);
    
    // 异步处理支持
    QFuture<ImageBuffer> processImageAsync(const ImageBuffer &input);
    QFuture<QImage> processImageAsync(const QImage &input);
    
    // 批处理支持
    QFuture<QList<ImageBuffer>> processBatch(const QList<ImageBuffer> &inputs);
    
    // 管道配置
    void saveConfiguration(const QString &configPath) const;
    bool loadConfiguration(const QString &configPath);
    
    // 性能统计
    struct ProcessingStats {
        qint64 totalProcessingTime;     // 总处理时间(ms)
        qint64 averageProcessingTime;   // 平均处理时间(ms)
        int processedImages;            // 已处理图像数量
        QStringList nodeTimings;        // 各节点耗时
    };
    ProcessingStats getProcessingStats() const;
    void resetStats();
    
    // 内存管理
    void setMaxMemoryUsage(qint64 maxBytes);
    qint64 getCurrentMemoryUsage() const;
    void optimizeMemoryUsage();

    // 新增SIMD优化接口
    /**
     * @brief 启用SIMD优化
     * @return true表示成功启用SIMD优化
     */
    bool enableSIMDOptimization();
    
    /**
     * @brief 检查SIMD优化是否已启用
     * @return true表示SIMD优化已启用
     */
    bool isSIMDEnabled() const { return m_simdEnabled; }
    
    /**
     * @brief 使用SIMD优化处理图像
     * @param image 输入图像
     * @param params 处理参数
     * @return 处理后的图像
     */
    QImage processImageWithSIMD(const QImage &image, const ProcessingParameters &params);
    
    /**
     * @brief 异步SIMD图像处理
     * @param image 输入图像
     * @param params 处理参数
     * @return Future对象
     */
    QFuture<QImage> processImageAsyncSIMD(const QImage &image, const ProcessingParameters &params);
    
    /**
     * @brief 批量SIMD处理
     * @param images 输入图像列表
     * @param params 处理参数
     * @return Future对象
     */
    QFuture<QList<QImage>> processBatchSIMD(const QList<QImage> &images, const ProcessingParameters &params);
    
    /**
     * @brief 优化颜色校正节点性能
     * @return true表示优化成功
     */
    bool optimizeColorCorrectionNode();
    
    /**
     * @brief 优化格式转换节点性能
     * @return true表示优化成功
     */
    bool optimizeFormatConversionNode();
    
    /**
     * @brief 优化内存对齐
     * @return true表示优化成功
     */
    bool optimizeMemoryAlignment();
    
    // 性能统计结构
    struct PerformanceStats {
        qint64 totalProcessingTime = 0;     // 总处理时间（毫秒）
        qint64 totalProcessedPixels = 0;    // 总处理像素数
        qint64 processingCount = 0;         // 处理次数
        double averageProcessingTime = 0.0; // 平均处理时间
        double pixelsPerSecond = 0.0;       // 处理速度（像素/秒）
    };
    
    /**
     * @brief 获取性能统计信息
     * @return 性能统计数据
     */
    PerformanceStats getPerformanceStats() const { return m_stats; }
    
    /**
     * @brief 重置性能统计
     */
    void resetPerformanceStats() { m_stats = PerformanceStats(); }

    // 处理参数结构
    struct ProcessingParameters {
        double brightness = 0.0;           // 亮度调整 (-100 到 100)
        double contrast = 0.0;             // 对比度调整 (-100 到 100)
        double saturation = 0.0;           // 饱和度调整 (-100 到 100)
        int gaussianBlurRadius = 0;        // 高斯模糊半径
        double gaussianBlurSigma = -1.0;   // 高斯模糊标准差
        bool convertToGrayscale = false;   // 是否转换为灰度
        bool enableAutoCorrection = false; // 自动色彩校正
        bool enableNoiseReduction = false; // 降噪处理
    };

signals:
    void processingStarted();
    void processingFinished();
    void processingProgress(int percentage);
    void nodeProcessingStarted(int nodeIndex, const QString &nodeName);
    void nodeProcessingFinished(int nodeIndex, const QString &nodeName, qint64 elapsedTime);
    void errorOccurred(const QString &error);

private slots:
    void onNodeProcessingFinished();
    void onProcessingCompleted();
    void onProcessingError(const QString &error);

private:
    QList<ImageProcessingNode*> m_nodes;
    mutable QMutex m_nodesMutex;
    
    // 性能统计
    ProcessingStats m_stats;
    mutable QMutex m_statsMutex;
    
    // 内存管理
    qint64 m_maxMemoryUsage;
    qint64 m_currentMemoryUsage;
    
    // 异步处理
    QThreadPool *m_threadPool;
    
    // 内部处理方法
    bool processInternal(const ImageBuffer &input, ImageBuffer &output);
    void updateMemoryUsage(qint64 change);
    void updateNodeStats(int nodeIndex, qint64 elapsedTime);
    
    // 内存优化
    void freeUnusedBuffers();
    bool canAllocateMemory(qint64 bytes) const;
    
    // SIMD优化相关成员
    bool m_simdEnabled = false;         // SIMD优化是否启用
    PerformanceStats m_stats;           // 性能统计数据
    
    // 性能配置
    struct Config {
        bool enableParallelProcessing = true;
        bool enableMemoryAlignment = false;
        int maxConcurrentJobs = 4;
        int memoryAlignment = 16;
        int cacheLineSize = 64;
    } m_config;
};

// 异步处理任务类
class ImageProcessingTask : public QObject, public QRunnable
{
    Q_OBJECT
    
public:
    ImageProcessingTask(AdvancedImageProcessor *processor, 
                       const ImageBuffer &input,
                       QObject *parent = nullptr);
    
    void run() override;
    ImageBuffer result() const { return m_result; }
    bool hasError() const { return m_hasError; }
    QString errorString() const { return m_errorString; }

signals:
    void finished();
    void errorOccurred(const QString &error);

private:
    AdvancedImageProcessor *m_processor;
    ImageBuffer m_input;
    ImageBuffer m_result;
    bool m_hasError;
    QString m_errorString;
};

DSCANNER_END_NAMESPACE

#endif // ADVANCED_IMAGE_PROCESSOR_H 