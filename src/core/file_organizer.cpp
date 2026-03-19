#include "file_organizer.h"
#include "logger.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;
namespace ccdesk {
namespace core {

FileOrganizer::FileOrganizer() {}

FileOrganizer::~FileOrganizer() {}

void FileOrganizer::setDesktopPath(const std::string& path) {
    m_desktopPath = path;
    Logger::getInstance().info("FileOrganizer: Set desktop path to: " + path);
}

void FileOrganizer::addRule(const OrganizeRule& rule) {
    m_rules.push_back(rule);
    Logger::getInstance().info("FileOrganizer: Added rule: " + rule.name);
}

const std::vector<OrganizeRule>& FileOrganizer::getRules() const {
    return m_rules;
}

void FileOrganizer::clearRules() {
    m_rules.clear();
    Logger::getInstance().info("FileOrganizer: Cleared all rules");
}

std::vector<std::string> FileOrganizer::scanDesktopFiles() const {
    std::vector<std::string> files;
    
    if (m_desktopPath.empty() || !fs::exists(m_desktopPath)) {
        Logger::getInstance().warning("FileOrganizer: Desktop path does not exist: " + m_desktopPath);
        return files;
    }
    
    try {
        // 仅扫描一级目录，不递归
        for (const auto& entry : fs::directory_iterator(m_desktopPath)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
        Logger::getInstance().info("FileOrganizer: Scanned desktop, found " + 
                                 std::to_string(files.size()) + " files");
    } catch (const std::exception& e) {
        Logger::getInstance().error("FileOrganizer: Error scanning desktop: " + 
                                   std::string(e.what()));
    }
    
    return files;
}

std::string FileOrganizer::getFileExtension(const std::string& fileName) const {
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos && dotPos > 0) {
        std::string ext = fileName.substr(dotPos + 1);
        // 转换为小写便于比较
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

bool FileOrganizer::matchesRule(const std::string& extension, 
                               const OrganizeRule& rule) const {
    if (!rule.enabled) {
        return false;
    }
    
    if (extension.empty()) {
        return false;
    }
    
    // 将规则中的扩展名列表转换为小写并检查
    std::string ruleExt = rule.extensions;
    std::transform(ruleExt.begin(), ruleExt.end(), ruleExt.begin(), ::tolower);
    
    // 检查扩展名是否在规则的扩展名列表中
    size_t pos = 0;
    while (pos < ruleExt.length()) {
        size_t commaPos = ruleExt.find(',', pos);
        size_t endPos = (commaPos == std::string::npos) ? ruleExt.length() : commaPos;
        
        std::string extItem = ruleExt.substr(pos, endPos - pos);
        // 移除前后空格
        size_t firstNonSpace = extItem.find_first_not_of(" \t");
        size_t lastNonSpace = extItem.find_last_not_of(" \t");
        
        if (firstNonSpace != std::string::npos) {
            extItem = extItem.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
        }
        
        if (extItem == extension) {
            return true;
        }
        
        pos = endPos + 1;
    }
    
    return false;
}

const ccdesk::core::OrganizeRule* FileOrganizer::findMatchingRule(
    const std::string& fileName) const {
    std::string ext = getFileExtension(fileName);
    
    for (const auto& rule : m_rules) {
        if (matchesRule(ext, rule)) {
            return &rule;
        }
    }
    
    return nullptr;
}

bool FileOrganizer::targetFileExists(const std::string& targetDir, 
                                    const std::string& fileName) const {
    fs::path targetPath = fs::path(targetDir) / fileName;
    return fs::exists(targetPath);
}

std::vector<OrganizePreviewItem> FileOrganizer::generatePreview() const {
    std::vector<OrganizePreviewItem> preview;
    
    Logger::getInstance().info("FileOrganizer: Generating preview...");
    
    auto files = scanDesktopFiles();
    
    for (const auto& filePath : files) {
        fs::path path(filePath);
        std::string fileName = path.filename().string();
        
        OrganizePreviewItem item;
        item.sourcePath = filePath;
        item.fileName = fileName;
        
        const OrganizeRule* matchedRule = findMatchingRule(fileName);
        
        if (!matchedRule) {
            item.status = OrganizePreviewItem::NoRule;
            item.statusMessage = "No matching rule";
            item.matchedRuleName = "N/A";
            item.targetDirectory = "N/A";
        } else {
            item.matchedRuleName = matchedRule->name;
            item.targetDirectory = matchedRule->targetPath;
            
            // 检查冲突
            if (targetFileExists(matchedRule->targetPath, fileName)) {
                item.status = OrganizePreviewItem::Conflict;
                item.statusMessage = "File exists in target directory";
            } else {
                item.status = OrganizePreviewItem::Movable;
                item.statusMessage = "Ready to move";
            }
        }
        
        preview.push_back(item);
    }
    
    Logger::getInstance().info("FileOrganizer: Preview generated with " + 
                             std::to_string(preview.size()) + " items");
    
    // 保存预览用于后续执行
    const_cast<FileOrganizer*>(this)->m_currentPreview = preview;
    
    return preview;
}

bool FileOrganizer::moveFile(const std::string& sourcePath, 
                            const std::string& targetPath) {
    try {
        // 确保目标目录存在
        fs::path targetDir = fs::path(targetPath).parent_path();
        if (!fs::exists(targetDir)) {
            try {
                fs::create_directories(targetDir);
                Logger::getInstance().info("FileOrganizer: Created target directory: " + targetDir.string());
            } catch (const std::exception& e) {
                Logger::getInstance().error("FileOrganizer: Failed to create target directory: " + 
                                          std::string(e.what()));
                return false;
            }
        }
        
        fs::rename(sourcePath, targetPath);
        Logger::getInstance().info("FileOrganizer: Successfully moved file: " + 
                                 sourcePath + " -> " + targetPath);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("FileOrganizer: Failed to move file: " + 
                                   sourcePath + " Error: " + e.what());
        return false;
    }
}

OrganizeSummary FileOrganizer::executeOrganize() {
    OrganizeSummary summary;
    
    Logger::getInstance().info("FileOrganizer: Starting real execution...");
    
    // 重新生成预览以确保数据最新
    auto previewItems = generatePreview();
    
    for (const auto& item : previewItems) {
        OrganizeResult result;
        result.sourcePath = item.sourcePath;
        result.fileName = item.fileName;
        result.matchedRuleName = item.matchedRuleName;
        
        switch (item.status) {
            case OrganizePreviewItem::NoRule:
            {
                result.finalStatus = OrganizeResult::NoRule;
                result.message = "No matching rule found";
                result.targetPath = "N/A";
                summary.noRuleCount++;
                Logger::getInstance().info("FileOrganizer: [NO RULE] " + item.fileName);
                break;
            }
            
            case OrganizePreviewItem::Conflict:
            {
                result.finalStatus = OrganizeResult::SkippedConflict;
                result.message = "Target file already exists, skipped";
                result.targetPath = item.targetDirectory;
                summary.skippedConflictCount++;
                Logger::getInstance().info("FileOrganizer: [CONFLICT] " + item.fileName + 
                                         " -> " + item.targetDirectory);
                break;
            }
            
            case OrganizePreviewItem::Movable:
            {
                // 执行实际移动
                fs::path targetPath = fs::path(item.targetDirectory) / item.fileName;
                
                if (moveFile(item.sourcePath, targetPath.string())) {
                    result.finalStatus = OrganizeResult::Success;
                    result.message = "Successfully moved";
                    result.targetPath = targetPath.string();
                    summary.movedCount++;
                } else {
                    result.finalStatus = OrganizeResult::Failed;
                    result.message = "Failed to move file";
                    result.targetPath = targetPath.string();
                    summary.failedCount++;
                    Logger::getInstance().error("FileOrganizer: [FAILED] " + item.fileName);
                }
                break;
            }
            
            default:
                result.finalStatus = OrganizeResult::Failed;
                result.message = "Unknown status";
                summary.failedCount++;
        }
        
        summary.details.push_back(result);
    }
    
    // 输出汇总
    Logger::getInstance().info(
        "FileOrganizer: Execution complete - " +
        std::to_string(summary.movedCount) + " moved, " +
        std::to_string(summary.skippedConflictCount) + " skipped, " +
        std::to_string(summary.failedCount) + " failed, " +
        std::to_string(summary.noRuleCount) + " no rule"
    );
    
    return summary;
}

} // namespace core
} // namespace ccdesk
