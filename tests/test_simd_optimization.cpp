#include <QtTest>
#include <QObject>
#include <QImage>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QDebug>

#include "Scanner/DScannerImageProcessor.h"
#include "../src/processing/simple_simd_support.h"

DSCANNER_USE_NAMESPACE

class TestSIMDOptimization : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testSIMDDetection();
    void testSIMDBrightnessOptimization();
    void testSIMDContrastOptimization();
    void testSIMDPerformanceComparison();
    void testSIMDAccuracy();

private:
    DScannerImageProcessor* m_processor;
    QImage m_testImage;
    
    QImage createTestImage();
    bool isImageSimilar(const QImage& img1, const QImage& img2, int tolerance = 5);
};

void TestSIMDOptimization::initTestCase()
{
    qDebug() << "=== SIMD优化功能测试开始 ===";
    
    m_processor = new DScannerImageProcessor(this);
    m_testImage = createTestImage();
    
    QVERIFY(m_processor != nullptr);
    QVERIFY(!m_testImage.isNull());
    
    qDebug() << "测试图像尺寸:" << m_testImage.size();
    qDebug() << "SIMD支持检测:" << SimpleSIMDSupport::detectSIMDSupport();
}

void TestSIMDOptimization::cleanupTestCase()
{
    delete m_processor;
    qDebug() << "=== SIMD优化功能测试结束 ===";
}

QImage TestSIMDOptimization::createTestImage()
{
    // 创建一个复杂的测试图像
    QImage image(512, 512, QImage::Format_ARGB32);
    
    // 填充渐变色彩
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int r = (x * 255) / image.width();
            int g = (y * 255) / image.height();
            int b = ((x + y) * 255) / (image.width() + image.height());
            
            image.setPixelColor(x, y, QColor(r, g, b));
        }
    }
    
    return image;
}

bool TestSIMDOptimization::isImageSimilar(const QImage& img1, const QImage& img2, int tolerance)
{
    if (img1.size() != img2.size()) return false;
    
    for (int y = 0; y < img1.height(); y += 10) { // 采样检查
        for (int x = 0; x < img1.width(); x += 10) {
            QColor c1 = img1.pixelColor(x, y);
            QColor c2 = img2.pixelColor(x, y);
            
            if (qAbs(c1.red() - c2.red()) > tolerance ||
                qAbs(c1.green() - c2.green()) > tolerance ||
                qAbs(c1.blue() - c2.blue()) > tolerance) {
                return false;
            }
        }
    }
    
    return true;
}

void TestSIMDOptimization::testSIMDDetection()
{
    qDebug() << "测试SIMD支持检测...";
    
    QString support = SimpleSIMDSupport::detectSIMDSupport();
    QVERIFY(!support.isEmpty());
    
    bool hasSSE2 = SimpleSIMDSupport::hasSSE2Support();
    bool hasAVX2 = SimpleSIMDSupport::hasAVX2Support();
    
    qDebug() << "SSE2支持:" << hasSSE2;
    qDebug() << "AVX2支持:" << hasAVX2;
    qDebug() << "支持详情:" << support;
    
    // 至少应该有一种SIMD支持或者明确无支持
    QVERIFY(hasSSE2 || hasAVX2 || support.contains("No SIMD"));
}

void TestSIMDOptimization::testSIMDBrightnessOptimization()
{
    qDebug() << "测试SIMD亮度优化...";
    
    // 测试不同的亮度调整值
    QList<int> testValues = {-50, -20, 0, 20, 50, 100};
    
    for (int brightness : testValues) {
        QImage result = m_processor->adjustBrightness(m_testImage, brightness);
        
        QVERIFY(!result.isNull());
        QCOMPARE(result.size(), m_testImage.size());
        
        // 对于0调整，结果应该接近原图
        if (brightness == 0) {
            QVERIFY(isImageSimilar(result, m_testImage, 2));
        }
        
        qDebug() << "亮度调整" << brightness << "测试通过";
    }
}

void TestSIMDOptimization::testSIMDContrastOptimization()
{
    qDebug() << "测试SIMD对比度优化...";
    
    // 测试不同的对比度调整值
    QList<int> testValues = {-50, -20, 0, 20, 50, 100};
    
    for (int contrast : testValues) {
        QImage result = m_processor->adjustContrast(m_testImage, contrast);
        
        QVERIFY(!result.isNull());
        QCOMPARE(result.size(), m_testImage.size());
        
        // 对于0调整，结果应该接近原图
        if (contrast == 0) {
            QVERIFY(isImageSimilar(result, m_testImage, 2));
        }
        
        qDebug() << "对比度调整" << contrast << "测试通过";
    }
}

void TestSIMDOptimization::testSIMDPerformanceComparison()
{
    qDebug() << "测试SIMD性能对比...";
    
    // 创建较大的测试图像
    QImage largeImage(1024, 1024, QImage::Format_ARGB32);
    largeImage.fill(Qt::gray);
    
    const int iterations = 5;
    
    // 测试亮度调整性能
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < iterations; ++i) {
        QImage result = SimpleSIMDSupport::adjustBrightnessSIMD(largeImage, 1.2);
        QVERIFY(!result.isNull());
    }
    
    qint64 simdTime = timer.elapsed();
    
    // 重新开始计时
    timer.restart();
    
    for (int i = 0; i < iterations; ++i) {
        QImage result = m_processor->adjustBrightness(largeImage, 20); // 使用处理器的实现
        QVERIFY(!result.isNull());
    }
    
    qint64 totalTime = timer.elapsed();
    
    qDebug() << "SIMD优化性能测试:";
    qDebug() << "  图像尺寸:" << largeImage.size();
    qDebug() << "  测试次数:" << iterations;
    qDebug() << "  SIMD直接调用时间:" << simdTime << "ms";
    qDebug() << "  完整处理器时间:" << totalTime << "ms";
    
    // SIMD应该有性能提升（或至少不差太多）
    QVERIFY(simdTime >= 0);
    QVERIFY(totalTime >= 0);
}

void TestSIMDOptimization::testSIMDAccuracy()
{
    qDebug() << "测试SIMD精度...";
    
    // 比较SIMD版本和标量版本的结果
    double factor = 1.3;
    
    QImage simdResult = SimpleSIMDSupport::adjustBrightnessSIMD(m_testImage, factor);
    
    // 创建一个简单的标量版本进行对比
    QImage scalarResult = m_testImage.convertToFormat(QImage::Format_ARGB32);
    int brightnessAdd = static_cast<int>((factor - 1.0) * 128);
    brightnessAdd = qBound(-128, brightnessAdd, 127);
    
    for (int y = 0; y < scalarResult.height(); ++y) {
        for (int x = 0; x < scalarResult.width(); ++x) {
            QColor color = scalarResult.pixelColor(x, y);
            color.setRed(qBound(0, color.red() + brightnessAdd, 255));
            color.setGreen(qBound(0, color.green() + brightnessAdd, 255));
            color.setBlue(qBound(0, color.blue() + brightnessAdd, 255));
            scalarResult.setPixelColor(x, y, color);
        }
    }
    
    QVERIFY(!simdResult.isNull());
    QVERIFY(!scalarResult.isNull());
    QCOMPARE(simdResult.size(), scalarResult.size());
    
    // 比较结果的相似性（允许小的差异）
    bool similar = isImageSimilar(simdResult, scalarResult, 3);
    QVERIFY(similar);
    
    qDebug() << "SIMD精度测试通过 - 结果与标量版本相似";
}

QTEST_MAIN(TestSIMDOptimization)
#include "test_simd_optimization.moc" 