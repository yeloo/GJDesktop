#include "main_window.h"
#include "settings_dialog.h"
#include "partition_widget.h"
#include "../core/file_organizer.h"
#include "../core/organize_result.h"
#include "../core/logger.h"
#include "../core/tray_manager.h"
#include "../core/config_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QCloseEvent>

using namespace ccdesk::core;

namespace ccdesk::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_fileOrganizer(nullptr)
    , m_trayManager(nullptr)
    , m_configManager(nullptr)
    , m_previewDialog(nullptr)
    , m_previewList(nullptr)
    , m_confirmBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_settingsDialog(nullptr)
{
    setWindowTitle("CCDesk - Desktop Organizer");
    setGeometry(100, 100, 600, 400);
    
    initializeUI();
    
    Logger::getInstance().info("MainWindow: Initialized");
}

MainWindow::~MainWindow() {
    Logger::getInstance().info("MainWindow: Destroyed");
}

void MainWindow::setFileOrganizer(FileOrganizer* organizer) {
    m_fileOrganizer = organizer;
    Logger::getInstance().info("MainWindow: FileOrganizer set");
}

void MainWindow::setTrayManager(TrayManager* trayManager) {
    m_trayManager = trayManager;
    Logger::getInstance().info("MainWindow: TrayManager set");
}

void MainWindow::setConfigManager(ConfigManager* configManager) {
    m_configManager = configManager;
    Logger::getInstance().info("MainWindow: ConfigManager set");
}

void MainWindow::initializeUI() {
    // 创建中央组件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // 标题标签
    QLabel* titleLabel = new QLabel("Desktop Organization Tool", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // 功能按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // 一键整理按钮
    m_organizeBtn = new QPushButton("One-Click Organize", this);
    connect(m_organizeBtn, &QPushButton::clicked, this, &MainWindow::onOrganizeClicked);
    buttonLayout->addWidget(m_organizeBtn);
    
    // 分区管理按钮
    m_partitionBtn = new QPushButton("Manage Partitions", this);
    buttonLayout->addWidget(m_partitionBtn);
    
    // 设置按钮
    m_settingsBtn = new QPushButton("Settings", this);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettingsDialog);
    buttonLayout->addWidget(m_settingsBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // 状态标签
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("color: gray;");
    mainLayout->addWidget(m_statusLabel);
    
    // 弹簧（填充剩余空间）
    mainLayout->addStretch();
}

void MainWindow::onOrganizeClicked() {
    if (!m_fileOrganizer) {
        QMessageBox::warning(this, "Error", "FileOrganizer not initialized");
        Logger::getInstance().error("MainWindow: FileOrganizer is null");
        return;
    }
    
    Logger::getInstance().info("MainWindow: User clicked 'One-Click Organize' button");
    m_statusLabel->setText("Scanning desktop files...");
    
    // 生成预览
    auto previewItems = m_fileOrganizer->generatePreview();
    
    if (previewItems.empty()) {
        Logger::getInstance().info("MainWindow: No files found on desktop to organize");
        QMessageBox::information(this, "No Files", 
            "No files found on desktop to organize");
        m_statusLabel->setText("Ready - No files to organize");
        return;
    }
    
    // 显示预览对话框
    showPreviewDialog(previewItems);
}

void MainWindow::showPreviewDialog(
    const std::vector<OrganizePreviewItem>& items) {
    
    // 如果对话框已存在，先销毁
    if (m_previewDialog) {
        delete m_previewDialog;
    }
    
    // 创建预览对话框
    m_previewDialog = new QDialog(this, Qt::Dialog | Qt::WindowCloseButtonHint);
    m_previewDialog->setWindowTitle("Organization Preview");
    m_previewDialog->setGeometry(200, 150, 700, 500);
    
    QVBoxLayout* dialogLayout = new QVBoxLayout(m_previewDialog);
    
    // 标题
    QLabel* titleLabel = new QLabel("Preview - Ready to organize:", m_previewDialog);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    dialogLayout->addWidget(titleLabel);
    
    // 预览列表
    m_previewList = new QListWidget(m_previewDialog);
    m_previewList->setStyleSheet("QListWidget { border: 1px solid #ccc; }");
    
    // 统计数据
    int movableCount = 0;
    int conflictCount = 0;
    int noRuleCount = 0;
    
    for (const auto& item : items) {
        QString itemText = formatPreviewItem(item);
        m_previewList->addItem(itemText);
        
        switch (item.status) {
            case OrganizePreviewItem::Movable:
                movableCount++;
                break;
            case OrganizePreviewItem::Conflict:
                conflictCount++;
                break;
            case OrganizePreviewItem::NoRule:
                noRuleCount++;
                break;
        }
    }
    
    dialogLayout->addWidget(m_previewList);
    
    // 统计信息 - 更清晰的格式
    std::stringstream ss;
    ss << "Total: " << items.size() << " files | "
       << "Movable: " << movableCount << " | "
       << "Conflict: " << conflictCount << " | "
       << "No Rule: " << noRuleCount;
    
    QLabel* statsLabel = new QLabel(QString::fromStdString(ss.str()), m_previewDialog);
    statsLabel->setStyleSheet("background-color: #fffacd; padding: 8px; border: 1px solid #f0e68c; "
                             "color: #333; font-size: 11px; border-radius: 3px;");
    dialogLayout->addWidget(statsLabel);
    
    // 按钮布局
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    m_confirmBtn = new QPushButton("Confirm Execute", m_previewDialog);
    m_confirmBtn->setMinimumWidth(120);
    connect(m_confirmBtn, &QPushButton::clicked, this, &MainWindow::onPreviewConfirmed);
    btnLayout->addWidget(m_confirmBtn);
    
    m_cancelBtn = new QPushButton("Cancel", m_previewDialog);
    m_cancelBtn->setMinimumWidth(120);
    connect(m_cancelBtn, &QPushButton::clicked, this, &MainWindow::onPreviewCancelled);
    btnLayout->addWidget(m_cancelBtn);
    
    dialogLayout->addLayout(btnLayout);
    
    Logger::getInstance().info("MainWindow: Preview dialog generated with " + 
                             std::to_string(items.size()) + " items");
    
    // 显示对话框
    m_previewDialog->exec();
}

void MainWindow::onPreviewConfirmed() {
    if (!m_fileOrganizer) {
        QMessageBox::warning(this, "Error", "FileOrganizer not initialized");
        Logger::getInstance().error("MainWindow: FileOrganizer is null during confirm");
        return;
    }
    
    Logger::getInstance().info("MainWindow: User confirmed execution, starting real organization");
    
    m_statusLabel->setText("Executing organization...");
    
    // 关闭预览对话框
    if (m_previewDialog) {
        m_previewDialog->close();
    }
    
    // 执行真实整理
    OrganizeSummary summary = m_fileOrganizer->executeOrganize();
    
    // 记录结果摘要
    Logger::getInstance().info("MainWindow: Execution completed - " +
                             std::to_string(summary.movedCount) + " moved, " +
                             std::to_string(summary.skippedConflictCount) + " skipped, " +
                             std::to_string(summary.failedCount) + " failed, " +
                             std::to_string(summary.noRuleCount) + " no rule");
    
    // 显示结果报告
    showResultReport(summary);
    
    m_statusLabel->setText("Organization complete");
}

void MainWindow::onPreviewCancelled() {
    Logger::getInstance().info("MainWindow: User cancelled preview");
    m_statusLabel->setText("Organization cancelled");
    
    if (m_previewDialog) {
        m_previewDialog->close();
    }
}

QString MainWindow::formatPreviewItem(const OrganizePreviewItem& item) const {
    std::stringstream ss;
    ss << "[" << item.fileName << "] -> ";
    
    switch (item.status) {
        case OrganizePreviewItem::Movable:
            ss << "Move to " << item.targetDirectory << " (Rule: " 
               << item.matchedRuleName << ")";
            break;
        case OrganizePreviewItem::Conflict:
            ss << "CONFLICT - File exists in " << item.targetDirectory;
            break;
        case OrganizePreviewItem::NoRule:
            ss << "NO RULE - No matching organization rule";
            break;
    }
    
    return QString::fromStdString(ss.str());
}

QString MainWindow::formatResultItem(const OrganizeResult& result) const {
    std::stringstream ss;
    ss << "[" << result.fileName << "] ";
    
    switch (result.finalStatus) {
        case OrganizeResult::Success:
            ss << "✓ SUCCESS - Moved to " << result.targetPath;
            break;
        case OrganizeResult::SkippedConflict:
            ss << "⊗ SKIPPED - Conflict: " << result.message;
            break;
        case OrganizeResult::Failed:
            ss << "✗ FAILED - " << result.message;
            break;
        case OrganizeResult::NoRule:
            ss << "○ NO RULE - " << result.message;
            break;
    }
    
    return QString::fromStdString(ss.str());
}

void MainWindow::showResultReport(const OrganizeSummary& summary) {
    // 创建结果报告对话框
    QDialog* resultDialog = new QDialog(this, Qt::Dialog | Qt::WindowCloseButtonHint);
    resultDialog->setWindowTitle("Organization Result Report");
    resultDialog->setGeometry(150, 100, 800, 600);
    
    QVBoxLayout* layout = new QVBoxLayout(resultDialog);
    
    // 标题
    QLabel* titleLabel = new QLabel("Organization Execution Report", resultDialog);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    layout->addWidget(titleLabel);
    
    // 汇总统计 - 优化显示格式
    std::stringstream summaryText;
    summaryText << "\n";
    summaryText << "  ════════════ SUMMARY ════════════\n";
    summaryText << "  Total Files Processed:  " << summary.getTotalProcessed() << "\n";
    summaryText << "  ✓ Successfully Moved:   " << summary.movedCount << "\n";
    summaryText << "  ⊗ Conflict Skipped:     " << summary.skippedConflictCount << "\n";
    summaryText << "  ✗ Move Failed:          " << summary.failedCount << "\n";
    summaryText << "  ○ No Matching Rule:     " << summary.noRuleCount << "\n";
    summaryText << "  ════════════════════════════════\n";
    
    QLabel* summaryLabel = new QLabel(QString::fromStdString(summaryText.str()), resultDialog);
    summaryLabel->setStyleSheet("background-color: #e8f4f8; padding: 15px; "
                               "font-family: 'Courier New', monospace; font-size: 12px; "
                               "border: 2px solid #3498db; border-radius: 3px; color: #333;");
    layout->addWidget(summaryLabel);
    
    // 详细结果列表
    QLabel* detailLabel = new QLabel("Detailed Results:", resultDialog);
    detailLabel->setStyleSheet("font-weight: bold; margin-top: 15px; margin-bottom: 8px;");
    layout->addWidget(detailLabel);
    
    QListWidget* resultList = new QListWidget(resultDialog);
    resultList->setStyleSheet("QListWidget { border: 1px solid #ccc; }");
    
    for (const auto& result : summary.details) {
        QString itemText = formatResultItem(result);
        QListWidgetItem* item = new QListWidgetItem(itemText);
        
        // 根据状态设置颜色
        switch (result.finalStatus) {
            case OrganizeResult::Success:
                item->setForeground(Qt::darkGreen);
                break;
            case OrganizeResult::SkippedConflict:
                item->setForeground(QColor(200, 120, 0));  // 深橙色
                break;
            case OrganizeResult::Failed:
                item->setForeground(Qt::darkRed);
                break;
            case OrganizeResult::NoRule:
                item->setForeground(QColor(150, 150, 150));  // 灰色
                break;
        }
        
        resultList->addItem(item);
    }
    
    // 如果没有详细结果，显示占位符
    if (summary.details.empty()) {
        QListWidgetItem* item = new QListWidgetItem("(No file details available)");
        item->setForeground(Qt::gray);
        resultList->addItem(item);
    }
    
    layout->addWidget(resultList);
    
    // 关闭按钮
    QPushButton* closeBtn = new QPushButton("Close", resultDialog);
    connect(closeBtn, &QPushButton::clicked, resultDialog, &QDialog::accept);
    layout->addWidget(closeBtn);
    
    // 显示对话框
    resultDialog->exec();
    
    // 删除对话框
    delete resultDialog;
}

void MainWindow::triggerOrganizeFromTray() {
    Logger::getInstance().info("MainWindow: One-Click Organize triggered from tray menu");
    
    // 显示主窗口
    if (!isVisible()) {
        showNormal();
        activateWindow();
        raise();
    }
    
    onOrganizeClicked();
}

void MainWindow::showSettingsDialog() {
    Logger::getInstance().info("MainWindow: Settings dialog requested");
    
    if (!m_settingsDialog && m_fileOrganizer && m_configManager) {
        m_settingsDialog = new SettingsDialog(
            m_configManager,
            m_fileOrganizer,
            this
        );
    }
    
    if (m_settingsDialog) {
        m_settingsDialog->exec();
    }
}

void MainWindow::hideToTray() {
    Logger::getInstance().info("MainWindow: Hiding to tray");
    
    // 隐藏主窗口
    this->hide();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    Logger::getInstance().info("MainWindow: Close event triggered");
    
    // 检查是否有托盘管理器
    if (m_trayManager && m_trayManager->isSystemTrayAvailable()) {
        // 隐藏到托盘而不是关闭
        Logger::getInstance().info("MainWindow: Minimizing to tray");
        hideToTray();
        event->ignore();
    } else {
        // 没有托盘支持，直接退出
        Logger::getInstance().info("MainWindow: No tray support available, exiting application");
        event->accept();
    }
}

} // namespace ccdesk::ui
