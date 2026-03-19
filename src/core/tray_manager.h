#ifndef TRAY_MANAGER_H
#define TRAY_MANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <memory>

class QMenu;
class QAction;
class QMainWindow;

namespace ccdesk::core {

class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(QMainWindow* mainWindow, QObject* parent = nullptr);
    ~TrayManager();
    
    // 初始化托盘
    bool initialize();
    
    // 检查系统是否支持托盘
    bool isSystemTrayAvailable() const;
    
    // 显示托盘
    void show();
    
    // 隐藏托盘
    void hide();
    
    // 获取托盘图标对象
    QSystemTrayIcon* getTrayIcon() const { return m_trayIcon.get(); }

private slots:
    // 托盘图标激活处理
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    
    // 菜单项点击处理
    void onShowMainWindow();
    void onOneClickOrganize();
    void onShowSettings();
    void onExitApp();

private:
    // 创建托盘菜单
    void createTrayMenu();
    
    // 成员变量
    QMainWindow* m_mainWindow;
    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    std::unique_ptr<QMenu> m_trayMenu;
    
    // 菜单项
    QAction* m_showAction;
    QAction* m_organizeAction;
    QAction* m_settingsAction;
    QAction* m_exitAction;
};

} // namespace ccdesk::core

#endif // TRAY_MANAGER_H
