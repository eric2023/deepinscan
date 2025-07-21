// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"
#include <DLog>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    // 设置应用程序信息
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // 创建应用程序实例
    Application app(argc, argv);

    // 设置日志系统
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(logDir);
    // 使用QLoggingCategory进行日志管理
    QLoggingCategory::setFilterRules("*.debug=true");

    qDebug() << "深度扫描应用程序启动";
    qDebug() << "版本:" << app.applicationVersion();
    qDebug() << "日志目录:" << logDir;

    // 初始化应用程序
    if (!app.initialize()) {
        qCritical() << "应用程序初始化失败";
        return -1;
    }

    // 显示主窗口
    app.showMainWindow();

    qDebug() << "应用程序进入事件循环";
    int result = app.exec();
    qDebug() << "应用程序退出，返回码:" << result;

    return result;
} 