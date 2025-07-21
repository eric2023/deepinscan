// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerNetworkDiscovery_Simple.h"

DSCANNER_USE_NAMESPACE

// 桩实现文件：提供DScannerNetworkDiscoverySimple的信号定义
// 这是为了解决链接错误，当主实现文件无法使用时的临时解决方案

DScannerNetworkDiscoverySimple::DScannerNetworkDiscoverySimple(QObject *parent)
    : QObject(parent)
{
    // 桩构造函数
}

DScannerNetworkDiscoverySimple::~DScannerNetworkDiscoverySimple()
{
    // 桩析构函数
}

void DScannerNetworkDiscoverySimple::startDiscovery()
{
    // 桩实现：开始发现
    emit discoveryFinished();
}

void DScannerNetworkDiscoverySimple::stopDiscovery()
{
    // 桩实现：停止发现
    emit discoveryFinished();
}

bool DScannerNetworkDiscoverySimple::isDiscovering() const
{
    // 桩实现：返回发现状态
    return false;
}

QList<NetworkDeviceInfo> DScannerNetworkDiscoverySimple::discoveredDevices() const
{
    // 桩实现：返回空的设备列表
    return QList<NetworkDeviceInfo>();
}

void DScannerNetworkDiscoverySimple::setDiscoveryInterval(int seconds)
{
    // 桩实现：设置发现间隔
    Q_UNUSED(seconds)
}

void DScannerNetworkDiscoverySimple::performDiscovery()
{
    // 桩实现：执行发现
    emit discoveryFinished();
}

// 包含moc文件以生成信号的实现
#include "dscannernetworkdiscovery_simple_stubs.moc" 