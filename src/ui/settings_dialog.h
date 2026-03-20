#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <memory>

class QCheckBox;
class QPushButton;
class QListWidget;
class QTabWidget;
class QLabel;

namespace ccdesk::core {
class ConfigManager;
class FileOrganizer;
} // namespace ccdesk::core

namespace ccdesk::ui {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(
        ccdesk::core::ConfigManager* configManager,
        ccdesk::core::FileOrganizer* fileOrganizer,
        QWidget* parent = nullptr);
    ~SettingsDialog();

private slots:
    // 开机启动开关
    void onStartupToggled(bool checked);
    
    // 保存设置
    void onApplySettings();
    
    // 刷新规则列表
    void onRefreshRules();

private:
    // UI组件
    QCheckBox* m_startupCheckbox;
    QListWidget* m_rulesList;
    QPushButton* m_applyBtn;
    QPushButton* m_closeBtn;
    QLabel* m_startupStatusLabel;
    
    // 核心模块指针
    ccdesk::core::ConfigManager* m_configManager;
    ccdesk::core::FileOrganizer* m_fileOrganizer;
    
    // 初始化UI
    void initializeUI();
    
    // 加载设置
    void loadSettings();
    
    // 更新规则列表显示
    void updateRulesList();
    
    // 分区列表显示已移除 - 当前版本不支持虚拟分区
    
    // 注册/移除开机启动
    bool setStartupRegistry(bool enable);
    
    // 获取应用路径
    std::string getApplicationPath() const;
};

} // namespace ccdesk::ui

#endif // SETTINGS_DIALOG_H
