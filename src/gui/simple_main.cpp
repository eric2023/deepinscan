// SPDX-FileCopyrightText: 2024-2025 eric2023
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file simple_main.cpp
 * @brief 最小化扫描仪GUI应用主程序
 * 
 * 使用标准Qt控件创建一个简单的扫描仪应用界面，验证核心库功能
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QMessageBox>
#include <QTimer>
#include <QStatusBar>

#include "Scanner/DScannerManager.h"
#include "Scanner/DScannerDevice.h"
#include "Scanner/DScannerException.h"

using namespace Dtk::Scanner;

class SimpleMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SimpleMainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_manager(DScannerManager::instance())
    {
        setWindowTitle("DeepinScan - 扫描仪管理工具");
        setMinimumSize(800, 600);
        setupUI();
        connectSignals();
        
        // 显示应用信息
        statusBar()->showMessage("DeepinScan v1.0.0 - 就绪");
        
        // 启动时自动搜索设备
        QTimer::singleShot(1000, this, &SimpleMainWindow::refreshDevices);
    }

private slots:
    void refreshDevices()
    {
        try {
            m_logText->append("开始搜索扫描仪设备...");
            m_deviceList->clear();
            
            // 使用管理器搜索设备
            QList<DeviceInfo> devices = m_manager->discoverDevices();
            
            if (devices.isEmpty()) {
                m_logText->append("未发现任何扫描仪设备");
                m_deviceList->addItem("(未发现设备)");
            } else {
                m_logText->append(QString("发现 %1 个设备:").arg(devices.size()));
                for (const DeviceInfo &device : devices) {
                    QString deviceText = QString("%1 (%2)").arg(device.name, device.manufacturer);
                    m_deviceList->addItem(deviceText);
                    m_logText->append(QString("  - %1").arg(deviceText));
                }
            }
            
            statusBar()->showMessage(QString("发现 %1 个设备").arg(devices.size()));
            
        } catch (const DScannerException &e) {
            QString error = QString("搜索设备失败: %1").arg(e.what());
            m_logText->append(error);
            QMessageBox::warning(this, "错误", error);
        }
    }

    void onDeviceSelected()
    {
        QListWidgetItem *item = m_deviceList->currentItem();
        if (!item || item->text() == "(未发现设备)") {
            return;
        }
        
        QString deviceName = item->text();
        m_logText->append(QString("选择设备: %1").arg(deviceName));
        statusBar()->showMessage(QString("已选择: %1").arg(deviceName));
    }

    void testScan()
    {
        QListWidgetItem *item = m_deviceList->currentItem();
        if (!item || item->text() == "(未发现设备)") {
            QMessageBox::information(this, "提示", "请先选择一个扫描仪设备");
            return;
        }
        
        QString deviceName = item->text();
        m_logText->append(QString("尝试测试扫描: %1").arg(deviceName));
        
        // 这里可以添加实际的扫描测试代码
        QMessageBox::information(this, "测试扫描", 
                                QString("扫描功能测试\n设备: %1\n\n注意: 实际扫描功能需要连接真实设备").arg(deviceName));
    }

    void showAbout()
    {
        QMessageBox::about(this, "关于 DeepinScan", 
                          "DeepinScan v1.0.0\n\n"
                          "现代化的Linux扫描仪驱动框架\n"
                          "支持SANE协议、USB/网络连接\n\n"
                          "构建信息:\n"
                          "- Qt5框架\n"
                          "- C++17标准\n"
                          "- 核心库编译成功");
    }

private:
    void setupUI()
    {
        auto *centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        
        auto *mainLayout = new QHBoxLayout(centralWidget);
        auto *splitter = new QSplitter(Qt::Horizontal);
        mainLayout->addWidget(splitter);
        
        // 左侧设备列表
        auto *leftWidget = new QWidget;
        auto *leftLayout = new QVBoxLayout(leftWidget);
        
        leftLayout->addWidget(new QLabel("扫描仪设备:"));
        m_deviceList = new QListWidget;
        leftLayout->addWidget(m_deviceList);
        
        auto *buttonLayout = new QHBoxLayout;
        m_refreshBtn = new QPushButton("刷新设备");
        m_testBtn = new QPushButton("测试扫描");
        buttonLayout->addWidget(m_refreshBtn);
        buttonLayout->addWidget(m_testBtn);
        leftLayout->addLayout(buttonLayout);
        
        splitter->addWidget(leftWidget);
        
        // 右侧日志区域
        auto *rightWidget = new QWidget;
        auto *rightLayout = new QVBoxLayout(rightWidget);
        
        rightLayout->addWidget(new QLabel("操作日志:"));
        m_logText = new QTextEdit;
        m_logText->setReadOnly(true);
        rightLayout->addWidget(m_logText);
        
        auto *aboutBtn = new QPushButton("关于");
        rightLayout->addWidget(aboutBtn);
        connect(aboutBtn, &QPushButton::clicked, this, &SimpleMainWindow::showAbout);
        
        splitter->addWidget(rightWidget);
        splitter->setSizes({300, 500});
    }

    void connectSignals()
    {
        connect(m_refreshBtn, &QPushButton::clicked, this, &SimpleMainWindow::refreshDevices);
        connect(m_testBtn, &QPushButton::clicked, this, &SimpleMainWindow::testScan);
        connect(m_deviceList, &QListWidget::currentItemChanged, this, &SimpleMainWindow::onDeviceSelected);
    }

private:
    DScannerManager *m_manager;
    QListWidget *m_deviceList;
    QTextEdit *m_logText;
    QPushButton *m_refreshBtn;
    QPushButton *m_testBtn;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DeepinScan");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Deepin");
    
    try {
        SimpleMainWindow window;
        window.show();
        
        return app.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "启动错误", 
                            QString("应用启动失败: %1").arg(e.what()));
        return 1;
    }
}

#include "simple_main.moc" 