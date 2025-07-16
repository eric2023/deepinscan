// SPDX-FileCopyrightText: 2024 DeepinScan Team
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Scanner/DScannerNetworkDiscovery.h"
#include "dscannernetworkdiscovery_p.h"

#include <QDebug>
#include <QDnsServiceRecord>
#include <QDnsTextRecord>
#include <QTimer>

DSCANNER_USE_NAMESPACE

// DScannerMdnsDiscoveryPrivate 实现

DScannerMdnsDiscoveryPrivate::DScannerMdnsDiscoveryPrivate(DScannerMdnsDiscovery *q)
    : q_ptr(q)
    , isDiscovering(false)
    , discoveryTimer(new QTimer(q))
{
    // 初始化统计信息
    stats.totalServicesFound = 0;
    stats.activeQueries = 0;
    
    // 设置发现定时器
    discoveryTimer->setInterval(60000); // 60秒间隔
    QObject::connect(discoveryTimer, &QTimer::timeout, q, [this]() {
        startServiceDiscovery();
    });
}

DScannerMdnsDiscoveryPrivate::~DScannerMdnsDiscoveryPrivate()
{
    cleanup();
}

void DScannerMdnsDiscoveryPrivate::init()
{
    qCDebug(dscannerNetwork) << "Initializing mDNS discovery";
}

void DScannerMdnsDiscoveryPrivate::cleanup()
{
    qCDebug(dscannerNetwork) << "Cleaning up mDNS discovery";
    
    stopServiceDiscovery();
    
    // 清理所有活动的DNS查询
    for (QDnsLookup *lookup : activeLookups) {
        lookup->abort();
        lookup->deleteLater();
    }
    activeLookups.clear();
    serviceLookups.clear();
    
    // 清理服务数据
    services.clear();
}

void DScannerMdnsDiscoveryPrivate::startServiceDiscovery()
{
    qCDebug(dscannerNetwork) << "Starting mDNS service discovery";
    
    stats.lastQueryTime = QDateTime::currentDateTime();
    
    // 查询所有支持的服务类型
    for (const QString &serviceType : MDNS::SERVICE_TYPES) {
        lookupService(serviceType);
    }
}

void DScannerMdnsDiscoveryPrivate::stopServiceDiscovery()
{
    qCDebug(dscannerNetwork) << "Stopping mDNS service discovery";
    
    discoveryTimer->stop();
    
    // 停止所有活动查询
    for (QDnsLookup *lookup : activeLookups) {
        lookup->abort();
    }
}

void DScannerMdnsDiscoveryPrivate::lookupService(const QString &serviceType)
{
    qCDebug(dscannerNetwork) << "Looking up service:" << serviceType;
    
    // 如果已经有这个服务类型的查询在进行，跳过
    if (serviceLookups.contains(serviceType)) {
        return;
    }
    
    QDnsLookup *lookup = new QDnsLookup(QDnsLookup::PTR, serviceType + ".local", q_ptr);
    
    QObject::connect(lookup, &QDnsLookup::finished, q_ptr, [this, lookup, serviceType]() {
        stats.activeQueries--;
        
        if (lookup->error() == QDnsLookup::NoError) {
            qCDebug(dscannerNetwork) << "Service lookup successful for:" << serviceType;
            
            // 处理服务记录
            const QList<QDnsServiceRecord> serviceRecords = lookup->serviceRecords();
            for (const QDnsServiceRecord &record : serviceRecords) {
                processServiceRecord(record);
            }
            
            // 处理主机记录
            const QList<QDnsHostAddressRecord> hostRecords = lookup->hostAddressRecords();
            for (const QDnsHostAddressRecord &record : hostRecords) {
                processHostRecord(record, serviceType);
            }
        } else {
            qCWarning(dscannerNetwork) << "Service lookup failed for:" << serviceType << lookup->errorString();
        }
        
        // 清理查询
        activeLookups.removeAll(lookup);
        serviceLookups.remove(serviceType);
        lookup->deleteLater();
    });
    
    activeLookups.append(lookup);
    serviceLookups.insert(serviceType, lookup);
    stats.activeQueries++;
    
    lookup->lookup();
}

void DScannerMdnsDiscoveryPrivate::processServiceRecord(const QDnsServiceRecord &record)
{
    qCDebug(dscannerNetwork) << "Processing service record:" << record.name() << record.target() << record.port();
    
    // 查询主机地址
    QDnsLookup *hostLookup = new QDnsLookup(QDnsLookup::A, record.target(), q_ptr);
    
    QObject::connect(hostLookup, &QDnsLookup::finished, q_ptr, [this, hostLookup, record]() {
        if (hostLookup->error() == QDnsLookup::NoError) {
            const QList<QDnsHostAddressRecord> hostRecords = hostLookup->hostAddressRecords();
            for (const QDnsHostAddressRecord &hostRecord : hostRecords) {
                // 提取服务类型
                QString serviceType = record.name();
                QRegularExpression re(R"(\.(_[^.]+\._tcp)\.local)");
                QRegularExpressionMatch match = re.match(serviceType);
                if (match.hasMatch()) {
                    serviceType = match.captured(1);
                }
                
                addService(record.name(), serviceType, hostRecord.value(), record.port());
            }
        } else {
            qCWarning(dscannerNetwork) << "Host lookup failed for:" << record.target() << hostLookup->errorString();
        }
        
        hostLookup->deleteLater();
    });
    
    hostLookup->lookup();
}

void DScannerMdnsDiscoveryPrivate::processHostRecord(const QDnsHostAddressRecord &record, const QString &serviceName)
{
    qCDebug(dscannerNetwork) << "Processing host record:" << record.name() << record.value();
    
    // 这里可以处理额外的主机记录信息
    Q_UNUSED(serviceName)
}

void DScannerMdnsDiscoveryPrivate::addService(const QString &serviceName, const QString &serviceType, 
                                            const QHostAddress &address, quint16 port)
{
    qCDebug(dscannerNetwork) << "Adding service:" << serviceName << serviceType << address << port;
    
    QString serviceKey = QStringLiteral("%1_%2").arg(serviceName, serviceType);
    
    ServiceInfo info;
    info.name = serviceName;
    info.type = serviceType;
    info.address = address;
    info.port = port;
    info.lastSeen = QDateTime::currentDateTime();
    
    bool isNew = !services.contains(serviceKey);
    services.insert(serviceKey, info);
    
    if (isNew) {
        stats.totalServicesFound++;
        emit q_ptr->serviceDiscovered(serviceName, serviceType, address, port);
    }
}

void DScannerMdnsDiscoveryPrivate::removeService(const QString &serviceName, const QString &serviceType)
{
    qCDebug(dscannerNetwork) << "Removing service:" << serviceName << serviceType;
    
    QString serviceKey = QStringLiteral("%1_%2").arg(serviceName, serviceType);
    
    if (services.remove(serviceKey) > 0) {
        emit q_ptr->serviceRemoved(serviceName, serviceType);
    }
}

// DScannerMdnsDiscovery 实现

DScannerMdnsDiscovery::DScannerMdnsDiscovery(QObject *parent)
    : QObject(parent)
    , d_ptr(new DScannerMdnsDiscoveryPrivate(this))
{
    Q_D(DScannerMdnsDiscovery);
    d->init();
}

DScannerMdnsDiscovery::~DScannerMdnsDiscovery()
{
    Q_D(DScannerMdnsDiscovery);
    d->cleanup();
}

void DScannerMdnsDiscovery::startDiscovery()
{
    Q_D(DScannerMdnsDiscovery);
    
    if (d->isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Starting mDNS discovery";
    
    d->isDiscovering = true;
    d->discoveryTimer->start();
    
    // 立即开始发现
    d->startServiceDiscovery();
}

void DScannerMdnsDiscovery::stopDiscovery()
{
    Q_D(DScannerMdnsDiscovery);
    
    if (!d->isDiscovering) {
        return;
    }
    
    qCDebug(dscannerNetwork) << "Stopping mDNS discovery";
    
    d->isDiscovering = false;
    d->stopServiceDiscovery();
}

QStringList DScannerMdnsDiscovery::supportedServiceTypes()
{
    return MDNS::SERVICE_TYPES;
}

void DScannerMdnsDiscovery::onServiceDiscovered()
{
    qCDebug(dscannerNetwork) << "Service discovered signal received";
}

void DScannerMdnsDiscovery::onServiceRemoved()
{
    qCDebug(dscannerNetwork) << "Service removed signal received";
}

 