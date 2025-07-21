// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class AboutDialog : public DDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private:
    void setupUI();
};

#endif // ABOUTDIALOG_H 