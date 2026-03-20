#ifndef FILE_ORGANIZER_H
#define FILE_ORGANIZER_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <map>
#include <sstream>
#include <filesystem>
#include "organize_result.h"
#include "config_manager.h"
#include "desktop_layout_manager.h"

namespace ccdesk::core {

namespace fs = std::filesystem;

// 预览项（展示给用户看）
struct OrganizePreviewItem {
    std::string sourcePath;
    std::string fileName;
    std::string matchedRuleName;
    std::string categoryName;       // 文件分类名称（新增）
    
    enum PreviewStatus {
        Movable,      // 可以移动（向后兼容）
        Conflict,     // 冲突（目标已存在同名文件）
        NoRule        // 无匹配规则
    };
    
    PreviewStatus status;
    std::string statusMessage;
    
    OrganizePreviewItem() : status(Movable) {}
};

// 整理规划结构体（最小可用规划器）
struct OrganizePlan {
    std::string desktopPath;                     // 桌面路径
    int totalFiles;                             // 总文件数
    std::map<FileCategory, int> categoryCounts; // 各分类文件数统计
    std::vector<OrganizePreviewItem> items;     // 文件明细
    
    OrganizePlan() : totalFiles(0) {}
    
    // 获取分类名称对应的数量
    int getCategoryCount(FileCategory category) const {
        auto it = categoryCounts.find(category);
        return (it != categoryCounts.end()) ? it->second : 0;
    }
    
    // 获取分析摘要文本
    std::string getSummaryText() const {
        std::stringstream ss;
        ss << "桌面分类分析摘要 (规划模式):\n";
        ss << "桌面路径: " << desktopPath << "\n";
        ss << "总项目数: " << totalFiles << " (文件+文件夹)\n";
        ss << "分类统计 (6个固定分类):\n";
        
        std::vector<FileCategory> fixedCategories = DesktopLayoutManager::getFixedCategories();
        for (FileCategory cat : fixedCategories) {
            int count = getCategoryCount(cat);
            if (count > 0) {
                ss << "  • " << DesktopLayoutManager::getCategoryName(cat) 
                   << ": " << count << " 个\n";
            }
        }
        
        return ss.str();
    }
};

class FileOrganizer {
public:
    FileOrganizer();
    ~FileOrganizer();
    
    // 设置要扫描的目录（通常为桌面路径）
    void setDesktopPath(const std::string& path);
    
    // 添加整理规则（向后兼容，但不再使用）
    void addRule(const OrganizeRule& rule);
    
    // 获取所有规则（向后兼容）
    const std::vector<OrganizeRule>& getRules() const;
    
    // 清空所有规则（向后兼容）
    void clearRules();
    
    // 扫描桌面文件（仅一级，不递归）- 改为返回 fs::path 避免编码往返
    std::vector<fs::path> scanDesktopFiles() const;
    
    // 生成预览（不实际移动文件）- 新分类预览
    std::vector<OrganizePreviewItem> generatePreview() const;
    
    // 执行整理 - 按新模型：不移动文件，只返回分类结果
    OrganizeSummary executeOrganize();
    
    // 取消整理操作（线程安全）
    void cancelOrganize();
    
    // 检查是否已取消
    bool isCancelled() const;
    
    // 设置桌面布局管理器
    void setDesktopLayoutManager(DesktopLayoutManager* layoutManager);
    
    // 获取桌面布局管理器
    DesktopLayoutManager* getDesktopLayoutManager() const;
    
    // 生成整理规划（最小可用规划器）- 不移动文件，只返回分类统计
    // 在开始前会自动重置取消标志
    OrganizePlan generateOrganizePlan();

private:
    std::string m_desktopPath;
    std::vector<OrganizeRule> m_rules;
    std::vector<OrganizePreviewItem> m_currentPreview;
    std::atomic<bool> m_isCancelled{false};  // 取消标志
    DesktopLayoutManager* m_layoutManager;   // 桌面布局管理器
    
    // 向后兼容的辅助方法
    std::string getFileExtension(const std::string& fileName) const;
    bool matchesRule(const std::string& extension, const OrganizeRule& rule) const;
    const OrganizeRule* findMatchingRule(const std::string& fileName) const;
    
    // 新的分类预览生成
    std::vector<OrganizePreviewItem> generateCategoryPreview() const;
    
    // 旧版文件移动方法（标记为已弃用）
    bool moveFile(const std::string& sourcePath, 
                  const std::string& targetPath);
    bool targetFileExists(const std::string& targetDir, 
                          const std::string& fileName) const;
    
    // 执行旧版整理（向后兼容）
    OrganizeSummary executeLegacyOrganize();
    
    // 执行新版整理（桌面收纳盒模式）
    OrganizeSummary executeDesktopOrganize();
};

} // namespace ccdesk::core

#endif // FILE_ORGANIZER_H
