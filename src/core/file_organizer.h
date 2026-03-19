#ifndef FILE_ORGANIZER_H
#define FILE_ORGANIZER_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include "organize_result.h"
#include "config_manager.h"

namespace ccdesk::core {

// 预览项（展示给用户看）
struct OrganizePreviewItem {
    std::string sourcePath;
    std::string fileName;
    std::string matchedRuleName;
    std::string targetDirectory;
    
    enum PreviewStatus {
        Movable,      // 可以移动
        Conflict,     // 冲突（目标已存在同名文件）
        NoRule        // 无匹配规则
    };
    
    PreviewStatus status;
    std::string statusMessage;
    
    OrganizePreviewItem() : status(Movable) {}
};

class FileOrganizer {
public:
    FileOrganizer();
    ~FileOrganizer();
    
    // 设置要扫描的目录（通常为桌面路径）
    void setDesktopPath(const std::string& path);
    
    // 添加整理规则
    void addRule(const OrganizeRule& rule);
    
    // 获取所有规则
    const std::vector<OrganizeRule>& getRules() const;
    
    // 清空所有规则
    void clearRules();
    
    // 扫描桌面文件（仅一级，不递归）
    std::vector<std::string> scanDesktopFiles() const;
    
    // 生成预览（不实际移动文件）
    std::vector<OrganizePreviewItem> generatePreview() const;
    
    // 执行真实整理（基于预览，实际移动文件）
    // 返回执行结果汇总
    OrganizeSummary executeOrganize();
    
    // 取消整理操作（线程安全）
    void cancelOrganize();
    
    // 检查是否已取消
    bool isCancelled() const;

private:
    std::string m_desktopPath;
    std::vector<OrganizeRule> m_rules;
    std::vector<OrganizePreviewItem> m_currentPreview;
    std::atomic<bool> m_isCancelled{false};  // 取消标志
    
    // 根据文件名获取扩展名
    std::string getFileExtension(const std::string& fileName) const;
    
    // 检查扩展名是否匹配规则
    bool matchesRule(const std::string& extension, const OrganizeRule& rule) const;
    
    // 查找匹配的规则
    const OrganizeRule* findMatchingRule(const std::string& fileName) const;
    
    // 检查目标路径是否存在该文件
    bool targetFileExists(const std::string& targetDir, const std::string& fileName) const;
    
    // 执行单个文件的移动
    bool moveFile(const std::string& sourcePath, 
                  const std::string& targetPath);
};

} // namespace ccdesk::core

#endif // FILE_ORGANIZER_H
