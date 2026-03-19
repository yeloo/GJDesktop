#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <memory>
#include <vector>
#include <string>

namespace ccdesk::core {
class Logger;
class ConfigManager;
class FileOrganizer;
class TrayManager;
} // namespace ccdesk::core

namespace ccdesk::ui {
class MainWindow;
} // namespace ccdesk::ui

namespace ccdesk::core {

class AppManager {
public:
    static AppManager& getInstance();
    
    // 初始化应用
    bool initialize();
    
    // 获取各模块
    Logger* getLogger() const;
    ConfigManager* getConfigManager() const;
    FileOrganizer* getFileOrganizer() const;
    TrayManager* getTrayManager() const;
    ui::MainWindow* getMainWindow() const;
    
    // 运行应用
    int run();
    
    // 清理资源
    void cleanup();
    
private:
    AppManager();
    ~AppManager();
    
    // 禁止拷贝
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;
    
    // 初始化Logger
    bool initializeLogger();
    
    // 初始化ConfigManager
    bool initializeConfigManager();
    
    // 初始化FileOrganizer
    bool initializeFileOrganizer();
    
    // 初始化UI
    bool initializeUI();
    
    // 初始化TrayManager
    bool initializeTrayManager();
    
    // 从配置恢复分区
    void restorePartitions();
    
    // 成员变量
    std::unique_ptr<Logger> m_logger;
    std::unique_ptr<ConfigManager> m_configManager;
    std::unique_ptr<FileOrganizer> m_fileOrganizer;
    std::unique_ptr<TrayManager> m_trayManager;
    std::unique_ptr<ui::MainWindow> m_mainWindow;
    
    std::string m_desktopPath;
};

} // namespace ccdesk::core

#endif // APP_MANAGER_H
