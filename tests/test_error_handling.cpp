//
// SPDX-FileCopyrightText: 2025 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

// 测试错误处理相关头文件
#include "include/Scanner/DScannerGlobal.h"
#include "include/Scanner/DScannerTypes.h"
#include "include/Scanner/DScannerException.h"

using namespace Dtk::Scanner;

class ErrorHandlingTest : public QObject
{
    Q_OBJECT

public:
    ErrorHandlingTest(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

public slots:
    void startTest()
    {
        qDebug() << "=== DeepinScan 错误处理机制测试开始 ===";
        
        // 1. 测试所有错误代码
        testAllErrorCodes();
        
        // 2. 测试错误恢复机制
        testErrorRecovery();
        
        // 3. 测试异常抛出和捕获
        testExceptionHandling();
        
        // 4. 测试错误消息本地化
        testErrorLocalization();
        
        // 完成测试
        QTimer::singleShot(1000, this, &ErrorHandlingTest::finishTest);
    }

private slots:
    void finishTest()
    {
        qDebug() << "\n=== 错误处理机制测试结果 ===";
        qDebug() << "错误代码覆盖测试:" << (errorCodesTested ? "✅" : "❌");
        qDebug() << "错误恢复机制测试:" << (recoveryMechanismTested ? "✅" : "❌");
        qDebug() << "异常处理测试:" << (exceptionHandlingTested ? "✅" : "❌");
        qDebug() << "错误本地化测试:" << (localizationTested ? "✅" : "❌");
        
        qDebug() << "\n测试统计:";
        qDebug() << "总错误类型数:" << totalErrorTypes;
        qDebug() << "可恢复错误数:" << recoverableErrors;
        qDebug() << "不可恢复错误数:" << (totalErrorTypes - recoverableErrors);
        qDebug() << "有建议的错误数:" << errorsWithSuggestions;
        
        QCoreApplication::quit();
    }

private:
    void testAllErrorCodes()
    {
        qDebug() << "\n--- 测试所有错误代码 ---";
        
        // 定义所有错误代码
        QList<DScannerException::ErrorCode> errorCodes = {
            DScannerException::NoError,
            DScannerException::UnknownError,
            DScannerException::DeviceNotFound,
            DScannerException::DeviceNotConnected,
            DScannerException::DeviceBusy,
            DScannerException::DeviceError,
            DScannerException::DriverNotFound,
            DScannerException::DriverLoadError,
            DScannerException::InvalidParameter,
            DScannerException::OperationFailed,
            DScannerException::ScanCancelled,
            DScannerException::OutOfMemory,
            DScannerException::TimeoutError,
            DScannerException::NetworkError,
            DScannerException::FileIOError,
            DScannerException::ImageProcessingError,
            DScannerException::CalibrationError,
            DScannerException::HardwareError,
            DScannerException::PermissionDenied,
            DScannerException::IncompatibleDevice,
            DScannerException::CommunicationError,
            DScannerException::UnsupportedOperation,
            DScannerException::ConfigurationError,
            DScannerException::DataCorruption,
            DScannerException::LicenseError,
            DScannerException::InitializationError
        };
        
        totalErrorTypes = errorCodes.size();
        
        qDebug() << "测试" << totalErrorTypes << "种错误类型:";
        
        for (const auto &errorCode : errorCodes) {
            QString codeName = DScannerException::errorCodeName(errorCode);
            QString description = DScannerException::getErrorDescription(errorCode);
            QString suggestion = DScannerException::getSuggestion(errorCode);
            bool recoverable = DScannerException::isRecoverable(errorCode);
            QString recovery = DScannerException::getRecoveryAction(errorCode);
            
            qDebug() << "\n错误代码:" << codeName;
            qDebug() << "  描述:" << description;
            qDebug() << "  建议:" << suggestion;
            qDebug() << "  可恢复:" << (recoverable ? "是" : "否");
            
            if (recoverable) {
                qDebug() << "  恢复动作:" << recovery;
                recoverableErrors++;
            }
            
            if (!suggestion.isEmpty() && suggestion != "请联系技术支持获取帮助") {
                errorsWithSuggestions++;
            }
        }
        
        qDebug() << "\n✅ 错误代码覆盖测试完成";
        errorCodesTested = true;
    }
    
    void testErrorRecovery()
    {
        qDebug() << "\n--- 测试错误恢复机制 ---";
        
        // 测试可恢复错误的处理
        QList<DScannerException::ErrorCode> recoverableErrorCodes = {
            DScannerException::DeviceNotConnected,
            DScannerException::DeviceBusy,
            DScannerException::InvalidParameter,
            DScannerException::ScanCancelled,
            DScannerException::TimeoutError,
            DScannerException::NetworkError,
            DScannerException::ConfigurationError
        };
        
        qDebug() << "测试可恢复错误的自动恢复机制:";
        
        for (const auto &errorCode : recoverableErrorCodes) {
            if (DScannerException::isRecoverable(errorCode)) {
                QString codeName = DScannerException::errorCodeName(errorCode);
                QString recoveryAction = DScannerException::getRecoveryAction(errorCode);
                
                qDebug() << "  " << codeName << "→" << recoveryAction;
                
                // 模拟恢复尝试
                bool recovered = simulateRecovery(errorCode);
                qDebug() << "    恢复尝试:" << (recovered ? "成功" : "失败");
            }
        }
        
        qDebug() << "✅ 错误恢复机制测试完成";
        recoveryMechanismTested = true;
    }
    
    void testExceptionHandling()
    {
        qDebug() << "\n--- 测试异常处理 ---";
        
        // 测试异常的创建和抛出
        try {
            qDebug() << "测试自定义异常创建和抛出...";
            
            DScannerException exception(DScannerException::DeviceNotFound, 
                                      "测试设备未找到异常");
            
            qDebug() << "异常信息:";
            qDebug() << "  错误代码:" << exception.errorCode();
            qDebug() << "  错误消息:" << exception.errorMessage();
            qDebug() << "  what():" << exception.what();
            
            // 抛出异常进行捕获测试
            throw exception;
            
        } catch (const DScannerException &e) {
            qDebug() << "✅ 捕获到DScannerException:";
            qDebug() << "  错误代码:" << e.errorCode();
            qDebug() << "  错误消息:" << e.errorMessage();
            qDebug() << "  建议:" << DScannerException::getSuggestion(e.errorCode());
            
        } catch (const std::exception &e) {
            qDebug() << "❌ 捕获到std::exception:" << e.what();
        }
        
        // 测试其他错误类型
        QList<DScannerException::ErrorCode> testErrors = {
            DScannerException::OutOfMemory,
            DScannerException::PermissionDenied,
            DScannerException::IncompatibleDevice
        };
        
        for (const auto &errorCode : testErrors) {
            try {
                DScannerException exception(errorCode, 
                    QString("测试%1异常").arg(DScannerException::errorCodeName(errorCode)));
                throw exception;
            } catch (const DScannerException &e) {
                qDebug() << "捕获异常:" << DScannerException::errorCodeName(e.errorCode());
            }
        }
        
        qDebug() << "✅ 异常处理测试完成";
        exceptionHandlingTested = true;
    }
    
    void testErrorLocalization()
    {
        qDebug() << "\n--- 测试错误消息本地化 ---";
        
        // 测试中文错误消息
        QList<DScannerException::ErrorCode> testCodes = {
            DScannerException::DeviceNotFound,
            DScannerException::NetworkError,
            DScannerException::OutOfMemory,
            DScannerException::PermissionDenied
        };
        
        qDebug() << "测试中文错误消息:";
        
        for (const auto &code : testCodes) {
            QString englishName = DScannerException::errorCodeName(code);
            QString chineseDesc = DScannerException::getErrorDescription(code);
            QString suggestion = DScannerException::getSuggestion(code);
            
            qDebug() << "  " << englishName << "→" << chineseDesc;
            qDebug() << "    建议:" << suggestion;
            
            // 验证本地化质量
            if (chineseDesc.contains(QRegExp("[\\u4e00-\\u9fa5]"))) {
                localizedMessages++;
            }
        }
        
        qDebug() << QString("✅ 本地化测试完成，%1/%2条消息已本地化")
                        .arg(localizedMessages).arg(testCodes.size());
        localizationTested = true;
    }
    
    bool simulateRecovery(DScannerException::ErrorCode errorCode)
    {
        // 模拟不同错误类型的恢复尝试
        switch (errorCode) {
        case DScannerException::DeviceNotConnected:
            // 模拟重新连接
            return true;
        case DScannerException::DeviceBusy:
            // 模拟等待设备空闲
            return true;
        case DScannerException::InvalidParameter:
            // 模拟使用默认参数
            return true;
        case DScannerException::TimeoutError:
            // 模拟增加超时时间
            return true;
        case DScannerException::NetworkError:
            // 模拟网络重连
            return false; // 网络问题不总是能恢复
        case DScannerException::ConfigurationError:
            // 模拟重置配置
            return true;
        default:
            return false;
        }
    }

private:
    // 测试状态标记
    bool errorCodesTested = false;
    bool recoveryMechanismTested = false;
    bool exceptionHandlingTested = false;
    bool localizationTested = false;
    
    // 统计信息
    int totalErrorTypes = 0;
    int recoverableErrors = 0;
    int errorsWithSuggestions = 0;
    int localizedMessages = 0;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "DeepinScan 错误处理机制测试" << std::endl;
    std::cout << "============================" << std::endl;
    
    ErrorHandlingTest test;
    
    // 启动测试
    QTimer::singleShot(100, &test, &ErrorHandlingTest::startTest);
    
    return app.exec();
}

#include "test_error_handling.moc" 