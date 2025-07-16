// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerGlobal.h"

DSCANNER_BEGIN_NAMESPACE

// 全局函数实现

QString dscannerVersion()
{
    return QStringLiteral("1.0.0");
}

QString dscannerBuildTime()
{
    return QStringLiteral(__DATE__ " " __TIME__);
}

DSCANNER_END_NAMESPACE 