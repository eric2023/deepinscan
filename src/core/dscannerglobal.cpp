// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerGlobal.h"

DSCANNER_BEGIN_NAMESPACE

// 日志类别定义 (与private头文件中的声明保持一致)
Q_LOGGING_CATEGORY(dscannerCore, "deepinscan.core")
Q_LOGGING_CATEGORY(scannerDevice, "deepinscan.device")
Q_LOGGING_CATEGORY(scannerDriver, "deepinscan.driver")
Q_LOGGING_CATEGORY(scannerComm, "deepinscan.comm")
Q_LOGGING_CATEGORY(scannerImage, "deepinscan.image")
Q_LOGGING_CATEGORY(scannerGui, "deepinscan.gui")

// 全局函数实现

QString dscannerVersion()
{
    return QStringLiteral("1.0.0");
}

QString dscannerBuildTime()
{
    return QStringLiteral(__DATE__ " " __TIME__);
}

QString dscannerCoreVersion()
{
    return QStringLiteral("DeepinScan Core v1.0.0");
}

DSCANNER_END_NAMESPACE 