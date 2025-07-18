// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_integration.cpp
 * @brief DeepinScan集成测试套件 - 现代化架构的端到端测试
 * 
 * 本测试文件验证整个DeepinScan系统的集成功能：
 * - SANE驱动与设备管理器集成
 * - 网络发现与设备连接集成
 * - 图像处理管道端到端测试
 * - GUI组件与后端服务集成
 * - 多线程和并发处理测试
 * - 完整扫描工作流测试
 */

#include <QTest>
#include <QObject>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>

#include "../src/core/dscannermanager.h"
#include "../src/drivers/sane/dscannersane.h"
#include "../src/communication/network/dscannernetworkdiscovery.h"
#include "../src/processing/advanced_image_processor.h"
#include "../src/gui/mainwindow.h"
#include "../src/gui/widgets/scancontrolwidget.h"
#include "../src/gui/widgets/devicelistwidget.h"
#include "../src/gui/widgets/imagepreviewwidget.h"

DSCANNER_USE_NAMESPACE

class TestIntegration : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // 核心系统集成测试
    void testScannerManagerInitialization();
    void testSANEDriverIntegration();
    void testNetworkDiscoveryIntegration();
    void testDeviceManagementIntegration();
    
    // 图像处理管道集成测试
    void testImageProcessingPipelineIntegration();
    void testAdvancedImageProcessorIntegration();
    void testMultiThreadedImageProcessing();
    void testBatchImageProcessingIntegration();
    
    // GUI组件集成测试
    void testMainWindowIntegration();
    void testScanControlWidgetIntegration();
    void testDeviceListWidgetIntegration();
    void testImagePreviewWidgetIntegration();
    
    // 完整扫描工作流测试
    void testCompleteVirtualScanWorkflow();
    void testScanParameterConfiguration();
    void testScanProgressMonitoring();
    void testScanResultHandling();
    
    // 并发和性能集成测试
    void testConcurrentDeviceDiscovery();
    void testConcurrentImageProcessing();
    void testMemoryManagementIntegration();
    void testResourceCleanupIntegration();
    
    // 错误处理和恢复测试
    void testErrorHandlingIntegration();
    void testDeviceConnectionErrorRecovery();
    void testNetworkErrorRecovery();
    void testImageProcessingErrorRecovery();
    
    // 性能基准集成测试
    void benchmarkFullScanWorkflow();
    void benchmarkConcurrentOperations();
    void benchmarkMemoryUsage();
    void benchmarkSystemResponsiveness();

private:
    // 测试辅助方法
    void setupVirtualTestEnvironment();
    void createMockScannerDevices();
    void simulateNetworkDevices();
    bool waitForSignal(QObject *sender, const char *signal, int timeout = 5000);
    void verifySystemState();
    
    // 测试数据
    DScannerManager *m_manager;
    AdvancedImageProcessor *m_imageProcessor;
    MainWindow *m_mainWindow;
    QTemporaryDir *m_tempDir;
    QList<DScannerDevice*> m_testDevices;
};

void TestIntegration::initTestCase()
{
    qDebug() << "开始DeepinScan集成测试 - 现代化架构验证";
    
    // 创建临时目录
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // 初始化核心组件
    m_manager = new DScannerManager();
    QVERIFY(m_manager != nullptr);
    
    m_imageProcessor = new AdvancedImageProcessor();
    QVERIFY(m_imageProcessor != nullptr);
    
    // 设置虚拟测试环境
    setupVirtualTestEnvironment();
    
    qDebug() << "集成测试环境初始化完成";
    qDebug() << "临时目录:" << m_tempDir->path();
}

void TestIntegration::cleanupTestCase()
{
    // 清理测试设备
    qDeleteAll(m_testDevices);
    m_testDevices.clear();
    
    // 清理主要组件
    delete m_mainWindow;
    delete m_imageProcessor;
    delete m_manager;
    delete m_tempDir;
    
    qDebug() << "集成测试清理完成";
}

void TestIntegration::init()
{
    // 每个测试前重置状态
    if (m_manager) {
        m_manager->clearDevices();
    }
    
    if (m_imageProcessor) {
        m_imageProcessor->clearNodes();
        m_imageProcessor->resetStats();
    }
}

void TestIntegration::cleanup()
{
    // 测试后清理
}

// =============================================================================
// 核心系统集成测试
// =============================================================================

void TestIntegration::testScannerManagerInitialization()
{
    qDebug() << "测试扫描仪管理器初始化集成";
    
    // 验证管理器初始状态
    QVERIFY(m_manager != nullptr);
    QCOMPARE(m_manager->deviceCount(), 0);
    QVERIFY(m_manager->availableDevices().isEmpty());
    
    // 测试初始化过程
    QSignalSpy initSpy(m_manager, &DScannerManager::initialized);
    m_manager->initialize();
    
    QVERIFY(waitForSignal(m_manager, SIGNAL(initialized()), 3000));
    QCOMPARE(initSpy.count(), 1);
    
    // 验证初始化后状态
    QVERIFY(m_manager->isInitialized());
    
    qDebug() << "扫描仪管理器初始化集成测试完成";
}

void TestIntegration::testSANEDriverIntegration()
{
    qDebug() << "测试SANE驱动集成";
    
    // 创建SANE驱动实例
    DScannerSANE *saneDriver = new DScannerSANE();
    QVERIFY(saneDriver != nullptr);
    
    // 测试SANE驱动初始化
    QSignalSpy initSpy(saneDriver, &DScannerSANE::initialized);
    saneDriver->initialize();
    
    if (waitForSignal(saneDriver, SIGNAL(initialized()), 5000)) {
        QCOMPARE(initSpy.count(), 1);
        QVERIFY(saneDriver->isInitialized());
        
        // 测试设备枚举
        QSignalSpy devicesSpy(saneDriver, &DScannerSANE::devicesFound);
        saneDriver->scanDevices();
        
        if (waitForSignal(saneDriver, SIGNAL(devicesFound()), 5000)) {
            QVERIFY(devicesSpy.count() >= 1);
            
            // 验证发现的设备
            QList<DScannerDevice*> devices = saneDriver->availableDevices();
            for (auto device : devices) {
                QVERIFY(device != nullptr);
                QVERIFY(!device->name().isEmpty());
                qDebug() << "发现SANE设备:" << device->name();
            }
        }
    } else {
        qDebug() << "SANE驱动初始化超时 - 可能系统没有安装SANE或没有扫描仪设备";
        QSKIP("跳过SANE集成测试 - 环境不支持");
    }
    
    delete saneDriver;
    qDebug() << "SANE驱动集成测试完成";
}

void TestIntegration::testNetworkDiscoveryIntegration()
{
    qDebug() << "测试网络发现集成";
    
    // 创建网络发现实例
    DScannerNetworkDiscovery *networkDiscovery = new DScannerNetworkDiscovery();
    QVERIFY(networkDiscovery != nullptr);
    
    // 设置发现参数
    networkDiscovery->setTimeout(3000); // 3秒超时
    networkDiscovery->setDiscoveryMethods({
        DScannerNetworkDiscovery::DiscoveryMethod::MDNS,
        DScannerNetworkDiscovery::DiscoveryMethod::WSD,
        DScannerNetworkDiscovery::DiscoveryMethod::SOAP
    });
    
    // 测试网络发现
    QSignalSpy discoveryStartedSpy(networkDiscovery, &DScannerNetworkDiscovery::discoveryStarted);
    QSignalSpy discoveryFinishedSpy(networkDiscovery, &DScannerNetworkDiscovery::discoveryFinished);
    QSignalSpy deviceFoundSpy(networkDiscovery, &DScannerNetworkDiscovery::deviceFound);
    
    networkDiscovery->startDiscovery();
    
    QVERIFY(waitForSignal(networkDiscovery, SIGNAL(discoveryStarted()), 1000));
    QCOMPARE(discoveryStartedSpy.count(), 1);
    
    // 等待发现完成
    QVERIFY(waitForSignal(networkDiscovery, SIGNAL(discoveryFinished()), 5000));
    QCOMPARE(discoveryFinishedSpy.count(), 1);
    
    // 检查发现的设备
    qDebug() << "网络发现完成，发现" << deviceFoundSpy.count() << "个设备";
    
    // 验证发现的设备信息
    QList<DScannerDevice*> networkDevices = networkDiscovery->discoveredDevices();
    for (auto device : networkDevices) {
        QVERIFY(device != nullptr);
        QVERIFY(!device->networkAddress().isEmpty());
        QVERIFY(device->port() > 0);
        qDebug() << "发现网络设备:" << device->name() << "地址:" << device->networkAddress();
    }
    
    delete networkDiscovery;
    qDebug() << "网络发现集成测试完成";
}

void TestIntegration::testDeviceManagementIntegration()
{
    qDebug() << "测试设备管理集成";
    
    // 确保管理器已初始化
    if (!m_manager->isInitialized()) {
        m_manager->initialize();
        QVERIFY(waitForSignal(m_manager, SIGNAL(initialized()), 3000));
    }
    
    // 创建虚拟测试设备
    createMockScannerDevices();
    
    // 测试设备添加
    QSignalSpy deviceAddedSpy(m_manager, &DScannerManager::deviceAdded);
    
    for (auto device : m_testDevices) {
        m_manager->addDevice(device);
    }
    
    QCOMPARE(deviceAddedSpy.count(), m_testDevices.size());
    QCOMPARE(m_manager->deviceCount(), m_testDevices.size());
    
    // 测试设备查找
    for (auto device : m_testDevices) {
        DScannerDevice *foundDevice = m_manager->findDevice(device->id());
        QVERIFY(foundDevice != nullptr);
        QCOMPARE(foundDevice->id(), device->id());
    }
    
    // 测试设备移除
    QSignalSpy deviceRemovedSpy(m_manager, &DScannerManager::deviceRemoved);
    
    if (!m_testDevices.isEmpty()) {
        DScannerDevice *deviceToRemove = m_testDevices.first();
        m_manager->removeDevice(deviceToRemove->id());
        
        QCOMPARE(deviceRemovedSpy.count(), 1);
        QCOMPARE(m_manager->deviceCount(), m_testDevices.size() - 1);
        
        // 验证设备已移除
        DScannerDevice *removedDevice = m_manager->findDevice(deviceToRemove->id());
        QVERIFY(removedDevice == nullptr);
    }
    
    qDebug() << "设备管理集成测试完成";
}

// =============================================================================
// 图像处理管道集成测试
// =============================================================================

void TestIntegration::testImageProcessingPipelineIntegration()
{
    qDebug() << "测试图像处理管道集成";
    
    // 创建完整的图像处理管道
    SourceNode *sourceNode = new SourceNode(m_imageProcessor);
    FormatConvertNode *convertNode = new FormatConvertNode(m_imageProcessor);
    PixelShiftNode *shiftNode = new PixelShiftNode(m_imageProcessor);
    ColorCorrectionNode *colorNode = new ColorCorrectionNode(m_imageProcessor);
    NoiseReductionNode *noiseNode = new NoiseReductionNode(m_imageProcessor);
    
    // 配置处理节点
    sourceNode->setGenerateTestPattern(true);
    sourceNode->setTestPatternType(SourceNode::TestPatternType::ColorBars);
    
    convertNode->setTargetFormat(PixelFormat::Format3);
    
    shiftNode->setColumnShift(2);
    shiftNode->setLineShift(1);
    
    colorNode->setBrightness(10.0);
    colorNode->setContrast(15.0);
    colorNode->setGamma(1.2);
    
    noiseNode->setStrength(30);
    noiseNode->setMethod(NoiseReductionNode::NoiseReductionMethod::Gaussian);
    
    // 构建处理管道
    m_imageProcessor->addNode(sourceNode);
    m_imageProcessor->addNode(convertNode);
    m_imageProcessor->addNode(shiftNode);
    m_imageProcessor->addNode(colorNode);
    m_imageProcessor->addNode(noiseNode);
    
    QCOMPARE(m_imageProcessor->nodeCount(), 5);
    
    // 测试管道处理
    ImageBuffer input; // 空输入，由SourceNode生成
    ImageBuffer output;
    
    QSignalSpy processingSpy(m_imageProcessor, &AdvancedImageProcessor::processingStarted);
    QSignalSpy finishedSpy(m_imageProcessor, &AdvancedImageProcessor::processingFinished);
    
    bool success = m_imageProcessor->processImage(input, output);
    
    QVERIFY(success);
    QCOMPARE(processingSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);
    
    // 验证输出图像
    QVERIFY(!output.isNull());
    QVERIFY(output.width() > 0);
    QVERIFY(output.height() > 0);
    QCOMPARE(output.format(), PixelFormat::Format3);
    
    // 验证处理统计
    auto stats = m_imageProcessor->getProcessingStats();
    QVERIFY(stats.totalProcessingTime > 0);
    QCOMPARE(stats.processedImages, 1);
    QCOMPARE(stats.nodeTimings.size(), 5);
    
    qDebug() << "图像处理管道集成测试完成";
    qDebug() << "处理时间:" << stats.totalProcessingTime << "ms";
}

void TestIntegration::testAdvancedImageProcessorIntegration()
{
    qDebug() << "测试高级图像处理器集成";
    
    // 测试异步处理
    SourceNode *sourceNode = new SourceNode(m_imageProcessor);
    sourceNode->setGenerateTestPattern(true);
    sourceNode->setTestPatternType(SourceNode::TestPatternType::Gradient);
    
    m_imageProcessor->addNode(sourceNode);
    
    ImageBuffer input;
    
    // 测试异步图像处理
    QFuture<ImageBuffer> future = m_imageProcessor->processImageAsync(input);
    
    // 等待异步处理完成
    future.waitForFinished();
    
    QVERIFY(future.isFinished());
    ImageBuffer result = future.result();
    
    QVERIFY(!result.isNull());
    QVERIFY(result.width() > 0);
    QVERIFY(result.height() > 0);
    
    qDebug() << "高级图像处理器集成测试完成";
}

void TestIntegration::testMultiThreadedImageProcessing()
{
    qDebug() << "测试多线程图像处理";
    
    // 创建多个图像处理任务
    QList<ImageBuffer> inputImages;
    
    for (int i = 0; i < 5; i++) {
        ImageBuffer buffer(200 + i * 50, 150 + i * 30, PixelFormat::Format3);
        
        // 填充测试数据
        for (int y = 0; y < buffer.height(); y++) {
            quint8 *line = buffer.scanLine(y);
            for (int x = 0; x < buffer.width(); x++) {
                line[x * 3 + 0] = static_cast<quint8>((x + i * 50) % 256);
                line[x * 3 + 1] = static_cast<quint8>((y + i * 30) % 256);
                line[x * 3 + 2] = static_cast<quint8>((x + y + i * 10) % 256);
            }
        }
        
        inputImages.append(buffer);
    }
    
    // 设置处理管道
    ColorCorrectionNode *colorNode = new ColorCorrectionNode(m_imageProcessor);
    colorNode->setBrightness(20.0);
    
    NoiseReductionNode *noiseNode = new NoiseReductionNode(m_imageProcessor);
    noiseNode->setStrength(25);
    
    m_imageProcessor->addNode(colorNode);
    m_imageProcessor->addNode(noiseNode);
    
    // 测试批处理
    QElapsedTimer timer;
    timer.start();
    
    QFuture<QList<ImageBuffer>> batchFuture = m_imageProcessor->processBatch(inputImages);
    batchFuture.waitForFinished();
    
    qint64 batchTime = timer.elapsed();
    
    QVERIFY(batchFuture.isFinished());
    QList<ImageBuffer> results = batchFuture.result();
    
    QCOMPARE(results.size(), inputImages.size());
    
    // 验证每个结果
    for (int i = 0; i < results.size(); i++) {
        const ImageBuffer &result = results[i];
        const ImageBuffer &input = inputImages[i];
        
        QCOMPARE(result.width(), input.width());
        QCOMPARE(result.height(), input.height());
        QCOMPARE(result.format(), input.format());
    }
    
    qDebug() << "多线程批处理完成，耗时:" << batchTime << "ms";
    qDebug() << "平均每张图像处理时间:" << (batchTime / inputImages.size()) << "ms";
}

void TestIntegration::testBatchImageProcessingIntegration()
{
    qDebug() << "测试批量图像处理集成";
    
    // 创建批量处理场景
    QList<ImageBuffer> batchImages;
    
    // 生成不同类型的测试图像
    SourceNode sourceNode;
    sourceNode.setGenerateTestPattern(true);
    
    QList<SourceNode::TestPatternType> patterns = {
        SourceNode::TestPatternType::ColorBars,
        SourceNode::TestPatternType::Gradient,
        SourceNode::TestPatternType::Checkerboard
    };
    
    for (auto pattern : patterns) {
        sourceNode.setTestPatternType(pattern);
        
        ImageBuffer input;
        ImageBuffer output;
        
        QVERIFY(sourceNode.process(input, output));
        batchImages.append(output);
    }
    
    // 配置批处理管道
    FormatConvertNode *convertNode = new FormatConvertNode(m_imageProcessor);
    convertNode->setTargetFormat(PixelFormat::Format3);
    
    ColorCorrectionNode *colorNode = new ColorCorrectionNode(m_imageProcessor);
    colorNode->setGamma(1.1);
    colorNode->setSaturation(10.0);
    
    m_imageProcessor->addNode(convertNode);
    m_imageProcessor->addNode(colorNode);
    
    // 执行批处理
    QSignalSpy progressSpy(m_imageProcessor, &AdvancedImageProcessor::processingProgress);
    
    QFuture<QList<ImageBuffer>> future = m_imageProcessor->processBatch(batchImages);
    future.waitForFinished();
    
    QList<ImageBuffer> results = future.result();
    
    QCOMPARE(results.size(), batchImages.size());
    QVERIFY(progressSpy.count() > 0); // 应该有进度更新
    
    // 验证批处理结果
    for (const auto &result : results) {
        QVERIFY(!result.isNull());
        QCOMPARE(result.format(), PixelFormat::Format3);
    }
    
    qDebug() << "批量图像处理集成测试完成";
}

// =============================================================================
// 测试辅助方法实现
// =============================================================================

void TestIntegration::setupVirtualTestEnvironment()
{
    qDebug() << "设置虚拟测试环境";
    
    // 设置测试用的环境变量
    qputenv("DEEPINSCAN_TEST_MODE", "1");
    qputenv("DEEPINSCAN_TEMP_DIR", m_tempDir->path().toUtf8());
    
    // 配置组件
    if (m_imageProcessor) {
        m_imageProcessor->setMaxMemoryUsage(100 * 1024 * 1024); // 100MB限制
    }
}

void TestIntegration::createMockScannerDevices()
{
    qDebug() << "创建模拟扫描仪设备";
    
    // 清理之前的测试设备
    qDeleteAll(m_testDevices);
    m_testDevices.clear();
    
    // 创建不同类型的虚拟设备
    QStringList deviceNames = {
        "虚拟Canon扫描仪",
        "虚拟Epson扫描仪",
        "虚拟HP扫描仪",
        "虚拟网络扫描仪"
    };
    
    QStringList deviceTypes = {
        "flatbed",
        "sheet-fed",
        "handheld",
        "network"
    };
    
    for (int i = 0; i < deviceNames.size(); i++) {
        DScannerDevice *device = new DScannerDevice();
        device->setId(QString("test_device_%1").arg(i));
        device->setName(deviceNames[i]);
        device->setType(deviceTypes[i % deviceTypes.size()]);
        device->setConnectionType(i < 3 ? DScannerDevice::ConnectionType::USB : DScannerDevice::ConnectionType::Network);
        
        if (device->connectionType() == DScannerDevice::ConnectionType::Network) {
            device->setNetworkAddress(QString("192.168.1.%1").arg(100 + i));
            device->setPort(8080 + i);
        }
        
        m_testDevices.append(device);
    }
    
    qDebug() << "创建了" << m_testDevices.size() << "个虚拟设备";
}

void TestIntegration::simulateNetworkDevices()
{
    // 模拟网络设备发现
    // 实际实现中可以启动模拟的网络服务
}

bool TestIntegration::waitForSignal(QObject *sender, const char *signal, int timeout)
{
    QSignalSpy spy(sender, signal);
    
    if (spy.count() > 0) {
        return true;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeout) {
        QCoreApplication::processEvents();
        if (spy.count() > 0) {
            return true;
        }
        QThread::msleep(10);
    }
    
    return false;
}

void TestIntegration::verifySystemState()
{
    // 验证系统各组件状态一致性
    if (m_manager) {
        QVERIFY(m_manager->isInitialized());
    }
    
    if (m_imageProcessor) {
        QVERIFY(m_imageProcessor->getCurrentMemoryUsage() >= 0);
    }
}

// 性能基准测试实现
void TestIntegration::benchmarkFullScanWorkflow()
{
    qDebug() << "性能基准测试：完整扫描工作流";
    
    QElapsedTimer timer;
    timer.start();
    
    // 模拟完整扫描流程
    createMockScannerDevices();
    
    for (auto device : m_testDevices) {
        m_manager->addDevice(device);
    }
    
    // 图像处理性能测试
    SourceNode *sourceNode = new SourceNode(m_imageProcessor);
    sourceNode->setGenerateTestPattern(true);
    
    ColorCorrectionNode *colorNode = new ColorCorrectionNode(m_imageProcessor);
    NoiseReductionNode *noiseNode = new NoiseReductionNode(m_imageProcessor);
    
    m_imageProcessor->addNode(sourceNode);
    m_imageProcessor->addNode(colorNode);
    m_imageProcessor->addNode(noiseNode);
    
    ImageBuffer input;
    ImageBuffer output;
    
    QElapsedTimer processingTimer;
    processingTimer.start();
    
    bool success = m_imageProcessor->processImage(input, output);
    QVERIFY(success);
    
    qint64 processingTime = processingTimer.elapsed();
    qint64 totalTime = timer.elapsed();
    
    qDebug() << "完整工作流基准测试结果:";
    qDebug() << "  图像处理时间:" << processingTime << "ms";
    qDebug() << "  总时间:" << totalTime << "ms";
    qDebug() << "  设备管理时间:" << (totalTime - processingTime) << "ms";
    
    // 性能断言
    QVERIFY(processingTime < 5000); // 图像处理应在5秒内完成
    QVERIFY(totalTime < 10000);     // 总流程应在10秒内完成
}

// 导出测试类的元对象信息
QTEST_MAIN(TestIntegration)
#include "test_integration.moc" 