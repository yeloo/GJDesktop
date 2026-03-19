#include "partition_widget.h"
#include "../core/logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>
#include <filesystem>

namespace fs = std::filesystem;

namespace ccdesk::ui {

PartitionWidget::PartitionWidget(const std::string& partitionId,
                               const std::string& partitionName,
                               const std::string& targetPath,
                               QWidget* parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_fileList(nullptr)
    , m_closeBtn(nullptr)
    , m_partitionId(partitionId)
    , m_partitionName(partitionName)
    , m_targetPath(targetPath)
    , m_isDragging(false)
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setGeometry(100, 100, 300, 400);
    
    initializeUI();
    refreshFileList();
    
    ccdesk::core::Logger::getInstance().info("PartitionWidget: Constructed - " + partitionName);
}

PartitionWidget::~PartitionWidget() {
    ccdesk::core::Logger::getInstance().info("PartitionWidget: Destroyed - " + m_partitionName);
}

void PartitionWidget::initializeUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 标题栏（可拖动）
    QHBoxLayout* titleLayout = new QHBoxLayout();
    
    m_titleLabel = new QLabel(QString::fromStdString(m_partitionName), this);
    m_titleLabel->setStyleSheet(
        "background-color: #3498db; color: white; padding: 5px; font-weight: bold;"
    );
    titleLayout->addWidget(m_titleLabel);
    
    m_closeBtn = new QPushButton("×", this);
    m_closeBtn->setMaximumWidth(30);
    m_closeBtn->setStyleSheet("background-color: #e74c3c; color: white;");
    connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(m_closeBtn);
    
    mainLayout->addLayout(titleLayout);
    
    // 文件列表
    m_fileList = new QListWidget(this);
    m_fileList->setStyleSheet("QListWidget { border: 1px solid #ccc; }");
    mainLayout->addWidget(m_fileList);
}

void PartitionWidget::refreshFileList() {
    m_fileList->clear();
    
    // 检查路径是否存在
    if (!fs::exists(m_targetPath)) {
        QListWidgetItem* item = new QListWidgetItem("⚠ 目录不存在");
        item->setForeground(Qt::red);
        m_fileList->addItem(item);
        ccdesk::core::Logger::getInstance().warning(
            "PartitionWidget: 目标路径不存在: " + m_targetPath
        );
        return;
    }
    
    try {
        int fileCount = 0;
        for (const auto& entry : fs::directory_iterator(m_targetPath)) {
            if (entry.is_regular_file()) {
                std::string fileName = entry.path().filename().string();
                QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(fileName));
                m_fileList->addItem(item);
                fileCount++;
            }
        }
        
        // 若没有文件，显示提示
        if (fileCount == 0) {
            QListWidgetItem* item = new QListWidgetItem("(空)");
            item->setForeground(Qt::gray);
            m_fileList->addItem(item);
        }
        
        ccdesk::core::Logger::getInstance().info(
            "PartitionWidget: Refreshed " + m_partitionName + ", " + std::to_string(fileCount) + " files"
        );
    } catch (const std::exception& e) {
        QListWidgetItem* item = new QListWidgetItem("✗ 读取目录时出错");
        item->setForeground(Qt::red);
        m_fileList->addItem(item);
        
        ccdesk::core::Logger::getInstance().error(
            "PartitionWidget: 读取目录时出错: " + std::string(e.what())
        );
    }
}

void PartitionWidget::mousePressEvent(QMouseEvent* event) {
    if (event->y() < 30) {  // 标题栏区域
        m_isDragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void PartitionWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        move(event->globalPosition().toPoint() - m_dragStartPos);
    }
}

} // namespace ccdesk::ui
