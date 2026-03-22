#include "main_window.h"
#include "settings_dialog.h"
#include "../core/file_organizer.h"
#include "../core/organize_result.h"
#include "../core/logger.h"
#include "../core/tray_manager.h"
#include "../core/config_manager.h"
#include "../core/desktop_auto_arrange_service.h"
#include "../core/desktop_layout_planner.h"
#include "../core/desktop_icon_accessor.h"
#include "../core/desktop_icon_writer.h"
#include "../core/desktop_arrange_rule_engine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QCloseEvent>
#include <QGroupBox>
#include <QScrollArea>
#include <QTimer>

using namespace ccdesk::core;

namespace ccdesk::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_fileOrganizer(nullptr)
    , m_trayManager(nullptr)
    , m_configManager(nullptr)
    , m_autoArrangeService(nullptr)
    , m_fileTotalLabel(nullptr)
    , m_fileCategorizedLabel(nullptr)
    , m_fileStatsLabel(nullptr)
    , m_iconTotalLabel(nullptr)
    , m_iconCategorizedLabel(nullptr)
    , m_iconPlannedLabel(nullptr)
    , m_iconStatsLabel(nullptr)
    , m_fileResultList(nullptr)
    , m_layoutResultList(nullptr)
    , m_logSummary(nullptr)
    , m_generateFilePlanBtn(nullptr)
    , m_refreshFileResultsBtn(nullptr)
    , m_generateLayoutPlanBtn(nullptr)
    , m_refreshLayoutResultsBtn(nullptr)
    , m_settingsBtn(nullptr)
    , m_logUpdateTimer(nullptr)
{
    setWindowTitle("CCDesk - 桌面收纳盒规划器");
    setGeometry(100, 100, 900, 700);
    
    initializeUI();
    
    // 设置定时器每3秒更新一次日志摘要
    m_logUpdateTimer = new QTimer(this);
    connect(m_logUpdateTimer, &QTimer::timeout, this, &MainWindow::onUpdateLogSummary);
    m_logUpdateTimer->start(3000);
    
    Logger::getInstance().info("MainWindow: 主窗口初始化完成");
}

MainWindow::~MainWindow() {
    if (m_logUpdateTimer) {
        m_logUpdateTimer->stop();
    }
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

void MainWindow::setDesktopAutoArrangeService(DesktopAutoArrangeService* service) {
    m_autoArrangeService = service;
    Logger::getInstance().info("MainWindow: DesktopAutoArrangeService set");
}

void MainWindow::initializeUI() {
    // 创建中央组件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 1. 状态栏区域
    QWidget* statusBar = createStatusBar();
    mainLayout->addWidget(statusBar);
    
    // 2. 文件整理模块
    QWidget* fileSection = createFileOrganizeSection();
    mainLayout->addWidget(fileSection, 1);  // stretch factor 1
    
    // 3. 桌面布局模块
    QWidget* layoutSection = createLayoutSection();
    mainLayout->addWidget(layoutSection, 1);  // stretch factor 1
    
    // 4. 日志摘要区域
    QWidget* logSection = createLogSection();
    mainLayout->addWidget(logSection);
}

QWidget* MainWindow::createStatusBar() {
    QFrame* frame = new QFrame(this);
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    frame->setStyleSheet("background-color: #f5f5f5; border-radius: 5px; padding: 5px;");
    
    QHBoxLayout* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(10, 5, 10, 5);
    
    QLabel* titleLabel = new QLabel(u8"CCDesk - 桌面图标自动整理工具", frame);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50;");
    layout->addWidget(titleLabel);

    layout->addStretch();

    QLabel* statusLabel = new QLabel(u8"就绪 - 支持规划与真实执行", frame);
    statusLabel->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    layout->addWidget(statusLabel);
    
    return frame;
}

QWidget* MainWindow::createFileOrganizeSection() {
    QGroupBox* group = new QGroupBox("文件整理规划", this);
    group->setStyleSheet("QGroupBox { font-weight: bold; color: #2c3e50; margin-top: 10px; }"
                         "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(group);
    
    // 顶部按钮行
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_generateFilePlanBtn = new QPushButton("生成文件分类规划", group);
    m_generateFilePlanBtn->setMinimumWidth(150);
    connect(m_generateFilePlanBtn, &QPushButton::clicked, this, &MainWindow::onGenerateFileOrganizePlan);
    buttonLayout->addWidget(m_generateFilePlanBtn);
    
    m_refreshFileResultsBtn = new QPushButton("刷新结果", group);
    m_refreshFileResultsBtn->setMinimumWidth(100);
    connect(m_refreshFileResultsBtn, &QPushButton::clicked, this, &MainWindow::onRefreshFileResults);
    buttonLayout->addWidget(m_refreshFileResultsBtn);
    
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    // 统计信息区域
    QHBoxLayout* statsLayout = new QHBoxLayout();
    m_fileTotalLabel = new QLabel("总文件数: 0", group);
    m_fileTotalLabel->setStyleSheet("color: #2c3e50; font-weight: bold;");
    statsLayout->addWidget(m_fileTotalLabel);
    
    m_fileCategorizedLabel = new QLabel("已分类: 0", group);
    m_fileCategorizedLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
    statsLayout->addWidget(m_fileCategorizedLabel);
    
    statsLayout->addStretch();
    
    m_fileStatsLabel = new QLabel("分类统计: 无", group);
    m_fileStatsLabel->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    statsLayout->addWidget(m_fileStatsLabel);
    
    mainLayout->addLayout(statsLayout);
    
    // 结果显示区域
    m_fileResultList = new QTextEdit(group);
    m_fileResultList->setReadOnly(true);
    m_fileResultList->setMaximumHeight(150);
    m_fileResultList->setStyleSheet("QTextEdit { border: 1px solid #bdc3c7; border-radius: 3px; "
                                    "background-color: #ffffff; font-family: 'Courier New', monospace; font-size: 10px; }");
    m_fileResultList->setPlaceholderText(u8"点击\"生成文件分类规划\"按钮开始分析...");
    mainLayout->addWidget(m_fileResultList);
    
    return group;
}

QWidget* MainWindow::createLayoutSection() {
    QGroupBox* group = new QGroupBox("桌面布局规划与执行", this);
    group->setStyleSheet("QGroupBox { font-weight: bold; color: #2c3e50; margin-top: 10px; }"
                         "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(group);

    // 顶部按钮行
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    // 规划按钮
    m_generateLayoutPlanBtn = new QPushButton("生成桌面布局规划", group);
    m_generateLayoutPlanBtn->setMinimumWidth(150);
    connect(m_generateLayoutPlanBtn, &QPushButton::clicked, this, &MainWindow::onGenerateLayoutPlan);
    buttonLayout->addWidget(m_generateLayoutPlanBtn);

    // 执行按钮（用不同颜色区分）
    m_executeArrangeBtn = new QPushButton(u8"执行桌面自动整理", group);
    m_executeArrangeBtn->setMinimumWidth(150);
    m_executeArrangeBtn->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #c0392b; }"
    );
    connect(m_executeArrangeBtn, &QPushButton::clicked, this, &MainWindow::onExecuteDesktopArrange);
    buttonLayout->addWidget(m_executeArrangeBtn);

    m_refreshLayoutResultsBtn = new QPushButton("刷新结果", group);
    m_refreshLayoutResultsBtn->setMinimumWidth(100);
    connect(m_refreshLayoutResultsBtn, &QPushButton::clicked, this, &MainWindow::onRefreshLayoutResults);
    buttonLayout->addWidget(m_refreshLayoutResultsBtn);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // 统计信息区域
    QHBoxLayout* statsLayout = new QHBoxLayout();
    m_iconTotalLabel = new QLabel("总图标数: 0", group);
    m_iconTotalLabel->setStyleSheet("color: #2c3e50; font-weight: bold;");
    statsLayout->addWidget(m_iconTotalLabel);

    m_iconCategorizedLabel = new QLabel("已分类: 0", group);
    m_iconCategorizedLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
    statsLayout->addWidget(m_iconCategorizedLabel);

    m_iconPlannedLabel = new QLabel("已规划: 0", group);
    m_iconPlannedLabel->setStyleSheet("color: #2980b9; font-weight: bold;");
    statsLayout->addWidget(m_iconPlannedLabel);

    // 执行统计标签（新增）
    QLabel* arrangeMovedLabel = new QLabel("移动成功: 0", group);
    arrangeMovedLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
    m_iconStatsLabel = arrangeMovedLabel;  // 复用现有变量
    statsLayout->addWidget(arrangeMovedLabel);

    statsLayout->addStretch();

    QLabel* executeStatsLabel = new QLabel("移动失败: 0", group);
    executeStatsLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
    executeStatsLabel->setObjectName("executeFailedLabel");  // 用于后续访问
    statsLayout->addWidget(executeStatsLabel);

    mainLayout->addLayout(statsLayout);

    // 结果显示区域
    m_layoutResultList = new QTextEdit(group);
    m_layoutResultList->setReadOnly(true);
    m_layoutResultList->setMaximumHeight(200);
    m_layoutResultList->setStyleSheet("QTextEdit { border: 1px solid #bdc3c7; border-radius: 3px; "
                                     "background-color: #ffffff; font-family: 'Courier New', monospace; font-size: 10px; }");
    m_layoutResultList->setPlaceholderText(u8"点击\"生成桌面布局规划\"按钮开始分析，或点击\"执行桌面自动整理\"按钮真实整理桌面...");
    mainLayout->addWidget(m_layoutResultList);

    return group;
}

QWidget* MainWindow::createLogSection() {
    QGroupBox* group = new QGroupBox("日志摘要", this);
    group->setStyleSheet("QGroupBox { font-weight: bold; color: #2c3e50; margin-top: 10px; }"
                         "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }");
    
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    m_logSummary = new QTextEdit(group);
    m_logSummary->setReadOnly(true);
    m_logSummary->setMaximumHeight(100);
    m_logSummary->setStyleSheet("QTextEdit { border: 1px solid #bdc3c7; border-radius: 3px; "
                                "background-color: #2c3e50; color: #ecf0f1; font-family: 'Courier New', monospace; font-size: 9px; }");
    m_logSummary->setPlaceholderText("日志信息将在此显示...");
    layout->addWidget(m_logSummary);
    
    return group;
}

void MainWindow::onGenerateFileOrganizePlan() {
    if (!m_fileOrganizer) {
        QMessageBox::warning(this, "错误", "文件整理器未初始化");
        Logger::getInstance().error("MainWindow: FileOrganizer is null");
        return;
    }
    
    Logger::getInstance().info("MainWindow: 用户点击了'生成文件分类规划'按钮");
    m_generateFilePlanBtn->setEnabled(false);
    m_generateFilePlanBtn->setText("正在生成...");
    
    // 重置取消标志
    m_fileOrganizer->cancelOrganize();
    
    // 生成整理规划（不移动文件，只分析分类）
    m_lastFilePlan = std::make_unique<ccdesk::core::OrganizePlan>(
        m_fileOrganizer->generateOrganizePlan()
    );
    
    // 更新状态
    m_state.fileTotalItems = m_lastFilePlan->totalFiles;
    m_state.fileCategorizedItems = 0;
    
    // 计算已分类文件数
    for (const auto& item : m_lastFilePlan->items) {
        if (item.status == OrganizePreviewItem::Movable) {
            m_state.fileCategorizedItems++;
            QString category = QString::fromStdString(item.categoryName);
            m_state.fileCategoryCounts[category]++;
        }
    }
    
    // 更新UI
    updateStatusBar(m_state);
    showFileOrganizeResults(m_state);
    
    m_generateFilePlanBtn->setEnabled(true);
    m_generateFilePlanBtn->setText("生成文件分类规划");
    
    Logger::getInstance().info("MainWindow: 文件分类规划生成完成，总文件数: " +
                             std::to_string(m_lastFilePlan->totalFiles));
}

void MainWindow::onRefreshFileResults() {
    if (m_lastFilePlan && !m_lastFilePlan->items.empty()) {
        showFileOrganizeResults(m_state);
        Logger::getInstance().info("MainWindow: 刷新文件整理结果显示");
    } else {
        QMessageBox::information(this, "提示", "暂无文件整理结果，请先生成规划");
    }
}

void MainWindow::onGenerateLayoutPlan() {
    if (!m_autoArrangeService) {
        QMessageBox::warning(this, "错误", "桌面整理服务未初始化");
        Logger::getInstance().error("MainWindow: DesktopAutoArrangeService is null");
        return;
    }
    
    Logger::getInstance().info("MainWindow: 用户点击了'生成桌面布局规划'按钮");
    m_generateLayoutPlanBtn->setEnabled(false);
    m_generateLayoutPlanBtn->setText("正在生成...");
    
    // 生成桌面布局规划（不执行写回）
    m_lastLayoutPlan = std::make_unique<ccdesk::core::LayoutPlanResult>(
        m_autoArrangeService->generateLayoutPlan()
    );
    
    // 更新状态
    m_state.iconTotalCount = static_cast<int>(m_lastLayoutPlan->totalIcons);
    m_state.iconCategorizedCount = static_cast<int>(m_lastLayoutPlan->categorizedIcons);
    m_state.iconPlannedCount = static_cast<int>(m_lastLayoutPlan->plannedIcons);
    
    // 如果规划失败，显示错误
    if (!m_lastLayoutPlan->success()) {
        m_state.lastOperation = "生成桌面布局规划";
        m_state.lastOperationResult = "失败";
        m_state.lastOperationDetail = QString::fromStdString(m_lastLayoutPlan->errorMessage);
        QMessageBox::warning(this, "规划失败",
                           QString::fromStdString(m_lastLayoutPlan->errorMessage));
    } else {
        // 获取布局规划器
        DesktopLayoutPlanner* planner = m_autoArrangeService->getLayoutPlanner();
        if (planner) {
            // 获取图标访问器
            DesktopIconAccessor* accessor = m_autoArrangeService->getIconAccessor();
            if (accessor) {
                // 读取桌面图标
                DesktopIconSnapshot snapshot = accessor->readDesktopIcons();
                if (snapshot.success()) {
                    // 构建身份信息并分类
                    std::vector<ArrangeableDesktopIcon> arrangeableIcons;
                    DesktopArrangeRuleEngine* ruleEngine = m_autoArrangeService->getRuleEngine();
                    
                    for (const auto& icon : snapshot.icons) {
                        ArrangeableDesktopIcon arrangeableIcon;
                        arrangeableIcon.identity.displayName = icon.displayName;
                        arrangeableIcon.identity.parsingName = icon.parsingName;
                        arrangeableIcon.identity.isFileSystemItem = icon.isFileSystemItem;
                        arrangeableIcon.currentPosition = icon.position;
                        
                        if (ruleEngine) {
                            IconCategory category = ruleEngine->classifyIcon(arrangeableIcon.identity);
                            arrangeableIcon.category = ruleEngine->getCategoryName(category);
                            QString categoryStr = QString::fromStdString(arrangeableIcon.category);
                            m_state.iconCategoryCounts[categoryStr]++;
                        }
                        
                        arrangeableIcons.push_back(arrangeableIcon);
                    }
                }
            }
        }
        
        m_state.lastOperation = "生成桌面布局规划";
        m_state.lastOperationResult = "成功";
        m_state.lastOperationDetail = "规划生成完成";
    }
    
    // 更新UI
    updateStatusBar(m_state);
    showLayoutResults(m_state);
    
    m_generateLayoutPlanBtn->setEnabled(true);
    m_generateLayoutPlanBtn->setText("生成桌面布局规划");
    
    Logger::getInstance().info("MainWindow: 桌面布局规划生成完成，总图标数: " +
                             std::to_string(m_lastLayoutPlan->totalIcons));
}

void MainWindow::onRefreshLayoutResults() {
    if (m_lastLayoutPlan && m_lastLayoutPlan->totalIcons > 0) {
        showLayoutResults(m_state);
        Logger::getInstance().info("MainWindow: 刷新桌面布局结果显示");
    } else {
        QMessageBox::information(this, "提示", "暂无桌面布局结果，请先生成规划");
    }
}

void MainWindow::onExecuteDesktopArrange() {
    if (!m_autoArrangeService) {
        QMessageBox::warning(this, "错误", "桌面整理服务未初始化");
        Logger::getInstance().error("MainWindow: DesktopAutoArrangeService is null");
        return;
    }

    // P0-4: 执行前确认弹窗
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        u8"确认执行桌面自动整理",
        u8"是否根据当前规划重新排列桌面图标？\n\n"
        "执行后桌面图标位置将发生变化。\n\n"
        "建议：\n"
        "• 确保桌面图标可以正常访问\n"
        "• 建议先执行\"生成桌面布局规划\"预览效果\n"
        "• 如效果不满意，可稍后添加\"恢复原布局\"功能\n\n"
        "是否继续执行？",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::No) {
        Logger::getInstance().info("MainWindow: 用户取消了桌面自动整理执行");
        return;
    }

    Logger::getInstance().info("MainWindow: 用户确认执行桌面自动整理");
    m_executeArrangeBtn->setEnabled(false);
    m_executeArrangeBtn->setText(u8"正在执行...");

    // 执行前基础校验
    // 先尝试读取桌面图标，确保可以访问
    DesktopIconAccessor* accessor = m_autoArrangeService->getIconAccessor();
    if (!accessor) {
        QMessageBox::critical(this, "错误", "无法访问桌面图标读取器");
        m_executeArrangeBtn->setEnabled(true);
        m_executeArrangeBtn->setText(u8"执行桌面自动整理");
        return;
    }

    DesktopIconSnapshot snapshot = accessor->readDesktopIcons();
    if (!snapshot.success()) {
        QMessageBox::critical(
            this,
            "执行失败",
            u8"无法读取桌面图标，请确保：\n"
            "• 桌面可正常访问\n"
            "• 没有其他程序占用桌面\n\n"
            "错误信息: " + QString::fromStdString(snapshot.errorMessage)
        );
        m_executeArrangeBtn->setEnabled(true);
        m_executeArrangeBtn->setText(u8"执行桌面自动整理");
        return;
    }

    if (snapshot.icons.empty()) {
        QMessageBox::warning(this, "执行失败", "桌面上没有可整理的图标");
        m_executeArrangeBtn->setEnabled(true);
        m_executeArrangeBtn->setText(u8"执行桌面自动整理");
        return;
    }

    // 执行真实自动整理
    Logger::getInstance().info("MainWindow: 开始执行桌面自动整理");
    ccdesk::core::AutoArrangeResult result = m_autoArrangeService->arrangeDesktop();

    // 更新状态
    m_state.iconTotalCount = static_cast<int>(result.totalIcons);
    m_state.iconCategorizedCount = static_cast<int>(result.categorizedIcons);
    m_state.iconPlannedCount = static_cast<int>(result.plannedIcons);
    m_state.arrangeMovedCount = static_cast<int>(result.movedIcons);
    m_state.arrangeFailedCount = static_cast<int>(result.failedIcons);

    // 转换失败详情
    m_state.arrangeExecutionDetails.clear();
    for (const auto& failure : result.failures) {
        ArrangeExecuteResultItem item;
        item.displayName = QString::fromStdString(failure.displayName);
        item.targetPosition = QString::fromStdString(failure.targetPosition);
        item.status = "失败";
        item.failureReason = QString::fromStdString(failure.errorMessage);
        m_state.arrangeExecutionDetails.push_back(item);
    }

    // 显示执行结果
    QString summaryText = QString::fromStdString(result.getSummaryText());
    m_layoutResultList->setText(summaryText);

    // 更新统计标签
    m_iconTotalLabel->setText(QString("总图标数: %1").arg(result.totalIcons));
    m_iconCategorizedLabel->setText(QString("已分类: %1").arg(result.categorizedIcons));
    m_iconPlannedLabel->setText(QString("已规划: %1").arg(result.plannedIcons));

    // 更新执行统计
    m_iconStatsLabel->setText(QString("移动成功: %1").arg(result.movedIcons));

    // 查找失败统计标签
    QLabel* executeFailedLabel = findChild<QLabel*>("executeFailedLabel");
    if (executeFailedLabel) {
        executeFailedLabel->setText(QString("移动失败: %1").arg(result.failedIcons));
    }

    // 根据结果显示不同的提示
    if (result.success()) {
        m_state.lastOperation = u8"执行桌面自动整理";
        m_state.lastOperationResult = "成功";
        m_state.lastOperationDetail = QString::fromStdString(u8"全部 %1 个图标移动成功").arg(result.movedIcons);

        QMessageBox::information(
            this,
            u8"整理完成",
            QString::fromStdString(result.getSummaryText())
        );

        Logger::getInstance().info("MainWindow: 桌面自动整理全部成功，移动 %zu 个图标", result.movedIcons);
    } else if (result.partialSuccess()) {
        m_state.lastOperation = u8"执行桌面自动整理";
        m_state.lastOperationResult = u8"部分成功";
        m_state.lastOperationDetail = QString::fromStdString(
            u8"移动成功: " + std::to_string(result.movedIcons) +
            u8", 移动失败: " + std::to_string(result.failedIcons)
        );

        QMessageBox::warning(
            this,
            u8"整理部分成功",
            QString::fromStdString(result.getSummaryText())
        );

        Logger::getInstance().warning(
            "MainWindow: 桌面自动整理部分成功，成功: %zu, 失败: %zu",
            result.movedIcons,
            result.failedIcons
        );
    } else {
        m_state.lastOperation = u8"执行桌面自动整理";
        m_state.lastOperationResult = "失败";
        m_state.lastOperationDetail = QString::fromStdString(result.errorMessage);

        QMessageBox::critical(
            this,
            u8"整理失败",
            QString::fromStdString(result.getSummaryText())
        );

        Logger::getInstance().error(
            "MainWindow: 桌面自动整理失败，错误: %s",
            result.errorMessage.c_str()
        );
    }

    // 更新状态栏
    updateStatusBar(m_state);

    m_executeArrangeBtn->setEnabled(true);
    m_executeArrangeBtn->setText(u8"执行桌面自动整理");
    Logger::getInstance().info("MainWindow: 桌面自动整理执行完成");
}

void MainWindow::onShowSettings() {
    Logger::getInstance().info("MainWindow: 请求打开设置对话框");
    
    // TODO: 实现设置对话框
    QMessageBox::information(this, "提示", "设置对话框功能开发中...");
}

void MainWindow::onUpdateLogSummary() {
    updateLogSummary();
}

void MainWindow::updateStatusBar(const MainWindowState& state) {
    // 更新文件整理统计
    m_fileTotalLabel->setText(QString("总文件数: %1").arg(state.fileTotalItems));
    m_fileCategorizedLabel->setText(QString("已分类: %1").arg(state.fileCategorizedItems));
    
    // 更新文件分类统计
    QString fileStatsText;
    if (!state.fileCategoryCounts.empty()) {
        QStringList stats;
        for (const auto& pair : state.fileCategoryCounts) {
            stats << QString("%1:%2").arg(pair.first).arg(pair.second);
        }
        fileStatsText = "分类统计: " + stats.join(", ");
    } else {
        fileStatsText = "分类统计: 无";
    }
    m_fileStatsLabel->setText(fileStatsText);
    
    // 更新桌面布局统计
    m_iconTotalLabel->setText(QString("总图标数: %1").arg(state.iconTotalCount));
    m_iconCategorizedLabel->setText(QString("已分类: %1").arg(state.iconCategorizedCount));
    m_iconPlannedLabel->setText(QString("已规划: %1").arg(state.iconPlannedCount));
    
    // 更新图标分类统计
    QString iconStatsText;
    if (!state.iconCategoryCounts.empty()) {
        QStringList stats;
        for (const auto& pair : state.iconCategoryCounts) {
            stats << QString("%1:%2").arg(pair.first).arg(pair.second);
        }
        iconStatsText = "分类统计: " + stats.join(", ");
    } else {
        iconStatsText = "分类统计: 无";
    }
    m_iconStatsLabel->setText(iconStatsText);
}

void MainWindow::showFileOrganizeResults(const MainWindowState& state) {
    if (!m_lastFilePlan || m_lastFilePlan->items.empty()) {
        m_fileResultList->setText("没有文件分类结果");
        return;
    }
    
    QString resultText;
    resultText += QString("=== 文件分类规划结果 ===\n");
    resultText += QString("桌面路径: %1\n").arg(QString::fromStdString(m_lastFilePlan->desktopPath));
    resultText += QString("总文件数: %1 | 已分类: %2\n\n")
                      .arg(m_lastFilePlan->totalFiles)
                      .arg(state.fileCategorizedItems);
    
    resultText += QString("【分类统计】\n");
    std::vector<FileCategory> fixedCategories = DesktopLayoutManager::getFixedCategories();
    for (FileCategory cat : fixedCategories) {
        int count = m_lastFilePlan->getCategoryCount(cat);
        if (count > 0) {
            QString categoryName = QString::fromStdString(DesktopLayoutManager::getCategoryName(cat));
            resultText += QString("  %1: %2 个文件\n").arg(categoryName).arg(count);
        }
    }
    
    resultText += QString("\n【文件明细】\n");
    for (size_t i = 0; i < m_lastFilePlan->items.size(); ++i) {
        const auto& item = m_lastFilePlan->items[i];
        OrganizeResultItem resultItem = formatOrganizeResultItem(item);
        
        QString statusIcon;
        if (item.status == OrganizePreviewItem::Movable) {
            statusIcon = "✓";
        } else if (item.status == OrganizePreviewItem::Conflict) {
            statusIcon = "⊗";
        } else {
            statusIcon = "○";
        }
        
        resultText += QString("%1 [%2] -> %3 (%4)\n")
                          .arg(statusIcon)
                          .arg(resultItem.fileName)
                          .arg(resultItem.category)
                          .arg(resultItem.status);
    }
    
    resultText += QString("\n注意: 这是规划模式，不会移动任何文件\n");
    m_fileResultList->setText(resultText);
}

void MainWindow::showLayoutResults(const MainWindowState& state) {
    QString resultText;
    resultText += QString("=== 桌面布局规划结果 ===\n");
    
    if (!m_lastLayoutPlan || !m_lastLayoutPlan->success()) {
        std::string errorMsg = m_lastLayoutPlan ? m_lastLayoutPlan->errorMessage : "未生成规划";
        resultText += QString("规划失败: %1\n").arg(QString::fromStdString(errorMsg));
        m_layoutResultList->setText(resultText);
        return;
    }
    
    resultText += QString("总图标数: %1 | 已分类: %2 | 已规划: %3\n\n")
                      .arg(m_lastLayoutPlan->totalIcons)
                      .arg(m_lastLayoutPlan->categorizedIcons)
                      .arg(m_lastLayoutPlan->plannedIcons);
    
    resultText += QString("【分类统计】\n");
    if (!state.iconCategoryCounts.empty()) {
        for (const auto& pair : state.iconCategoryCounts) {
            resultText += QString("  %1: %2 个图标\n").arg(pair.first).arg(pair.second);
        }
    }
    
    resultText += QString("\n注意: 这是规划模式，不会修改桌面图标位置\n");
    m_layoutResultList->setText(resultText);
}

void MainWindow::updateLogSummary() {
    // 读取日志文件的最后几行
    std::string logFile = "ccdesk.log";
    std::string summary = "";
    
    try {
        std::ifstream file(logFile, std::ios::ate);
        if (file.is_open()) {
            std::streampos fileSize = file.tellg();
            const int maxBytes = 2000;  // 读取最后2KB
            int readSize = static_cast<int>(std::min(static_cast<std::streampos>(maxBytes), fileSize));
            
            file.seekg(-readSize, std::ios::end);
            
            std::vector<char> buffer(readSize);
            file.read(buffer.data(), readSize);
            file.close();
            
            summary = std::string(buffer.data(), readSize);
            
            // 只保留最后20行
            std::vector<std::string> lines;
            std::istringstream iss(summary);
            std::string line;
            while (std::getline(iss, line)) {
                lines.push_back(line);
            }
            
            if (lines.size() > 20) {
                lines.erase(lines.begin(), lines.begin() + (lines.size() - 20));
            }
            
            summary = "";
            for (const auto& l : lines) {
                summary += l + "\n";
            }
        }
    } catch (...) {
        summary = "无法读取日志文件";
    }
    
    m_logSummary->setText(QString::fromStdString(summary));
}

OrganizeResultItem MainWindow::formatOrganizeResultItem(const OrganizePreviewItem& item) const {
    OrganizeResultItem resultItem;
    resultItem.fileName = QString::fromUtf8(item.fileName.c_str());
    resultItem.category = QString::fromUtf8(item.categoryName.c_str());
    
    switch (item.status) {
        case OrganizePreviewItem::Movable:
            resultItem.status = "可归类";
            resultItem.statusDetail = "建议归类到" + resultItem.category;
            break;
        case OrganizePreviewItem::Conflict:
            resultItem.status = "冲突";
            resultItem.statusDetail = "分类建议冲突";
            break;
        case OrganizePreviewItem::NoRule:
            resultItem.status = "未匹配";
            resultItem.statusDetail = "未找到匹配的分类规则";
            break;
    }
    
    return resultItem;
}

LayoutResultItem MainWindow::formatLayoutResultItem(const LayoutPlanResult& result, size_t index) const {
    LayoutResultItem resultItem;
    resultItem.iconName = QString("图标 #%1").arg(static_cast<int>(index) + 1);
    resultItem.category = QString::fromStdString("");  // TODO: 从规划结果中获取
    resultItem.targetPosition = QString("(0, 0)");     // TODO: 从规划结果中获取
    resultItem.status = "已规划";
    
    return resultItem;
}

QString MainWindow::getCategoryDisplayName(const QString& category) const {
    // 分类名称已经是中文，直接返回
    return category;
}

void MainWindow::triggerOrganizeFromTray() {
    Logger::getInstance().info("MainWindow: 从托盘菜单触发文件分类分析");
    
    // 显示主窗口
    if (!isVisible()) {
        showNormal();
        activateWindow();
        raise();
    }
    
    onGenerateFileOrganizePlan();
}

void MainWindow::triggerLayoutFromTray() {
    Logger::getInstance().info("MainWindow: 从托盘菜单触发桌面布局分析");

    // 显示主窗口
    if (!isVisible()) {
        showNormal();
        activateWindow();
        raise();
    }

    onGenerateLayoutPlan();
}

void MainWindow::triggerExecuteArrangeFromTray() {
    Logger::getInstance().info("MainWindow: 从托盘菜单触发桌面自动整理执行");

    // 显示主窗口
    if (!isVisible()) {
        showNormal();
        activateWindow();
        raise();
    }

    onExecuteDesktopArrange();
}

void MainWindow::showSettingsDialog() {
    onShowSettings();
}

void MainWindow::hideToTray() {
    Logger::getInstance().info("MainWindow: 隐藏到托盘");
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
