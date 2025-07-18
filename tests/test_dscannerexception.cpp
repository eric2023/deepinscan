// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QtTest>
#include <Scanner/DScannerException.h>

using namespace Dtk::Scanner;

class TestDScannerException : public QObject
{
    Q_OBJECT

private slots:
    void testErrorCodeConstructor();
    void testMessageConstructor();
    void testCopyConstructor();
    void testAssignmentOperator();
    void testErrorCodeName();
    void testDefaultErrorMessage();
    void testWhat();
    void testCloneAndRaise();
};

void TestDScannerException::testErrorCodeConstructor()
{
    DScannerException exception(DScannerException::ErrorCode::DeviceNotFound, "Custom message");
    
    QCOMPARE(exception.errorCode(), DScannerException::ErrorCode::DeviceNotFound);
    QCOMPARE(exception.errorMessage(), QString("Custom message"));
}

void TestDScannerException::testMessageConstructor()
{
    DScannerException exception("Test message");
    
    QCOMPARE(exception.errorCode(), DScannerException::ErrorCode::Unknown);
    QCOMPARE(exception.errorMessage(), QString("Test message"));
}

void TestDScannerException::testCopyConstructor()
{
    DScannerException original(DScannerException::ErrorCode::ScanFailed, "Original message");
    DScannerException copy(original);
    
    QCOMPARE(copy.errorCode(), original.errorCode());
    QCOMPARE(copy.errorMessage(), original.errorMessage());
}

void TestDScannerException::testAssignmentOperator()
{
    DScannerException original(DScannerException::ErrorCode::InvalidParameter, "Original message");
    DScannerException assigned("Different message");
    
    assigned = original;
    
    QCOMPARE(assigned.errorCode(), original.errorCode());
    QCOMPARE(assigned.errorMessage(), original.errorMessage());
}

void TestDScannerException::testErrorCodeName()
{
    QCOMPARE(DScannerException::errorCodeName(DScannerException::ErrorCode::DeviceNotFound), 
             QString("DeviceNotFound"));
    QCOMPARE(DScannerException::errorCodeName(DScannerException::ErrorCode::ScanFailed), 
             QString("ScanFailed"));
    QCOMPARE(DScannerException::errorCodeName(DScannerException::ErrorCode::Unknown), 
             QString("Unknown"));
}

void TestDScannerException::testDefaultErrorMessage()
{
    QString message = DScannerException::defaultErrorMessage(DScannerException::ErrorCode::DeviceNotFound);
    QVERIFY(!message.isEmpty());
    QVERIFY(message.contains("device", Qt::CaseInsensitive));
}

void TestDScannerException::testWhat()
{
    DScannerException exception(DScannerException::ErrorCode::DeviceNotFound, "Test message");
    
    QString whatStr = QString::fromUtf8(exception.what());
    QVERIFY(whatStr.contains("DeviceNotFound"));
    QVERIFY(whatStr.contains("Test message"));
}

void TestDScannerException::testCloneAndRaise()
{
    DScannerException original(DScannerException::ErrorCode::ScanFailed, "Test message");
    
    // Test clone
    std::unique_ptr<DScannerException> cloned(original.clone());
    QCOMPARE(cloned->errorCode(), original.errorCode());
    QCOMPARE(cloned->errorMessage(), original.errorMessage());
    
    // Test raise
    bool exceptionCaught = false;
    try {
        original.raise();
    } catch (const DScannerException &e) {
        exceptionCaught = true;
        QCOMPARE(e.errorCode(), original.errorCode());
        QCOMPARE(e.errorMessage(), original.errorMessage());
    }
    QVERIFY(exceptionCaught);
}

QTEST_MAIN(TestDScannerException)
#include "test_dscannerexception.moc"