// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSCANNERGLOBAL_H
#define DSCANNERGLOBAL_H

#include <QtCore/qglobal.h>

#define DSCANNER_NAMESPACE Dtk::Scanner

#define DSCANNER_BEGIN_NAMESPACE namespace Dtk { namespace Scanner {
#define DSCANNER_END_NAMESPACE }}
#define DSCANNER_USE_NAMESPACE using namespace DSCANNER_NAMESPACE;

#if defined(DSCANNER_LIBRARY)
#  define DSCANNER_EXPORT Q_DECL_EXPORT
#else
#  define DSCANNER_EXPORT Q_DECL_IMPORT
#endif

// Version information
#define DSCANNER_VERSION_MAJOR 1
#define DSCANNER_VERSION_MINOR 0
#define DSCANNER_VERSION_PATCH 0

#define DSCANNER_VERSION_STR "1.0.0"

// Logging categories
#include <QLoggingCategory>

DSCANNER_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(scannerCore)
Q_DECLARE_LOGGING_CATEGORY(scannerDevice)
Q_DECLARE_LOGGING_CATEGORY(scannerDriver)
Q_DECLARE_LOGGING_CATEGORY(scannerComm)
Q_DECLARE_LOGGING_CATEGORY(scannerImage)
Q_DECLARE_LOGGING_CATEGORY(scannerGui)

DSCANNER_END_NAMESPACE

#endif // DSCANNERGLOBAL_H 