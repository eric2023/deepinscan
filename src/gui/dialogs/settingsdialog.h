// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class SettingsDialog : public DDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private:
    void setupUI();
};

#endif // SETTINGSDIALOG_H 