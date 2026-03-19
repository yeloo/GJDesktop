#include "app_manager.h"
#include "logger.h"
#include "config_manager.h"
#include "file_organizer.h"
#include "tray_manager.h"
#include "../ui/main_window.h"

#include <QApplication>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace ccdesk::core {

// 静态单例实例
static AppManager* g_instance = nullptr;

AppManager& AppManager::getInstance() {
    if (!g_instance) {
        g_instance = new AppManager();
    }
    return *g_instance;
}

AppManager::AppManager()
    : m_logger(nullptr)
    , m_configManager(nullptr)
    , m_fileOrganizer(nullptr)
    , m_trayManager(nullptr)
    , m_mainWindow(nullptr)
{
    // 获取用户桌面路径
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        m_desktopPath = std::string(userProfile) + "\\Desktop";
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        m_desktopPath = std::string(home) + "/Desktop";
    }
#endif
    
    if (m_desktopPath.empty() || !fs::exists(m_desktopPath)) {
        m_desktopPath = ".";
    }
}

AppManager::~AppManager() {
    cleanup();
}

bool AppManager::initialize() {
    std::cout << "AppManager: Initializing..." << std::endl;
    
    if (!initializeLogger()) {
        std::cerr << "Failed to initialize Logger" << std::endl;
        return false;
    }
    
    Logger::getInstance().info("AppManager: Logger initialized");
    
    if (!initializeConfigManager()) {
        Logger::getInstance().error("AppManager: Failed to initialize ConfigManager");
        return false;
    }
    
    Logger::getInstance().info("AppManager: ConfigManager initialized");
    
    if (!initializeFileOrganizer()) {
        Logger::getInstance().error("AppManager: Failed to initialize FileOrganizer");
        return false;
    }
    
    Logger::getInstance().info("AppManager: FileOrganizer initialized");
    
    if (!initializeUI()) {
        Logger::getInstance().error("AppManager: Failed to initialize UI");
        return false;
    }
    
    Logger::getInstance().info("AppManager: UI initialized");
    
    if (!initializeTrayManager()) {
        Logger::getInstance().error("AppManager: Failed to initialize TrayManager");
        return false;
    }
    
    Logger::getInstance().info("AppManager: TrayManager initialized");
    
    // 恢复分区
    restorePartitions();
    
    Logger::getInstance().info("AppManager: Initialization complete");
    return true;
}

bool AppManager::initializeLogger() {
    // Logger 单例管理自己，直接调用 getInstance 
    m_logger.reset(&Logger::getInstance());
    return m_logger != nullptr;
}

bool AppManager::initializeConfigManager() {
    m_configManager = std::make_unique<ConfigManager>();
    return m_configManager != nullptr;
}

bool AppManager::initializeFileOrganizer() {
    m_fileOrganizer = std::make_unique<FileOrganizer>();
    
    m_fileOrganizer->setDesktopPath(m_desktopPath);
    Logger::getInstance().info("AppManager: FileOrganizer desktop path set to: " + m_desktopPath);
    
    if (m_configManager) {
        auto rules = m_configManager->getOrganizeRules();
        for (const auto& rule : rules) {
            m_fileOrganizer->addRule(rule);
        }
        Logger::getInstance().info("AppManager: Loaded " + 
                                 std::to_string(rules.size()) + " organize rules");
    }
    
    return m_fileOrganizer != nullptr;
}

bool AppManager::initializeUI() {
    m_mainWindow = std::make_unique<ui::MainWindow>();
    
    if (m_fileOrganizer) {
        m_mainWindow->setFileOrganizer(m_fileOrganizer.get());
    }
    
    if (m_configManager) {
        m_mainWindow->setConfigManager(m_configManager.get());
    }
    
    return m_mainWindow != nullptr;
}

bool AppManager::initializeTrayManager() {
    m_trayManager = std::make_unique<TrayManager>(m_mainWindow.get());
    
    if (!m_trayManager->initialize()) {
        Logger::getInstance().warning("AppManager: TrayManager initialization failed");
        return false;
    }
    
    // 设置MainWindow的TrayManager引用
    if (m_mainWindow) {
        m_mainWindow->setTrayManager(m_trayManager.get());
    }
    
    Logger::getInstance().info("AppManager: TrayManager initialized successfully");
    return true;
}

void AppManager::restorePartitions() {
    if (!m_configManager) {
        Logger::getInstance().warning("AppManager: ConfigManager is null, cannot restore partitions");
        return;
    }
    
    const auto& partitions = m_configManager->getPartitions();
    
    if (partitions.empty()) {
        Logger::getInstance().info("AppManager: No partitions to restore");
        return;
    }
    
    Logger::getInstance().info("AppManager: Restoring " + std::to_string(partitions.size()) + " partitions");
    
    // 分区恢复在后续阶段实现（UI需展示分区窗口）
    for (const auto& partition : partitions) {
        Logger::getInstance().debug("AppManager: Restored partition: " + partition.name + 
                                  " -> " + partition.targetPath);
    }
}

Logger* AppManager::getLogger() const {
    return m_logger.get();
}

ConfigManager* AppManager::getConfigManager() const {
    return m_configManager.get();
}

FileOrganizer* AppManager::getFileOrganizer() const {
    return m_fileOrganizer.get();
}

TrayManager* AppManager::getTrayManager() const {
    return m_trayManager.get();
}

ui::MainWindow* AppManager::getMainWindow() const {
    return m_mainWindow.get();
}

int AppManager::run() {
    if (!m_mainWindow) {
        Logger::getInstance().error("AppManager: MainWindow is null");
        return 1;
    }
    
    Logger::getInstance().info("AppManager: Showing MainWindow");
    m_mainWindow->show();
    
    return 0;
}

void AppManager::cleanup() {
    Logger::getInstance().info("AppManager: Cleaning up");
    
    m_mainWindow.reset();
    m_trayManager.reset();
    m_fileOrganizer.reset();
    m_configManager.reset();
    m_logger.reset();
}

} // namespace ccdesk::core
