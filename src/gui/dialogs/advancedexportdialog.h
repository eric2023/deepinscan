// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ADVANCEDEXPORTDIALOG_H
#define ADVANCEDEXPORTDIALOG_H

#include <DDialog>
#include <QImage>
#include <QStringList>
#include "Scanner/DScannerGlobal.h"

DWIDGET_USE_NAMESPACE
DSCANNER_USE_NAMESPACE

namespace Dtk {
namespace Scanner {

class AdvancedExportDialog : public DDialog
{
    Q_OBJECT

public:
    explicit AdvancedExportDialog(QWidget *parent = nullptr);
    ~AdvancedExportDialog();

    void setImages(const QList<QImage> &images);

signals:
    void exportStarted();
    void exportProgress(int current, int total);
    void exportFinished(bool success, const QStringList &filePaths);
    void exportError(const QString &error);

private:
    void setupUI();
};

} // namespace Scanner
} // namespace Dtk

#endif // ADVANCEDEXPORTDIALOG_H 