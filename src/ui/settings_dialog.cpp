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
    , m_applyBtn(nullptr)
    , m_closeBtn(nullptr)
    , m_startupStatusLabel(nullptr)
    , m_configManager(configManager)
    , m_fileOrganizer(fileOrganizer)
{
    setWindowTitle("设置");
    setGeometry(300, 300, 700, 500);
    setModal(true);
    
    initializeUI();
    loadSettings();
    
    Logger::getInstance().info("SettingsDialog: 设置对话框初始化完成");
}

SettingsDialog::~SettingsDialog() {
    Logger::getInstance().info("SettingsDialog: Destroyed");
}

void SettingsDialog::initializeUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 标题
    QLabel* titleLabel = new QLabel("应用程序设置", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // 标签页
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // ============ 标签页1：通用设置 ============
    QWidget* generalTab = new QWidget();
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);
    
    // 开机启动设置组
    QGroupBox* startupGroup = new QGroupBox("开机启动设置", this);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    
    m_startupCheckbox = new QCheckBox("系统启动时自动运行", this);
    connect(m_startupCheckbox, QOverload<bool>::of(&QCheckBox::toggled),
            this, &SettingsDialog::onStartupToggled);
    startupLayout->addWidget(m_startupCheckbox);
    
    m_startupStatusLabel = new QLabel("状态: 未配置", this);
    m_startupStatusLabel->setStyleSheet("color: #666; font-size: 11px;");
    startupLayout->addWidget(m_startupStatusLabel);
    
    generalLayout->addWidget(startupGroup);
    generalLayout->addStretch();
    
    tabWidget->addTab(generalTab, "通用");
    
    // ============ 标签页2：整理规则 ============
    QWidget* rulesTab = new QWidget();
    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesTab);
    
    QLabel* rulesLabel = new QLabel("整理规则:", this);
    rulesLabel->setStyleSheet("font-weight: bold;");
    rulesLayout->addWidget(rulesLabel);
    
    m_rulesList = new QListWidget(this);
    m_rulesList->setStyleSheet("QListWidget { border: 1px solid #ccc; }");
    rulesLayout->addWidget(m_rulesList);
    
    QPushButton* refreshRulesBtn = new QPushButton("刷新规则", this);
    connect(refreshRulesBtn, &QPushButton::clicked, this, &SettingsDialog::onRefreshRules);
    rulesLayout->addWidget(refreshRulesBtn);
    
    QLabel* rulesNote = new QLabel(
        "说明：这是一个整理规则的预览。\n"
        "规则编辑功能将在后续版本中提供。",
        this);
    rulesNote->setStyleSheet("color: #999; font-size: 10px;");
    rulesLayout->addWidget(rulesNote);
    
    tabWidget->addTab(rulesTab, "规则");
    
    // 分区管理标签页已移除 - 当前版本不支持虚拟分区
    
    mainLayout->addWidget(tabWidget);
    
    // ============ 按钮 ============
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_applyBtn = new QPushButton("应用", this);
    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApplySettings);
    buttonLayout->addWidget(m_applyBtn);
    
    m_closeBtn = new QPushButton("关闭", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::loadSettings() {
    if (!m_configManager) {
        Logger::getInstance().warning("SettingsDialog: ConfigManager 为空");
        return;
    }
    
    // 加载开机启动设置
    bool startupEnabled = m_configManager->isStartupEnabled();
    m_startupCheckbox->setChecked(startupEnabled);
    
    if (startupEnabled) {
        m_startupStatusLabel->setText("状态: 已启用 (将在下次系统启动时运行)");
        m_startupStatusLabel->setStyleSheet("color: green; font-size: 11px;");
    } else {
        m_startupStatusLabel->setText("状态: 已禁用");
        m_startupStatusLabel->setStyleSheet("color: #666; font-size: 11px;");
    }
    
    Logger::getInstance().info("SettingsDialog: 设置已加载，开机启动: " + 
                             std::string(startupEnabled ? "启用" : "禁用"));
    
    // 加载规则列表
    updateRulesList();
    
    // 分区列表加载已移除
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
        QListWidgetItem* item = new QListWidgetItem("未配置任何规则");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_rulesList->addItem(item);
    }
}

// updatePartitionsList() 已移除 - 当前版本不支持虚拟分区

void SettingsDialog::onStartupToggled(bool checked) {
    Logger::getInstance().info("SettingsDialog: 开机启动切换为: " + 
                             std::string(checked ? "启用" : "禁用"));
    
    if (setStartupRegistry(checked)) {
        if (checked) {
            m_startupStatusLabel->setText("状态: 已启用 (将在下次系统启动时运行)");
            m_startupStatusLabel->setStyleSheet("color: green; font-size: 11px;");
        } else {
            m_startupStatusLabel->setText("状态: 已禁用");
            m_startupStatusLabel->setStyleSheet("color: #666; font-size: 11px;");
        }
        
        // 保存到配置
        if (m_configManager) {
            m_configManager->setStartupEnabled(checked);
        }
    } else {
        Logger::getInstance().warning("SettingsDialog: 设置开机启动注册表失败");
    }
}

bool SettingsDialog::setStartupRegistry(bool enable) {
#ifdef _WIN32
    // 获取应用路径
    std::string appPath = getApplicationPath();
    if (appPath.empty()) {
        Logger::getInstance().error("SettingsDialog: 获取应用程序路径失败");
        QMessageBox::warning(this, "错误", "获取应用程序路径失败");
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
            Logger::getInstance().error("SettingsDialog: 打开注册表键失败，错误代码: " + 
                                       std::to_string(result));
            QMessageBox::warning(this, "错误", "访问注册表失败");
            return false;
        }
        
        if (enable) {
            // 添加到启动项
            result = RegSetValueExA(hKey, appName, 0, REG_SZ, 
                                   (const BYTE*)appPath.c_str(), 
                                   appPath.length() + 1);
            
            if (result == ERROR_SUCCESS) {
                Logger::getInstance().info("SettingsDialog: 成功添加到开机启动注册表");
            } else {
                Logger::getInstance().error("SettingsDialog: 设置注册表值失败，错误代码: " + 
                                           std::to_string(result));
                RegCloseKey(hKey);
                QMessageBox::warning(this, "错误", "设置注册表值失败");
                return false;
            }
        } else {
            // 从启动项移除
            result = RegDeleteValueA(hKey, appName);
            
            if (result == ERROR_SUCCESS) {
                Logger::getInstance().info("SettingsDialog: 成功从开机启动注册表移除");
            } else if (result == ERROR_FILE_NOT_FOUND) {
                // 键不存在，这也是成功的
                Logger::getInstance().info("SettingsDialog: 注册表项已不存在");
            } else {
                Logger::getInstance().error("SettingsDialog: 删除注册表值失败，错误代码: " + 
                                           std::to_string(result));
                RegCloseKey(hKey);
                QMessageBox::warning(this, "错误", "删除注册表值失败");
                return false;
            }
        }
        
        RegCloseKey(hKey);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("SettingsDialog: setStartupRegistry 异常: " + 
                                   std::string(e.what()));
        QMessageBox::warning(this, "错误", QString::fromStdString("发生异常: " + std::string(e.what())));
        return false;
    }
#else
    // Linux/Mac 暂不支持
    Logger::getInstance().warning("SettingsDialog: 当前平台不支持开机启动设置");
    QMessageBox::information(this, "信息", "当前平台不支持开机启动设置");
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
    Logger::getInstance().info("SettingsDialog: 刷新规则列表");
    updateRulesList();
}

// onRefreshPartitions() 已移除 - 当前版本不支持虚拟分区

void SettingsDialog::onApplySettings() {
    Logger::getInstance().info("SettingsDialog: 点击了应用设置");
    
    if (m_configManager) {
        if (m_configManager->save()) {
            Logger::getInstance().info("SettingsDialog: 设置保存成功");
            QMessageBox::information(this, "成功", "设置保存成功");
        } else {
            Logger::getInstance().error("SettingsDialog: 设置保存失败");
            QMessageBox::warning(this, "错误", "设置保存失败");
        }
    } else {
        Logger::getInstance().error("SettingsDialog: ConfigManager 为空");
    }
}

} // namespace ccdesk::ui
