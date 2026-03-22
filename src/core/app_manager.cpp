#include "app_manager.h"
#include "logger.h"
#include "config_manager.h"
#include "desktop_layout_manager.h"
#include "file_organizer.h"
#include "tray_manager.h"
#include "desktop_icon_accessor.h"
#include "desktop_auto_arrange_service.h"
#include "../ui/main_window.h"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace ccdesk::core {

// 辅助函数：将 QString 转换为 UTF-8 编码的 std::string
// Windows: QString (UTF-16) -> toUtf8() -> std::string (UTF-8)
// Linux/macOS: QString (UTF-8) -> toUtf8() -> std::string (UTF-8)
static std::string qstringToUtf8String(const QString& qstr) {
    return qstr.toUtf8().toStdString();
}

// 静态单例实例（使用 Meyer's Singleton Pattern）

AppManager& AppManager::getInstance() {
    static AppManager instance;
    return instance;
}

AppManager::AppManager()
    : m_logger(nullptr)
    , m_configManager(nullptr)
    , m_desktopLayoutManager(nullptr)
    , m_fileOrganizer(nullptr)
    , m_trayManager(nullptr)
    , m_mainWindow(nullptr)
{
    // 获取用户桌面路径 - 使用Qt标准路径API，确保平台兼容性
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    
    if (desktopPath.isEmpty()) {
        // 如果Qt获取失败，尝试使用传统方式
        #ifdef _WIN32
            const char* userProfile = std::getenv("USERPROFILE");
            if (userProfile) {
                // Windows 环境变量是 GBK 编码，需要转换为 UTF-8
                m_desktopPath = qstringToUtf8String(QString::fromLocal8Bit(userProfile) + "\\Desktop");
            }
        #else
            const char* home = std::getenv("HOME");
            if (home) {
                // Linux/macOS 环境变量是 UTF-8 编码
                m_desktopPath = std::string(home) + "/Desktop";
            }
        #endif
    } else {
        // Qt 返回的 QString 直接转换为 UTF-8
        m_desktopPath = qstringToUtf8String(desktopPath);
    }
    
    // 验证桌面路径是否存在
    if (m_desktopPath.empty()) {
        Logger::getInstance().error("AppManager: Failed to get desktop path");
    } else if (!fs::exists(m_desktopPath)) {
        Logger::getInstance().warning("AppManager: Desktop path does not exist: " + m_desktopPath);
        // 不再静默回退到exe目录，让上层处理这个错误
    }
}

AppManager::~AppManager() {
    cleanup();
}

bool AppManager::initialize() {

    Logger::getInstance().info("==================================================");
    Logger::getInstance().info("AppManager: initialize() 开始");

    if (!initializeLogger()) {
        Logger::getInstance().error("AppManager: initialize() 失败 - Logger 初始化失败");
        return false;
    }

    Logger::getInstance().info("AppManager: Logger initialized");

    if (!initializeConfigManager()) {
        Logger::getInstance().error("AppManager: initialize() 失败 - ConfigManager 初始化失败");
        return false;
    }

    Logger::getInstance().info("AppManager: ConfigManager initialized");

    if (!initializeDesktopLayoutManager()) {
        Logger::getInstance().error("AppManager: initialize() 失败 - DesktopLayoutManager 初始化失败");
        return false;
    }

    Logger::getInstance().info("AppManager: DesktopLayoutManager initialized");

    if (!initializeFileOrganizer()) {
        Logger::getInstance().error("AppManager: initialize() 失败 - FileOrganizer 初始化失败");
        return false;
    }

    Logger::getInstance().info("AppManager: FileOrganizer initialized");

    if (!initializeUI()) {
        Logger::getInstance().error("AppManager: initialize() 失败 - UI 初始化失败");
        return false;
    }

    Logger::getInstance().info("AppManager: UI initialized");

    if (!initializeTrayManager()) {
        Logger::getInstance().error("AppManager: initialize() 失败 - TrayManager 初始化失败");
        return false;
    }

    Logger::getInstance().info("AppManager: TrayManager initialized");

    // 初始化 DesktopIconAccessor（可选功能，不预检查桌面可访问性）
    if (!initializeDesktopIconAccessor()) {
        Logger::getInstance().warning("AppManager: DesktopIconAccessor initialization failed (non-critical)");
    }

    // 初始化 DesktopAutoArrangeService（自动整理服务）
    if (!initializeAutoArrangeService()) {
        Logger::getInstance().warning("AppManager: DesktopAutoArrangeService initialization failed (non-critical)");
    }

    // 恢复分区
    restorePartitions();

    Logger::getInstance().info("AppManager: initialize() 完成");
    Logger::getInstance().info("==================================================");
    return true;
}

bool AppManager::initializeLogger() {
    // Logger 单例，直接获取引用并存储指针
    m_logger = &Logger::getInstance();

    // 确保日志路径已设置（main.cpp 已设置，但做双重检查）
    if (m_logger && m_logger->getLogPath().empty()) {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString logPath = appDataDir + "/logs/ccdesk.log";
        QDir dir(appDataDir);
        dir.mkpath(appDataDir + "/logs");
        // 转换为 UTF-8
        m_logger->setLogPath(qstringToUtf8String(logPath));
    }

    return m_logger != nullptr;
}

bool AppManager::initializeConfigManager() {
    m_configManager = std::make_unique<ConfigManager>();
    return m_configManager != nullptr;
}

bool AppManager::initializeDesktopLayoutManager() {
    m_desktopLayoutManager = std::make_unique<DesktopLayoutManager>();
    
    m_desktopLayoutManager->setDesktopPath(m_desktopPath);
    Logger::getInstance().info("AppManager: DesktopLayoutManager initialized with desktop path: " + m_desktopPath);
    
    return m_desktopLayoutManager != nullptr;
}

bool AppManager::initializeFileOrganizer() {
    m_fileOrganizer = std::make_unique<FileOrganizer>();
    
    m_fileOrganizer->setDesktopPath(m_desktopPath);
    Logger::getInstance().info("AppManager: FileOrganizer desktop path set to: " + m_desktopPath);
    
    // 设置桌面布局管理器
    if (m_desktopLayoutManager) {
        m_fileOrganizer->setDesktopLayoutManager(m_desktopLayoutManager.get());
    }
    
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

bool AppManager::initializeDesktopIconAccessor() {
    m_desktopIconAccessor = std::make_unique<DesktopIconAccessor>();
    return m_desktopIconAccessor != nullptr;
}

bool AppManager::initializeAutoArrangeService() {
    m_autoArrangeService = std::make_unique<DesktopAutoArrangeService>();
    return m_autoArrangeService != nullptr;
}

void AppManager::restorePartitions() {
    if (!m_configManager) {
        Logger::getInstance().warning("AppManager: ConfigManager is null, cannot load partition config");
        return;
    }

    const auto& partitions = m_configManager->getPartitions();

    if (partitions.empty()) {
        Logger::getInstance().info("AppManager: No partition config found");
        return;
    }

    Logger::getInstance().info("AppManager: Loaded " + std::to_string(partitions.size()) + " partition configs (compatibility only)");

    // 当前版本仅加载分区配置作为兼容残留，不代表已实现分区系统
    // 这些配置仅用于分类统计，不会创建虚拟分区窗口
    for (const auto& partition : partitions) {
        Logger::getInstance().debug("AppManager: Loaded partition config: " + partition.name +
                                  " (category: " + std::to_string(partition.category) + ")");
    }

    // 检查是否需要在启动时自动整理桌面
    if (m_configManager && m_configManager->isAutoArrangeOnStartup()) {
        Logger::getInstance().info("AppManager: Auto-generate layout plan on startup enabled, executing...");
        generateLayoutPlan();  // 【行为变更】启动时调用 generateLayoutPlan 而非 arrangeDesktop
    }
}

//=============================================================================
// 生成桌面布局规划（规划/预演，不执行真实写回）
//=============================================================================

void AppManager::generateLayoutPlan() {
    Logger::getInstance().info("==================================================");
    Logger::getInstance().info("AppManager: generateLayoutPlan() 开始");

    if (!m_autoArrangeService) {
        Logger::getInstance().error("AppManager: generateLayoutPlan() 失败 - DesktopAutoArrangeService is null");
        Logger::getInstance().info("==================================================");
        return;
    }

    // 调用服务层的布局规划（不执行写回）
    LayoutPlanResult result = m_autoArrangeService->generateLayoutPlan();

    if (result.success()) {
        Logger::getInstance().info("AppManager: generateLayoutPlan() 成功完成");
        Logger::getInstance().info("  Total icons:   " + std::to_string(result.totalIcons));
        Logger::getInstance().info("  Categorized:   " + std::to_string(result.categorizedIcons));
        Logger::getInstance().info("  Planned icons: " + std::to_string(result.plannedIcons));
        Logger::getInstance().info("  注意：当前仅规划，不执行真实桌面图标写回");
    } else {
        Logger::getInstance().error("AppManager: generateLayoutPlan() 失败: " + result.errorMessage);
        Logger::getInstance().error("  Total icons: " + std::to_string(result.totalIcons));
    }

    Logger::getInstance().info("AppManager: generateLayoutPlan() 结束");
    Logger::getInstance().info("==================================================");
}

//=============================================================================
// 自动整理桌面（真实写回路线）
//=============================================================================

void AppManager::arrangeDesktop() {
    Logger::getInstance().info("==================================================");
    Logger::getInstance().info("AppManager: arrangeDesktop() 开始");

    if (!m_autoArrangeService) {
        Logger::getInstance().error("AppManager: arrangeDesktop() 失败 - DesktopAutoArrangeService is null");
        Logger::getInstance().info("==================================================");
        return;
    }

    // 调用自动整理服务
    AutoArrangeResult result = m_autoArrangeService->arrangeDesktop();

    // 输出结果
    if (result.success()) {
        Logger::getInstance().info("AppManager: arrangeDesktop() 成功完成");
        Logger::getInstance().info("  Total icons: " + std::to_string(result.totalIcons));
        Logger::getInstance().info("  Categorized icons: " + std::to_string(result.categorizedIcons));
        Logger::getInstance().info("  Moved icons: " + std::to_string(result.movedIcons));
        Logger::getInstance().info("  Failed icons: " + std::to_string(result.failedIcons));
    } else {
        Logger::getInstance().error("AppManager: arrangeDesktop() 失败: " + result.errorMessage);
        Logger::getInstance().error("  Total icons: " + std::to_string(result.totalIcons));
        Logger::getInstance().error("  Failed icons: " + std::to_string(result.failedIcons));
    }

    Logger::getInstance().info("AppManager: arrangeDesktop() 结束");
    Logger::getInstance().info("==================================================");
}

Logger* AppManager::getLogger() const {
    return m_logger;
}

ConfigManager* AppManager::getConfigManager() const {
    return m_configManager.get();
}

FileOrganizer* AppManager::getFileOrganizer() const {
    return m_fileOrganizer.get();
}

DesktopLayoutManager* AppManager::getDesktopLayoutManager() const {
    return m_desktopLayoutManager.get();
}

TrayManager* AppManager::getTrayManager() const {
    return m_trayManager.get();
}

DesktopIconAccessor* AppManager::getDesktopIconAccessor() const {
    return m_desktopIconAccessor.get();
}

DesktopAutoArrangeService* AppManager::getAutoArrangeService() const {
    return m_autoArrangeService.get();
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
    m_autoArrangeService.reset();
    m_desktopIconAccessor.reset();
    m_fileOrganizer.reset();
    m_desktopLayoutManager.reset();
    m_configManager.reset();
    // m_logger 是单例 raw pointer，不需要手动清理
    // 单例的生命周期由程序管理，不应在 AppManager 中销毁
    m_logger = nullptr;
}

} // namespace ccdesk::core
