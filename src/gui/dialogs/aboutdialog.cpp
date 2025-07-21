// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aboutdialog.h"
#include <DLabel>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget *parent)
    : DDialog(parent)
{
    setWindowTitle(tr("关于 DeepinScan"));
    setModal(true);
    setupUI();
}

AboutDialog::~AboutDialog()
{
}

void AboutDialog::setupUI()
{
    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    
    auto *titleLabel = new DLabel(tr("DeepinScan v1.0.0"), this);
    auto *descLabel = new DLabel(tr("现代化深度系统扫描应用程序"), this);
    
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    
    setContentLayoutContentsMargins(QMargins(20, 20, 20, 20));
    addContent(content);
    
    addButton(tr("确定"), true);
} 