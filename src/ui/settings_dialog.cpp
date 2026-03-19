#include "settings_dialog.h"
#include "../core/config_manager.h"
#include "../core/file_organizer.h"
#include "../core/logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QTabWidget>
#include <QGroupBox>
#include <QApplication>
#include <QMessageBox>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace ccdesk::core;

namespace ccdesk::ui {

SettingsDialog::SettingsDialog(
    ConfigManager* configManager,
    FileOrganizer* fileOrganizer,
    QWidget* parent)
    : QDialog(parent)
    , m_startupCheckbox(nullptr)
    , m_rulesList(nullptr)
    , m_partitionsList(nullptr)
    , m_applyBtn(nullptr)
    , m_closeBtn(nullptr)
    , m_startupStatusLabel(nullptr)
    , m_configManager(configManager)
    , m_fileOrganizer(fileOrganizer)
{
    setWindowTitle("Settings");
    setGeometry(300, 300, 700, 500);
    setModal(true);
    
    initializeUI();
    loadSettings();
    
    Logger::getInstance().info("SettingsDialog: Initialized");
}

SettingsDialog::~SettingsDialog() {
    Logger::getInstance().info("SettingsDialog: Destroyed");
}

void SettingsDialog::initializeUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 标题
    QLabel* titleLabel = new QLabel("Application Settings", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // 标签页
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // ============ 标签页1：通用设置 ============
    QWidget* generalTab = new QWidget();
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);
    
    // 开机启动设置组
    QGroupBox* startupGroup = new QGroupBox("Startup Settings", this);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    
    m_startupCheckbox = new QCheckBox("Run on system startup", this);
    connect(m_startupCheckbox, QOverload<bool>::of(&QCheckBox::toggled),
            this, &SettingsDialog::onStartupToggled);
    startupLayout->addWidget(m_startupCheckbox);
    
    m_startupStatusLabel = new QLabel("Status: Not configured", this);
    m_startupStatusLabel->setStyleSheet("color: #666; font-size: 11px;");
    startupLayout->addWidget(m_startupStatusLabel);
    
    generalLayout->addWidget(startupGroup);
    generalLayout->addStretch();
    
    tabWidget->addTab(generalTab, "General");
    
    // ============ 标签页2：整理规则 ============
    QWidget* rulesTab = new QWidget();
    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesTab);
    
    QLabel* rulesLabel = new QLabel("Organization Rules:", this);
    rulesLabel->setStyleSheet("font-weight: bold;");
    rulesLayout->addWidget(rulesLabel);
    
    m_rulesList = new QListWidget(this);
    m_rulesList->setStyleSheet("QListWidget { border: 1px solid #ccc; }");
    rulesLayout->addWidget(m_rulesList);
    
    QPushButton* refreshRulesBtn = new QPushButton("Refresh Rules", this);
    connect(refreshRulesBtn, &QPushButton::clicked, this, &SettingsDialog::onRefreshRules);
    rulesLayout->addWidget(refreshRulesBtn);
    
    QLabel* rulesNote = new QLabel(
        "Note: This is a preview of organization rules.\n"
        "Rule editing will be available in future versions.",
        this);
    rulesNote->setStyleSheet("color: #999; font-size: 10px;");
    rulesLayout->addWidget(rulesNote);
    
    tabWidget->addTab(rulesTab, "Rules");
    
    // ============ 标签页3：分区管理 ============
    QWidget* partitionsTab = new QWidget();
    QVBoxLayout* partitionsLayout = new QVBoxLayout(partitionsTab);
    
    QLabel* partitionsLabel = new QLabel("Desktop Partitions:", this);
    partitionsLabel->setStyleSheet("font-weight: bold;");
    partitionsLayout->addWidget(partitionsLabel);
    
    m_partitionsList = new QListWidget(this);
    m_partitionsList->setStyleSheet("QListWidget { border: 1px solid #ccc; }");
    partitionsLayout->addWidget(m_partitionsList);
    
    QPushButton* refreshPartitionsBtn = new QPushButton("Refresh Partitions", this);
    connect(refreshPartitionsBtn, &QPushButton::clicked, this, &SettingsDialog::onRefreshPartitions);
    partitionsLayout->addWidget(refreshPartitionsBtn);
    
    QLabel* partitionsNote = new QLabel(
        "Note: This is a preview of desktop partitions.\n"
        "Partition editing will be available in future versions.",
        this);
    partitionsNote->setStyleSheet("color: #999; font-size: 10px;");
    partitionsLayout->addWidget(partitionsNote);
    
    tabWidget->addTab(partitionsTab, "Partitions");
    
    mainLayout->addWidget(tabWidget);
    
    // ============ 按钮 ============
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_applyBtn = new QPushButton("Apply", this);
    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApplySettings);
    buttonLayout->addWidget(m_applyBtn);
    
    m_closeBtn = new QPushButton("Close", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::loadSettings() {
    if (!m_configManager) {
        Logger::getInstance().warning("SettingsDialog: ConfigManager is null");
        return;
    }
    
    // 加载开机启动设置
    bool startupEnabled = m_configManager->isStartupEnabled();
    m_startupCheckbox->setChecked(startupEnabled);
    
    if (startupEnabled) {
        m_startupStatusLabel->setText("Status: Enabled (will run on next startup)");
        m_startupStatusLabel->setStyleSheet("color: green; font-size: 11px;");
    } else {
        m_startupStatusLabel->setText("Status: Disabled");
        m_startupStatusLabel->setStyleSheet("color: #666; font-size: 11px;");
    }
    
    Logger::getInstance().info("SettingsDialog: Settings loaded, startup enabled: " + 
                             std::string(startupEnabled ? "true" : "false"));
    
    // 加载规则列表
    updateRulesList();
    
    // 加载分区列表
    updatePartitionsList();
}

void SettingsDialog::updateRulesList() {
    m_rulesList->clear();
    
    if (!m_fileOrganizer) {
        return;
    }
    
    const auto& rules = m_fileOrganizer->getRules();
    
    for (const auto& rule : rules) {
        std::stringstream ss;
        ss << rule.name << " [" << rule.extensions << "]";
        
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(ss.str()));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_rulesList->addItem(item);
    }
    
    if (rules.empty()) {
        QListWidgetItem* item = new QListWidgetItem("No rules configured");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_rulesList->addItem(item);
    }
}

void SettingsDialog::updatePartitionsList() {
    m_partitionsList->clear();
    
    if (!m_configManager) {
        return;
    }
    
    const auto& partitions = m_configManager->getPartitions();
    
    for (const auto& partition : partitions) {
        std::stringstream ss;
        ss << partition.name << " [" << partition.targetPath << "]";
        
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(ss.str()));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_partitionsList->addItem(item);
    }
    
    if (partitions.empty()) {
        QListWidgetItem* item = new QListWidgetItem("No partitions configured");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_partitionsList->addItem(item);
    }
}

void SettingsDialog::onStartupToggled(bool checked) {
    Logger::getInstance().info("SettingsDialog: Startup toggle changed to: " + 
                             std::string(checked ? "enabled" : "disabled"));
    
    if (setStartupRegistry(checked)) {
        if (checked) {
            m_startupStatusLabel->setText("Status: Enabled (will run on next startup)");
            m_startupStatusLabel->setStyleSheet("color: green; font-size: 11px;");
        } else {
            m_startupStatusLabel->setText("Status: Disabled");
            m_startupStatusLabel->setStyleSheet("color: #666; font-size: 11px;");
        }
        
        // 保存到配置
        if (m_configManager) {
            m_configManager->setStartupEnabled(checked);
        }
    } else {
        Logger::getInstance().warning("SettingsDialog: Failed to set startup registry");
    }
}

bool SettingsDialog::setStartupRegistry(bool enable) {
#ifdef _WIN32
    // 获取应用路径
    std::string appPath = getApplicationPath();
    if (appPath.empty()) {
        Logger::getInstance().error("SettingsDialog: Failed to get application path");
        QMessageBox::warning(this, "Error", "Failed to get application path");
        return false;
    }
    
    // 注册表路径
    const char* regPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const char* appName = "CCDesk";
    
    HKEY hKey;
    LONG result;
    
    try {
        // 打开或创建注册表键
        result = RegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0, KEY_SET_VALUE, &hKey);
        if (result != ERROR_SUCCESS) {
            Logger::getInstance().error("SettingsDialog: Failed to open registry key, error: " + 
                                       std::to_string(result));
            QMessageBox::warning(this, "Error", "Failed to access registry");
            return false;
        }
        
        if (enable) {
            // 添加到启动项
            result = RegSetValueExA(hKey, appName, 0, REG_SZ, 
                                   (const BYTE*)appPath.c_str(), 
                                   appPath.length() + 1);
            
            if (result == ERROR_SUCCESS) {
                Logger::getInstance().info("SettingsDialog: Successfully added to startup registry");
            } else {
                Logger::getInstance().error("SettingsDialog: Failed to set registry value, error: " + 
                                           std::to_string(result));
                RegCloseKey(hKey);
                QMessageBox::warning(this, "Error", "Failed to set registry value");
                return false;
            }
        } else {
            // 从启动项移除
            result = RegDeleteValueA(hKey, appName);
            
            if (result == ERROR_SUCCESS) {
                Logger::getInstance().info("SettingsDialog: Successfully removed from startup registry");
            } else if (result == ERROR_FILE_NOT_FOUND) {
                // 键不存在，这也是成功的
                Logger::getInstance().info("SettingsDialog: Registry entry already not present");
            } else {
                Logger::getInstance().error("SettingsDialog: Failed to delete registry value, error: " + 
                                           std::to_string(result));
                RegCloseKey(hKey);
                QMessageBox::warning(this, "Error", "Failed to remove registry value");
                return false;
            }
        }
        
        RegCloseKey(hKey);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("SettingsDialog: Exception in setStartupRegistry: " + 
                                   std::string(e.what()));
        QMessageBox::warning(this, "Error", "Exception occurred: " + std::string(e.what()));
        return false;
    }
#else
    // Linux/Mac 暂不支持
    Logger::getInstance().warning("SettingsDialog: Startup registry not supported on this platform");
    QMessageBox::information(this, "Info", "Startup setting not supported on this platform");
    return false;
#endif
}

std::string SettingsDialog::getApplicationPath() const {
    // 获取应用程序完整路径
#ifdef _WIN32
    char path[260];
    GetModuleFileNameA(nullptr, path, sizeof(path));
    return std::string(path);
#else
    return "";
#endif
}

void SettingsDialog::onRefreshRules() {
    Logger::getInstance().info("SettingsDialog: Refreshing rules list");
    updateRulesList();
}

void SettingsDialog::onRefreshPartitions() {
    Logger::getInstance().info("SettingsDialog: Refreshing partitions list");
    updatePartitionsList();
}

void SettingsDialog::onApplySettings() {
    Logger::getInstance().info("SettingsDialog: Apply settings clicked");
    
    if (m_configManager) {
        if (m_configManager->save()) {
            Logger::getInstance().info("SettingsDialog: Settings saved successfully");
            QMessageBox::information(this, "Success", "Settings saved successfully");
        } else {
            Logger::getInstance().error("SettingsDialog: Failed to save settings");
            QMessageBox::warning(this, "Error", "Failed to save settings");
        }
    } else {
        Logger::getInstance().error("SettingsDialog: ConfigManager is null");
    }
}

} // namespace ccdesk::ui
