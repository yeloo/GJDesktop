#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <memory>
#include <vector>
#include <sstream>

class QPushButton;
class QLabel;
class QDialog;
class QListWidget;

namespace ccdesk::core {
class FileOrganizer;
class TrayManager;
class ConfigManager;
struct OrganizePreviewItem;
struct OrganizeSummary;
struct OrganizeResult;
} // namespace ccdesk::core

namespace ccdesk::ui {

class PartitionWidget;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
    void setFileOrganizer(ccdesk::core::FileOrganizer* organizer);
    void setTrayManager(ccdesk::core::TrayManager* trayManager);
    void setConfigManager(ccdesk::core::ConfigManager* configManager);

public slots:
    // 从托盘触发的槽
    void triggerOrganizeFromTray();
    void showSettingsDialog();

private slots:
    // 一键整理按钮
    void onOrganizeClicked();
    
    // 预览对话框确认按钮
    void onPreviewConfirmed();
    
    // 预览对话框取消按钮
    void onPreviewCancelled();

protected:
    // 改造关闭事件为隐藏到托盘
    void closeEvent(QCloseEvent* event) override;

private:
    // UI组件
    QPushButton* m_organizeBtn;
    QPushButton* m_partitionBtn;
    QPushButton* m_settingsBtn;
    QLabel* m_statusLabel;
    
    // 文件整理器指针
    ccdesk::core::FileOrganizer* m_fileOrganizer;
    ccdesk::core::TrayManager* m_trayManager;
    ccdesk::core::ConfigManager* m_configManager;
    
    // 预览对话框
    QDialog* m_previewDialog;
    QListWidget* m_previewList;
    QPushButton* m_confirmBtn;
    QPushButton* m_cancelBtn;
    
    // 分区窗口管理
    std::vector<std::shared_ptr<PartitionWidget>> m_partitions;
    
    // 设置对话框
    SettingsDialog* m_settingsDialog;
    
    // 初始化UI
    void initializeUI();
    
    // 显示预览对话框
    void showPreviewDialog(const std::vector<ccdesk::core::OrganizePreviewItem>& items);
    
    // 显示结果报告
    void showResultReport(const ccdesk::core::OrganizeSummary& summary);
    
    // 隐藏到托盘
    void hideToTray();
    
    // 创建预览项文本（用于ListWidget显示）
    QString formatPreviewItem(const ccdesk::core::OrganizePreviewItem& item) const;
    
    // 创建结果项文本（用于结果报告显示）
    QString formatResultItem(const ccdesk::core::OrganizeResult& result) const;
};

} // namespace ccdesk::ui

#endif // MAIN_WINDOW_H
