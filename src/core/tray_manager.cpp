#include "tray_manager.h"
#include "logger.h"
#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QPixmap>

namespace ccdesk::core {

TrayManager::TrayManager(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_showAction(nullptr)
    , m_organizeAction(nullptr)
    , m_settingsAction(nullptr)
    , m_exitAction(nullptr)
{
    Logger::getInstance().info("TrayManager: Constructed");
}

TrayManager::~TrayManager() {
    Logger::getInstance().info("TrayManager: Destroyed");
}

bool TrayManager::initialize() {
    Logger::getInstance().info("TrayManager: Initializing system tray...");
    
    // 检查系统托盘支持
    if (!isSystemTrayAvailable()) {
        Logger::getInstance().warning("TrayManager: System tray not available on this platform");
        return false;
    }
    
    // 创建托盘图标
    m_trayIcon = std::make_unique<QSystemTrayIcon>(m_mainWindow);
    
    // 设置托盘图标（简单的蓝色方块图标）
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::blue);
    m_trayIcon->setIcon(QIcon(pixmap));
    
    // 创建托盘菜单
    createTrayMenu();
    m_trayIcon->setContextMenu(m_trayMenu.get());
    
    // 连接托盘图标激活信号
    connect(m_trayIcon.get(), QOverload<QSystemTrayIcon::ActivationReason>::of(&QSystemTrayIcon::activated),
            this, &TrayManager::onTrayIconActivated);
    
    // 显示托盘图标
    m_trayIcon->show();
    
    Logger::getInstance().info("TrayManager: System tray initialized successfully");
    return true;
}

bool TrayManager::isSystemTrayAvailable() const {
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayManager::show() {
    if (m_trayIcon) {
        m_trayIcon->show();
    }
}

void TrayManager::hide() {
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

void TrayManager::createTrayMenu() {
    m_trayMenu = std::make_unique<QMenu>();
    
    // 显示主窗口
    m_showAction = m_trayMenu->addAction("Show Window");
    connect(m_showAction, &QAction::triggered, this, &TrayManager::onShowMainWindow);
    
    m_trayMenu->addSeparator();
    
    // 一键整理
    m_organizeAction = m_trayMenu->addAction("One-Click Organize");
    connect(m_organizeAction, &QAction::triggered, this, &TrayManager::onOneClickOrganize);
    
    // 设置
    m_settingsAction = m_trayMenu->addAction("Settings");
    connect(m_settingsAction, &QAction::triggered, this, &TrayManager::onShowSettings);
    
    m_trayMenu->addSeparator();
    
    // 退出程序
    m_exitAction = m_trayMenu->addAction("Exit");
    connect(m_exitAction, &QAction::triggered, this, &TrayManager::onExitApp);
    
    Logger::getInstance().info("TrayManager: Tray menu created");
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    Logger::getInstance().info("TrayManager: Tray icon activated, reason: " + std::to_string(reason));
    
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        // 点击托盘图标时恢复窗口
        if (m_mainWindow) {
            m_mainWindow->showNormal();
            m_mainWindow->activateWindow();
            m_mainWindow->raise();
            Logger::getInstance().info("TrayManager: Main window restored from tray");
        }
    }
}

void TrayManager::onShowMainWindow() {
    Logger::getInstance().info("TrayManager: 'Show Window' menu item clicked");
    
    if (m_mainWindow) {
        m_mainWindow->showNormal();
        m_mainWindow->activateWindow();
        m_mainWindow->raise();
        Logger::getInstance().info("TrayManager: Main window restored to screen");
    }
}

void TrayManager::onOneClickOrganize() {
    Logger::getInstance().info("TrayManager: 'One-Click Organize' menu item clicked");
    
    if (m_mainWindow) {
        // 发射信号给主窗口进行一键整理
        QMetaObject::invokeMethod(m_mainWindow, "triggerOrganizeFromTray", Qt::QueuedConnection);
    }
}

void TrayManager::onShowSettings() {
    Logger::getInstance().info("TrayManager: 'Settings' menu item clicked");
    
    if (m_mainWindow) {
        // 发射信号给主窗口打开设置
        QMetaObject::invokeMethod(m_mainWindow, "showSettingsDialog", Qt::QueuedConnection);
    }
}

void TrayManager::onExitApp() {
    Logger::getInstance().info("TrayManager: 'Exit' menu item clicked, shutting down application");
    
    // 退出应用程序
    QApplication::quit();
}

} // namespace ccdesk::core
