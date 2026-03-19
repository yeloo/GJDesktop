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
#include <iomanip>

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
    , m_isOrganizing(false)
{
    setWindowTitle("CCDesk - 桌面整理工具");
    setGeometry(100, 100, 600, 400);
    
    initializeUI();
    
    Logger::getInstance().info("MainWindow: 主窗口初始化完成");
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
    QLabel* titleLabel = new QLabel("桌面整理工具", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // 功能按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // 一键整理/取消整理按钮
    m_organizeBtn = new QPushButton("一键整理", this);
    connect(m_organizeBtn, &QPushButton::clicked, this, &MainWindow::onOrganizeClicked);
    buttonLayout->addWidget(m_organizeBtn);
    
    // 分区管理按钮
    m_partitionBtn = new QPushButton("分区管理", this);
    buttonLayout->addWidget(m_partitionBtn);
    
    // 设置按钮
    m_settingsBtn = new QPushButton("设置", this);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettingsDialog);
    buttonLayout->addWidget(m_settingsBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // 状态标签
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("color: gray;");
    mainLayout->addWidget(m_statusLabel);
    
    // 弹簧（填充剩余空间）
    mainLayout->addStretch();
}

void MainWindow::onOrganizeClicked() {
    // 如果正在整理中，则切换为取消操作
    if (m_isOrganizing) {
        onCancelOrganizeClicked();
        return;
    }
    
    if (!m_fileOrganizer) {
        QMessageBox::warning(this, "错误", "文件整理器未初始化");
        Logger::getInstance().error("MainWindow: FileOrganizer is null");
        return;
    }
    
    Logger::getInstance().info("MainWindow: 用户点击了'一键整理'按钮");
    m_statusLabel->setText("正在扫描桌面文件...");
    
    // 生成预览
    auto previewItems = m_fileOrganizer->generatePreview();
    
    if (previewItems.empty()) {
        Logger::getInstance().info("MainWindow: 桌面上未找到需要整理的文件");
        QMessageBox::information(this, "没有文件", 
            "桌面上未找到需要整理的文件");
        m_statusLabel->setText("就绪 - 没有文件需要整理");
        return;
    }
    
    // 显示预览对话框
    showPreviewDialog(previewItems);
}

void MainWindow::onCancelOrganizeClicked() {
    if (!m_isOrganizing || !m_fileOrganizer) {
        return;
    }
    
    Logger::getInstance().info("MainWindow: 用户请求取消整理");
    m_statusLabel->setText("正在取消整理...");
    
    // 请求取消整理
    m_fileOrganizer->cancelOrganize();
}

void MainWindow::showPreviewDialog(
    const std::vector<OrganizePreviewItem>& items) {
    
    // 如果对话框已存在，先销毁
    if (m_previewDialog) {
        delete m_previewDialog;
    }
    
    // 创建预览对话框
    m_previewDialog = new QDialog(this, Qt::Dialog | Qt::WindowCloseButtonHint);
    m_previewDialog->setWindowTitle("整理预览");
    m_previewDialog->setGeometry(200, 150, 700, 500);
    
    QVBoxLayout* dialogLayout = new QVBoxLayout(m_previewDialog);
    
    // 标题
    QLabel* titleLabel = new QLabel("预览 - 准备整理以下文件：", m_previewDialog);
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
    ss << "总计: " << items.size() << " 个文件 | "
       << "可移动: " << movableCount << " | "
       << "冲突: " << conflictCount << " | "
       << "无规则: " << noRuleCount;
    
    QLabel* statsLabel = new QLabel(QString::fromStdString(ss.str()), m_previewDialog);
    statsLabel->setStyleSheet("background-color: #fffacd; padding: 8px; border: 1px solid #f0e68c; "
                             "color: #333; font-size: 11px; border-radius: 3px;");
    dialogLayout->addWidget(statsLabel);
    
    // 按钮布局
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    m_confirmBtn = new QPushButton("确认执行", m_previewDialog);
    m_confirmBtn->setMinimumWidth(120);
    connect(m_confirmBtn, &QPushButton::clicked, this, &MainWindow::onPreviewConfirmed);
    btnLayout->addWidget(m_confirmBtn);
    
    m_cancelBtn = new QPushButton("取消", m_previewDialog);
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
        QMessageBox::warning(this, "错误", "文件整理器未初始化");
        Logger::getInstance().error("MainWindow: FileOrganizer is null during confirm");
        return;
    }
    
    Logger::getInstance().info("MainWindow: 用户确认执行，开始正式整理");
    
    // 设置整理状态
    m_isOrganizing = true;
    m_organizeBtn->setText("取消整理");
    m_organizeBtn->setStyleSheet("background-color: #ff9999;");  // 红色背景提示取消
    
    m_statusLabel->setText("正在执行整理...");
    
    // 关闭预览对话框
    if (m_previewDialog) {
        m_previewDialog->close();
    }
    
    // 执行真实整理（同步阻塞）
    OrganizeSummary summary = m_fileOrganizer->executeOrganize();
    
    // 重置整理状态
    m_isOrganizing = false;
    m_organizeBtn->setText("一键整理");
    m_organizeBtn->setStyleSheet("");  // 恢复默认样式
    
    // 根据是否取消显示不同的状态
    if (summary.cancelled) {
        m_statusLabel->setText("整理已取消");
        Logger::getInstance().info("MainWindow: 整理被用户取消，已完成 " + 
                                 std::to_string(summary.getTotalProcessed()) + "/" +
                                 std::to_string(summary.getTotalItems()) + " 个项目");
        
        QMessageBox::information(this, "整理已取消",
            QString("整理操作已被取消。\n已完成 %1/%2 个项目。")
                .arg(summary.getTotalProcessed())
                .arg(summary.getTotalItems()));
    } else {
        // 记录结果摘要
        Logger::getInstance().info("MainWindow: 执行完成 - " +
                                 std::to_string(summary.movedCount) + " 个文件已移动, " +
                                 std::to_string(summary.skippedConflictCount) + " 个文件因冲突跳过, " +
                                 std::to_string(summary.failedCount) + " 个文件失败, " +
                                 std::to_string(summary.noRuleCount) + " 个文件无匹配规则");
        
        // 显示结果报告
        showResultReport(summary);
        
        m_statusLabel->setText("整理完成");
    }
}

void MainWindow::onPreviewCancelled() {
    Logger::getInstance().info("MainWindow: 用户取消了预览");
    m_statusLabel->setText("整理已取消");
    
    if (m_previewDialog) {
        m_previewDialog->close();
    }
}

QString MainWindow::formatPreviewItem(const OrganizePreviewItem& item) const {
    std::stringstream ss;
    ss << "[" << item.fileName << "] -> ";
    
    switch (item.status) {
        case OrganizePreviewItem::Movable:
            ss << "移动到 " << item.targetDirectory << " (规则: " 
               << item.matchedRuleName << ")";
            break;
        case OrganizePreviewItem::Conflict:
            ss << "冲突 - 目标目录已存在同名文件: " << item.targetDirectory;
            break;
        case OrganizePreviewItem::NoRule:
            ss << "无规则 - 没有匹配的整理规则";
            break;
    }
    
    return QString::fromStdString(ss.str());
}

QString MainWindow::formatResultItem(const OrganizeResult& result) const {
    std::stringstream ss;
    ss << "[" << result.fileName << "] ";
    
    switch (result.finalStatus) {
        case OrganizeResult::Success:
            ss << "✓ 成功 - 已移动到 " << result.targetPath;
            break;
        case OrganizeResult::SkippedConflict:
            ss << "⊗ 跳过 - 冲突: " << result.message;
            break;
        case OrganizeResult::Failed:
            ss << "✗ 失败 - " << result.message;
            break;
        case OrganizeResult::NoRule:
            ss << "○ 无规则 - " << result.message;
            break;
    }
    
    return QString::fromStdString(ss.str());
}

void MainWindow::showResultReport(const OrganizeSummary& summary) {
    // 创建结果报告对话框
    QDialog* resultDialog = new QDialog(this, Qt::Dialog | Qt::WindowCloseButtonHint);
    
    // 根据是否取消设置不同的标题
    if (summary.cancelled) {
        resultDialog->setWindowTitle("整理取消报告");
    } else {
        resultDialog->setWindowTitle("整理结果报告");
    }
    
    resultDialog->setGeometry(150, 100, 800, 600);
    
    QVBoxLayout* layout = new QVBoxLayout(resultDialog);
    
    // 标题
    QString titleText = summary.cancelled ? "整理取消报告" : "整理执行报告";
    QLabel* titleLabel = new QLabel(titleText, resultDialog);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    layout->addWidget(titleLabel);
    
    // 汇总统计 - 优化显示格式
    std::stringstream summaryText;
    summaryText << "\n";
    
    if (summary.cancelled) {
        summaryText << "  ═════════════ 整理已取消 ═════════════\n";
        summaryText << "  已处理文件数:  " << summary.getTotalProcessed() << " / " << summary.getTotalItems() << "\n";
        summaryText << "  完成进度:      " << std::fixed << std::setprecision(1) 
                   << (summary.getTotalItems() > 0 ? 
                       (100.0 * summary.getTotalProcessed() / summary.getTotalItems()) : 0.0) 
                   << "%\n";
        summaryText << "  ✗ 取消于项目:  " << summary.cancelledAtItem << "\n";
    } else {
        summaryText << "  ═════════════ 汇总 ═════════════\n";
        summaryText << "  总处理文件数:  " << summary.getTotalProcessed() << "\n";
    }
    
    summaryText << "  ✓ 成功移动:    " << summary.movedCount << "\n";
    summaryText << "  ⊗ 冲突跳过:    " << summary.skippedConflictCount << "\n";
    summaryText << "  ✗ 移动失败:    " << summary.failedCount << "\n";
    summaryText << "  ○ 无匹配规则:  " << summary.noRuleCount << "\n";
    summaryText << "  ════════════════════════════════\n";
    
    QLabel* summaryLabel = new QLabel(QString::fromStdString(summaryText.str()), resultDialog);
    
    // 根据是否取消使用不同的颜色样式
    if (summary.cancelled) {
        summaryLabel->setStyleSheet("background-color: #fffacd; padding: 15px; "
                                   "font-family: 'Courier New', monospace; font-size: 12px; "
                                   "border: 2px solid #f0e68c; border-radius: 3px; color: #333;");
    } else {
        summaryLabel->setStyleSheet("background-color: #e8f4f8; padding: 15px; "
                                   "font-family: 'Courier New', monospace; font-size: 12px; "
                                   "border: 2px solid #3498db; border-radius: 3px; color: #333;");
    }
    
    layout->addWidget(summaryLabel);
    
    // 详细结果列表
    QLabel* detailLabel = new QLabel("详细结果:", resultDialog);
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
        QListWidgetItem* item = new QListWidgetItem("(没有可用的文件详情)");
        item->setForeground(Qt::gray);
        resultList->addItem(item);
    }
    
    layout->addWidget(resultList);
    
    // 关闭按钮
    QPushButton* closeBtn = new QPushButton("关闭", resultDialog);
    connect(closeBtn, &QPushButton::clicked, resultDialog, &QDialog::accept);
    layout->addWidget(closeBtn);
    
    // 显示对话框
    resultDialog->exec();
    
    // 删除对话框
    delete resultDialog;
}

void MainWindow::triggerOrganizeFromTray() {
    Logger::getInstance().info("MainWindow: 从托盘菜单触发一键整理");
    
    // 显示主窗口
    if (!isVisible()) {
        showNormal();
        activateWindow();
        raise();
    }
    
    onOrganizeClicked();
}

void MainWindow::showSettingsDialog() {
    Logger::getInstance().info("MainWindow: 请求打开设置对话框");
    
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
    Logger::getInstance().info("MainWindow: 隐藏到托盘");
    
    // 隐藏主窗口
    this->hide();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    Logger::getInstance().info("MainWindow: 触发了关闭事件");
    
    // 检查是否有托盘管理器
    if (m_trayManager && m_trayManager->isSystemTrayAvailable()) {
        // 隐藏到托盘而不是关闭
        Logger::getInstance().info("MainWindow: 最小化到托盘");
        hideToTray();
        event->ignore();
    } else {
        // 没有托盘支持，直接退出
        Logger::getInstance().info("MainWindow: 托盘不可用，退出应用程序");
        event->accept();
    }
}

} // namespace ccdesk::ui
