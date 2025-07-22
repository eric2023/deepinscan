// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsdialog.h"
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent)
    : DDialog(parent)
{
    qDebug() << "SettingsDialog::SettingsDialog - 存根实现";
    setupUI();
}

SettingsDialog::~SettingsDialog()
{
    qDebug() << "SettingsDialog::~SettingsDialog - 存根实现";
}

// Qt MOC系统会自动生成qt_metacall等函数，不需要手动实现

void SettingsDialog::setupUI()
{
    qDebug() << "SettingsDialog::setupUI - 存根实现";
    setWindowTitle("设置");
    resize(600, 400);
} 