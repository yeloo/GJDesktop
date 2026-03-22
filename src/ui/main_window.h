#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <memory>
#include <vector>
#include <sstream>
#include <QTimer>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QProgressBar>

class QPushButton;
class QWidget;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QTabWidget;
class QTextEdit;
class QGroupBox;
class QDialog;

namespace ccdesk::core {
class FileOrganizer;
class TrayManager;
class ConfigManager;
class DesktopAutoArrangeService;
struct OrganizePreviewItem;
struct OrganizeSummary;
struct OrganizePlan;
struct LayoutPlanResult;
} // namespace ccdesk::core

namespace ccdesk::ui {

class SettingsDialog;

// 数据模型：文件整理结果展示项
struct OrganizeResultItem {
    QString fileName;
    QString category;
    QString status;  // "可归类" / "未匹配" / "冲突"
    QString statusDetail;
};

// 数据模型：桌面布局规划结果展示项
struct LayoutResultItem {
    QString iconName;
    QString category;
    QString targetPosition;  // "(X, Y)"
    QString status;  // "已规划"
};

// 数据模型：主窗口状态摘要
struct MainWindowState {
    // 文件整理模块状态
    int fileTotalItems = 0;
    int fileCategorizedItems = 0;
    std::map<QString, int> fileCategoryCounts;  // category name -> count
    
    // 桌面布局模块状态
    int iconTotalCount = 0;
    int iconCategorizedCount = 0;
    int iconPlannedCount = 0;
    std::map<QString, int> iconCategoryCounts;
    
    // 最近操作结果
    QString lastOperation;
    QString lastOperationResult;  // "成功" / "失败"
    QString lastOperationDetail;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
    // 设置各模块引用
    void setFileOrganizer(ccdesk::core::FileOrganizer* organizer);
    void setTrayManager(ccdesk::core::TrayManager* trayManager);
    void setConfigManager(ccdesk::core::ConfigManager* configManager);
    void setDesktopAutoArrangeService(ccdesk::core::DesktopAutoArrangeService* service);

public slots:
    // 从托盘触发的槽
    void triggerOrganizeFromTray();
    void triggerLayoutFromTray();
    void showSettingsDialog();

private slots:
    // 文件整理模块操作
    void onGenerateFileOrganizePlan();
    void onRefreshFileResults();
    
    // 桌面布局模块操作
    void onGenerateLayoutPlan();
    void onRefreshLayoutResults();
    
    // 设置
    void onShowSettings();
    
    // 定时更新日志摘要
    void onUpdateLogSummary();

protected:
    // 拦截关闭事件为隐藏到托盘
    void closeEvent(QCloseEvent* event) override;

private:
    // 初始化 UI
    void initializeUI();
    
    // 创建各个区域
    QWidget* createStatusBar();
    QWidget* createFileOrganizeSection();
    QWidget* createLayoutSection();
    QWidget* createLogSection();
    QGroupBox* createFileStatsBox();
    QGroupBox* createLayoutStatsBox();
    
    // 更新状态栏
    void updateStatusBar(const MainWindowState& state);
    
    // 显示文件整理规划结果
    void showFileOrganizeResults(const MainWindowState& state);
    
    // 显示桌面布局规划结果
    void showLayoutResults(const MainWindowState& state);
    
    // 更新日志摘要
    void updateLogSummary();
    
    // 格式化文件整理结果项
    OrganizeResultItem formatOrganizeResultItem(const ccdesk::core::OrganizePreviewItem& item) const;
    
    // 格式化布局结果项
    LayoutResultItem formatLayoutResultItem(const ccdesk::core::LayoutPlanResult& result, size_t index) const;
    
    // 获取分类中文名称（从 IconCategory 枚举值）
    QString getCategoryDisplayName(const QString& category) const;
    
    // 隐藏到托盘
    void hideToTray();

    // 成员变量
    ccdesk::core::FileOrganizer* m_fileOrganizer;
    ccdesk::core::TrayManager* m_trayManager;
    ccdesk::core::ConfigManager* m_configManager;
    ccdesk::core::DesktopAutoArrangeService* m_autoArrangeService;
    
    MainWindowState m_state;
    
    // UI 组件
    QLabel* m_fileTotalLabel;
    QLabel* m_fileCategorizedLabel;
    QLabel* m_fileStatsLabel;  // 分类统计显示
    
    QLabel* m_iconTotalLabel;
    QLabel* m_iconCategorizedLabel;
    QLabel* m_iconPlannedLabel;
    QLabel* m_iconStatsLabel;  // 分类统计显示
    
    QTextEdit* m_fileResultList;
    QTextEdit* m_layoutResultList;
    QTextEdit* m_logSummary;
    
    QPushButton* m_generateFilePlanBtn;
    QPushButton* m_refreshFileResultsBtn;
    QPushButton* m_generateLayoutPlanBtn;
    QPushButton* m_refreshLayoutResultsBtn;
    QPushButton* m_settingsBtn;
    
    QTimer* m_logUpdateTimer;  // 定时更新日志摘要
    
    // 最近的文件整理规划结果
    ccdesk::core::OrganizePlan m_lastFilePlan;
    
    // 最近的桌面布局规划结果
    ccdesk::core::LayoutPlanResult m_lastLayoutPlan;
};

} // namespace ccdesk::ui

#endif // MAIN_WINDOW_H
