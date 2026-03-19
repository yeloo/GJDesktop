#ifndef PARTITION_WIDGET_H
#define PARTITION_WIDGET_H

#include <QWidget>
#include <memory>
#include <string>

class QListWidget;
class QLabel;
class QPushButton;

namespace ccdesk::ui {

class PartitionWidget : public QWidget {
    Q_OBJECT

public:
    explicit PartitionWidget(const std::string& partitionId,
                           const std::string& partitionName,
                           const std::string& targetPath,
                           QWidget* parent = nullptr);
    ~PartitionWidget();
    
    // 获取分区ID
    std::string getPartitionId() const { return m_partitionId; }
    
    // 刷新文件列表
    void refreshFileList();

protected:
    // 鼠标事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    // UI组件
    QLabel* m_titleLabel;
    QListWidget* m_fileList;
    QPushButton* m_closeBtn;
    
    // 分区信息
    std::string m_partitionId;
    std::string m_partitionName;
    std::string m_targetPath;
    
    // 拖动相关
    QPoint m_dragStartPos;
    bool m_isDragging;
    
    // 初始化UI
    void initializeUI();
};

} // namespace ccdesk::ui

#endif // PARTITION_WIDGET_H
