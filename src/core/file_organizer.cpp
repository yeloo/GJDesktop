#include "file_organizer.h"
#include "logger.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <QString> // 仅在Windows下需要转换
#endif

namespace fs = std::filesystem;
namespace ccdesk {
namespace core {

// 辅助函数：将 filesystem::path 转换为 UTF-8 编码的 std::string
// Windows: path.wstring() -> QString -> UTF-8 -> std::string
// Linux/macOS: 直接使用 path.string()
static std::string pathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    // Windows: 从 UTF-16 转换为 UTF-8
    return QString::fromStdWString(path.wstring()).toUtf8().toStdString();
#else
    // Linux/macOS: 直接使用 UTF-8
    return path.string();
#endif
}

// 辅助函数：将 filesystem::path::filename 转换为 UTF-8 编码的 std::string
static std::string filenameToUtf8String(const fs::path& path) {
#ifdef _WIN32
    // Windows: 从 UTF-16 转换为 UTF-8
    return QString::fromStdWString(path.filename().wstring()).toUtf8().toStdString();
#else
    // Linux/macOS: 直接使用 UTF-8
    return path.filename().string();
#endif
}

// 辅助函数：将 UTF-8 编码的 std::string 安全转换为 fs::path
// Windows: UTF-8 -> QString -> UTF-16 -> fs::path
// Linux/macOS: 直接使用 UTF-8 std::string 构造
static fs::path utf8StringToPath(const std::string& utf8Path) {
#ifdef _WIN32
    // Windows: 从 UTF-8 转换为 UTF-16，再构造 fs::path
    return fs::path(QString::fromUtf8(utf8Path.c_str()).toStdWString());
#else
    // Linux/macOS: 直接使用 UTF-8
    return fs::path(utf8Path);
#endif
}

FileOrganizer::FileOrganizer()
    : m_layoutManager(nullptr)
{}

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

std::vector<fs::path> FileOrganizer::scanDesktopFiles() const {
    std::vector<fs::path> files;
    
    // 调试日志：输出当前桌面路径
    Logger::getInstance().debug("FileOrganizer: [DEBUG] Current m_desktopPath: " + m_desktopPath);
    
    if (m_desktopPath.empty()) {
        Logger::getInstance().warning("FileOrganizer: Desktop path is empty");
        return files;
    }
    
    // 使用安全转换：从 UTF-8 std::string 构造 fs::path
    fs::path desktopPath = utf8StringToPath(m_desktopPath);
    if (!fs::exists(desktopPath)) {
        Logger::getInstance().warning("FileOrganizer: Desktop path does not exist: " + m_desktopPath);
        return files;
    }
    
    try {
        // 仅扫描一级目录，不递归
        // 直接使用 fs::path，避免 std::string 中转的编码风险
        for (const auto& entry : fs::directory_iterator(desktopPath)) {
            // 扫描普通文件和文件夹，不扫描子目录
            if (entry.is_regular_file() || entry.is_directory()) {
                // 调试日志：输出每个文件的完整路径和文件名（UTF-8 转换后）
                std::string fullPathUtf8 = pathToUtf8String(entry.path());
                std::string fileNameUtf8 = filenameToUtf8String(entry.path());
                
                Logger::getInstance().debug("FileOrganizer: [DEBUG] Found file - Path: " + fullPathUtf8 + 
                                         ", Filename: " + fileNameUtf8);
                
                // 直接添加 fs::path 到结果中，不做任何转换
                files.push_back(entry.path());
            }
        }
        Logger::getInstance().info("FileOrganizer: Scanned desktop, found " + 
                                 std::to_string(files.size()) + " items (files + folders)");
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
    // 使用安全转换：从 UTF-8 std::string 构造 fs::path
    fs::path targetDirPath = utf8StringToPath(targetDir);
    fs::path fileNamePath = utf8StringToPath(fileName);
    fs::path targetPath = targetDirPath / fileNamePath;
    return fs::exists(targetPath);
}

bool FileOrganizer::moveFile(const std::string& sourcePath, 
                            const std::string& targetPath) {
    // 彻底收口：moveFile 不再允许调用，直接返回失败
    Logger::getInstance().error("FileOrganizer: moveFile() 已被禁用，当前版本仅支持规划模式");
    Logger::getInstance().error("FileOrganizer: 拒绝移动 " + sourcePath + " -> " + targetPath);
    
    // 不再模拟成功，明确返回失败
    return false;
}

std::vector<OrganizePreviewItem> FileOrganizer::generatePreview() const {
    // 保持原有逻辑：如果有布局管理器，使用新分类预览；否则使用旧规则预览
    if (m_layoutManager) {
        return generateCategoryPreview();
    }

    // 否则使用旧规则预览（向后兼容）
    Logger::getInstance().info("FileOrganizer: Generating preview using legacy rules...");

    std::vector<OrganizePreviewItem> preview;

    // 直接获取 fs::path 列表，避免 std::string 中转
    auto files = scanDesktopFiles();

    for (const auto& filePath : files) {
        // 直接从 fs::path 提取文件名，避免编码往返
        std::string fileName = filenameToUtf8String(filePath);

        OrganizePreviewItem item;
        item.sourcePath = pathToUtf8String(filePath);  // 只在存入时转换
        item.fileName = fileName;

        const OrganizeRule* matchedRule = findMatchingRule(fileName);

        if (!matchedRule) {
            item.status = OrganizePreviewItem::NoRule;
            item.statusMessage = "未找到匹配的分类规则";
            item.matchedRuleName = "未匹配";
            item.categoryName = "未匹配";
        } else {
            item.matchedRuleName = matchedRule->name;
            item.categoryName = matchedRule->name;

            // 检查冲突（仅信息展示，不影响规划）
            if (targetFileExists(matchedRule->targetPath, fileName)) {
                item.status = OrganizePreviewItem::Conflict;
                item.statusMessage = "分类建议冲突 - 目标位置已存在同名文件";
            } else {
                item.status = OrganizePreviewItem::Movable;
                item.statusMessage = "建议归类到: " + matchedRule->name + " 分类";
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

std::vector<OrganizePreviewItem> FileOrganizer::generateCategoryPreview() const {
    Logger::getInstance().info("FileOrganizer: Generating preview using desktop layout manager...");
    
    std::vector<OrganizePreviewItem> preview;
    
    if (!m_layoutManager) {
        Logger::getInstance().error("FileOrganizer: No layout manager available");
        return preview;
    }
    
    // 直接获取 fs::path 列表
    auto files = scanDesktopFiles();
    
    for (const auto& filePath : files) {
        // 分类文件：直接传入 fs::path，避免编码转换
        FileCategory category = m_layoutManager->classifyFile(filePath);
        
        // 跳过 OTHER 分类（内部未匹配标记）
        if (category == CATEGORY_OTHER) {
            Logger::getInstance().debug("FileOrganizer: 跳过未分类文件: " + pathToUtf8String(filePath));
            continue;
        }
        
        // 只在存入时转换为 UTF-8
        OrganizePreviewItem item;
        item.sourcePath = pathToUtf8String(filePath);
        item.fileName = filenameToUtf8String(filePath);
        item.categoryName = DesktopLayoutManager::getCategoryName(category);
        item.matchedRuleName = item.categoryName;  // 使用分类名称作为"规则名称"
        
        // 规划模式：仅提供分类建议，不实际移动文件
        item.status = OrganizePreviewItem::Movable;
        item.statusMessage = "建议归类到: " + item.categoryName + " 分类";
        
        preview.push_back(item);
    }
    
    Logger::getInstance().info("FileOrganizer: Category preview generated with " + 
                             std::to_string(preview.size()) + " items");
    
    // 保存预览用于后续执行
    const_cast<FileOrganizer*>(this)->m_currentPreview = preview;
    
    return preview;
}

void FileOrganizer::cancelOrganize() {
    m_isCancelled.store(true, std::memory_order_release);
    Logger::getInstance().info("FileOrganizer: Cancel requested");
    
    // 布局管理器已移除 cancelLayout 方法，无需额外操作
}

bool FileOrganizer::isCancelled() const {
    return m_isCancelled.load(std::memory_order_acquire);
}

void FileOrganizer::setDesktopLayoutManager(DesktopLayoutManager* layoutManager) {
    m_layoutManager = layoutManager;
    Logger::getInstance().info("FileOrganizer: DesktopLayoutManager set");
}

DesktopLayoutManager* FileOrganizer::getDesktopLayoutManager() const {
    return m_layoutManager;
}

OrganizeSummary FileOrganizer::executeOrganize() {
    OrganizeSummary summary;
    
    Logger::getInstance().warning("FileOrganizer: executeOrganize() 已弃用，当前版本不支持真实文件移动");
    Logger::getInstance().warning("FileOrganizer: 请使用 generateOrganizePlan() 生成整理规划");
    
    // 设置弃用信息
    summary.cancelled = true;
    summary.totalItems = 0;
    summary.movedCount = 0;
    summary.skippedConflictCount = 0;
    summary.failedCount = 0;
    summary.noRuleCount = 0;
    summary.message = "此功能已弃用，当前版本仅支持规划模式";
    
    return summary;
}

OrganizeSummary FileOrganizer::executeLegacyOrganize() {
    OrganizeSummary summary;
    
    Logger::getInstance().warning("FileOrganizer: executeLegacyOrganize() 已弃用");
    Logger::getInstance().warning("FileOrganizer: 当前版本仅支持规划模式，不会执行真实文件移动");
    
    // 彻底收口：直接返回弃用提示，不再执行任何伪流程
    summary.cancelled = true;
    summary.totalItems = 0;
    summary.movedCount = 0;
    summary.skippedConflictCount = 0;
    summary.failedCount = 0;
    summary.noRuleCount = 0;
    summary.message = "此功能已弃用，仅支持规划模式";
    
    return summary;
}

OrganizeSummary FileOrganizer::executeDesktopOrganize() {
    OrganizeSummary summary;
    
    Logger::getInstance().warning("FileOrganizer: executeDesktopOrganize() 已弃用");
    Logger::getInstance().warning("FileOrganizer: 当前版本仅支持规划模式，不会执行真实文件移动");
    
    // 彻底收口：直接返回弃用提示，不再包含任何伪执行流程
    summary.cancelled = true;
    summary.totalItems = 0;
    summary.movedCount = 0;
    summary.skippedConflictCount = 0;
    summary.failedCount = 0;
    summary.noRuleCount = 0;
    summary.message = "此功能已弃用，仅支持规划模式";
    
    return summary;
}

OrganizePlan FileOrganizer::generateOrganizePlan() {
    OrganizePlan plan;
    
    // 重置取消标志（每次新分析前必须重置）
    m_isCancelled.store(false, std::memory_order_release);
    
    Logger::getInstance().info("FileOrganizer: 开始分析桌面文件分类");
    
    if (m_desktopPath.empty()) {
        Logger::getInstance().error("FileOrganizer: 桌面路径未设置，无法生成规划");
        return plan;
    }
    
    plan.desktopPath = m_desktopPath;
    
    // 直接获取 fs::path 列表
    auto files = scanDesktopFiles();
    plan.totalFiles = static_cast<int>(files.size());
    
    if (plan.totalFiles == 0) {
        Logger::getInstance().info("FileOrganizer: 桌面上没有文件需要整理");
        return plan;
    }
    
    Logger::getInstance().info("FileOrganizer: 扫描到 " + std::to_string(plan.totalFiles) + " 个桌面项目（文件+文件夹）");
    
    // 如果有桌面布局管理器，使用分类逻辑
    if (m_layoutManager) {
        for (const auto& filePath : files) {
            // 检查取消标志
            if (isCancelled()) {
                Logger::getInstance().info("FileOrganizer: 规划生成被取消");
                break;
            }
            
            // 分类文件：直接传入 fs::path
            FileCategory category = m_layoutManager->classifyFile(filePath);
            
            // 统计分类数量
            plan.categoryCounts[category]++;
            
            // 只在存入时转换为 UTF-8
            OrganizePreviewItem item;
            item.sourcePath = pathToUtf8String(filePath);
            item.fileName = filenameToUtf8String(filePath);
            item.categoryName = DesktopLayoutManager::getCategoryName(category);
            item.matchedRuleName = item.categoryName;
            item.status = OrganizePreviewItem::Movable;
            item.statusMessage = "属于: " + item.categoryName + " 分类";

            plan.items.push_back(item);
        }
    } else {
        // 如果没有布局管理器，使用旧规则
        Logger::getInstance().warning("FileOrganizer: 没有布局管理器，使用旧规则生成规划");

        for (const auto& filePath : files) {
            // 检查取消标志
            if (isCancelled()) {
                Logger::getInstance().info("FileOrganizer: 规划生成被取消");
                break;
            }

            // 只在存入时转换为 UTF-8
            OrganizePreviewItem item;
            item.sourcePath = pathToUtf8String(filePath);
            item.fileName = filenameToUtf8String(filePath);

            const OrganizeRule* matchedRule = findMatchingRule(item.fileName);

            if (!matchedRule) {
                item.status = OrganizePreviewItem::NoRule;
                item.statusMessage = "未找到匹配的分类规则";
                item.matchedRuleName = "未匹配";
                item.categoryName = "未匹配";
                plan.categoryCounts[CATEGORY_OTHER]++; // 归为其他
            } else {
                item.matchedRuleName = matchedRule->name;
                item.categoryName = matchedRule->name;

                // 尝试根据规则名称映射到分类
                FileCategory ruleCategory = CATEGORY_OTHER;
                if (matchedRule->name == "快捷方式") ruleCategory = CATEGORY_SHORTCUT;
                else if (matchedRule->name == "文件夹") ruleCategory = CATEGORY_FOLDER;
                else if (matchedRule->name == "文档") ruleCategory = CATEGORY_DOCUMENT;
                else if (matchedRule->name == "图片") ruleCategory = CATEGORY_IMAGE;
                else if (matchedRule->name == "视频") ruleCategory = CATEGORY_VIDEO;
                else if (matchedRule->name == "压缩包") ruleCategory = CATEGORY_ARCHIVE;

                // 检查冲突（仅信息展示，不影响规划）
                if (targetFileExists(matchedRule->targetPath, item.fileName)) {
                    item.status = OrganizePreviewItem::Conflict;
                    item.statusMessage = "分类建议冲突 - 目标位置已存在同名文件";
                    plan.categoryCounts[ruleCategory]++; // 冲突也归入对应分类
                } else {
                    item.status = OrganizePreviewItem::Movable;
                    item.statusMessage = "建议归类到: " + matchedRule->name + " 分类";
                    plan.categoryCounts[ruleCategory]++; // 归入对应分类
                }
            }

            plan.items.push_back(item);
        }
    }
    
    // 记录统计结果
    std::string summaryText = "桌面分类分析完成: " + std::to_string(plan.totalFiles) + " 个项目";
    if (m_layoutManager) {
        summaryText += "，分类统计: ";
        std::vector<FileCategory> fixedCategories = DesktopLayoutManager::getFixedCategories();
        for (FileCategory cat : fixedCategories) {
            auto it = plan.categoryCounts.find(cat);
            if (it != plan.categoryCounts.end() && it->second > 0) {
                summaryText += DesktopLayoutManager::getCategoryName(cat) + "(" + 
                               std::to_string(it->second) + ") ";
            }
        }
    }
    Logger::getInstance().info("FileOrganizer: " + summaryText);
    
    return plan;
}

} // namespace core
} // namespace ccdesk
