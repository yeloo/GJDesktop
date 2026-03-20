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
    Logger::getInstance().info("TrayManager: 托盘管理器已构造");
}

TrayManager::~TrayManager() {
    Logger::getInstance().info("TrayManager: 托盘管理器已销毁");
}

bool TrayManager::initialize() {
    Logger::getInstance().info("TrayManager: 正在初始化系统托盘...");
    
    // 检查系统托盘支持
    if (!isSystemTrayAvailable()) {
        Logger::getInstance().warning("TrayManager: 当前平台不支持系统托盘");
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
    
    Logger::getInstance().info("TrayManager: 系统托盘初始化成功");
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
    m_showAction = m_trayMenu->addAction("显示窗口");
    connect(m_showAction, &QAction::triggered, this, &TrayManager::onShowMainWindow);
    
    m_trayMenu->addSeparator();
    
    // 生成整理规划
    m_organizeAction = m_trayMenu->addAction("生成整理规划");
    connect(m_organizeAction, &QAction::triggered, this, &TrayManager::onGenerateOrganizePlan);
    
    // 设置
    m_settingsAction = m_trayMenu->addAction("设置");
    connect(m_settingsAction, &QAction::triggered, this, &TrayManager::onShowSettings);
    
    m_trayMenu->addSeparator();
    
    // 退出程序
    m_exitAction = m_trayMenu->addAction("退出");
    connect(m_exitAction, &QAction::triggered, this, &TrayManager::onExitApp);
    
    Logger::getInstance().info("TrayManager: 托盘菜单创建成功");
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    Logger::getInstance().info("TrayManager: 托盘图标被激活，原因: " + std::to_string(reason));
    
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        // 点击托盘图标时恢复窗口
        if (m_mainWindow) {
            m_mainWindow->showNormal();
            m_mainWindow->activateWindow();
            m_mainWindow->raise();
            Logger::getInstance().info("TrayManager: 主窗口已从托盘恢复");
        }
    }
}

void TrayManager::onShowMainWindow() {
    Logger::getInstance().info("TrayManager: 点击了'显示窗口'菜单项");
    
    if (m_mainWindow) {
        m_mainWindow->showNormal();
        m_mainWindow->activateWindow();
        m_mainWindow->raise();
        Logger::getInstance().info("TrayManager: 主窗口已恢复到屏幕");
    }
}

void TrayManager::onGenerateOrganizePlan() {
    Logger::getInstance().info("TrayManager: 点击了'生成整理规划'菜单项");
    
    if (m_mainWindow) {
        // 发射信号给主窗口生成整理规划
        QMetaObject::invokeMethod(m_mainWindow, "triggerOrganizeFromTray", Qt::QueuedConnection);
    }
}

void TrayManager::onShowSettings() {
    Logger::getInstance().info("TrayManager: 点击了'设置'菜单项");
    
    if (m_mainWindow) {
        // 发射信号给主窗口打开设置
        QMetaObject::invokeMethod(m_mainWindow, "showSettingsDialog", Qt::QueuedConnection);
    }
}

void TrayManager::onExitApp() {
    Logger::getInstance().info("TrayManager: 点击了'退出'菜单项，正在关闭应用程序");
    
    // 退出应用程序
    QApplication::quit();
}

} // namespace ccdesk::core
