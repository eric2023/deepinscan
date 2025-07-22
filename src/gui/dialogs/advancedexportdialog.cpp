// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "advancedexportdialog.h"
#include <QDebug>

namespace Dtk {
namespace Scanner {

AdvancedExportDialog::AdvancedExportDialog(QWidget *parent)
    : DDialog(parent)
{
    qDebug() << "AdvancedExportDialog::AdvancedExportDialog - 存根实现";
    setupUI();
}

AdvancedExportDialog::~AdvancedExportDialog()
{
    qDebug() << "AdvancedExportDialog::~AdvancedExportDialog - 存根实现";
}

// Qt MOC系统会自动生成qt_metacall, staticMetaObject等，不需要手动实现

void AdvancedExportDialog::setImages(const QList<QImage> &images)
{
    Q_UNUSED(images)
    qDebug() << "AdvancedExportDialog::setImages - 存根实现";
}

void AdvancedExportDialog::setupUI()
{
    qDebug() << "AdvancedExportDialog::setupUI - 存根实现";
    setWindowTitle("高级导出");
    resize(800, 600);
}

// Qt MOC系统会自动生成所有信号函数，不需要手动实现

} // namespace Scanner
} // namespace Dtk 