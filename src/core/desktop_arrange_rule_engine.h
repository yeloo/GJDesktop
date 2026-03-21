#ifndef DESKTOP_ARRANGE_RULE_ENGINE_H
#define DESKTOP_ARRANGE_RULE_ENGINE_H

#include <string>
#include <vector>
#include <memory>
#include "desktop_icon_accessor.h"

namespace ccdesk::core {

/**
 * 图标分类枚举（v1）
 * 
 * 固定分类，不扩展配置化
 */
enum class IconCategory {
    Folder,       // 文件夹
    Shortcut,     // 快捷方式
    Image,        // 图片
    Document,     // 文档
    Archive,      // 压缩包
    Executable,   // 可执行文件
    Other         // 其他
};

/**
 * 桌面整理规则引擎 v1
 * 
 * 职责：
 *   - 基于固定规则对图标进行分类
 *   - 不支持复杂配置，使用内置规则
 *   - 提供清晰代码边界
 */
class DesktopArrangeRuleEngine {
public:
    DesktopArrangeRuleEngine();
    ~DesktopArrangeRuleEngine() = default;
    
    /**
     * 对单个图标进行分类
     * 
     * @param identity 图标身份
     * @return 分类
     */
    IconCategory classifyIcon(const DesktopIconIdentity& identity);
    
    /**
     * 获取分类名称
     * 
     * @param category 分类
     * @return 分类名称（用于日志和 UI）
     */
    static std::string getCategoryName(IconCategory category);
    
    /**
     * 获取所有固定分类
     * 
     * @return 分类列表（不包含 Other）
     */
    static std::vector<IconCategory> getFixedCategories();

private:
    // 快捷方式扩展名
    static constexpr const char* SHORTCUT_EXTENSIONS[] = {
        ".lnk", ".url"
    };
    
    // 图片扩展名
    static constexpr const char* IMAGE_EXTENSIONS[] = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp",
        ".svg", ".webp", ".tiff", ".ico", ".heic"
    };
    
    // 文档扩展名
    static constexpr const char* DOCUMENT_EXTENSIONS[] = {
        ".doc", ".docx", ".pdf", ".txt", ".ppt", ".pptx",
        ".xls", ".xlsx", ".md", ".rtf", ".odt", ".wps"
    };
    
    // 压缩包扩展名
    static constexpr const char* ARCHIVE_EXTENSIONS[] = {
        ".zip", ".rar", ".7z", ".tar", ".gz",
        ".bz2", ".xz", ".z", ".cab", ".iso"
    };
    
    // 可执行文件扩展名
    static constexpr const char* EXECUTABLE_EXTENSIONS[] = {
        ".exe", ".bat", ".cmd", ".msi", ".jar", ".app"
    };
    
    /**
     * 判断是否为文件夹
     * 
     * @param parsingName Shell parsing name
     * @return true 如果是文件夹
     */
    bool isFolder(const std::string& parsingName) const;
    
    /**
     * 判断是否为快捷方式
     * 
     * @param parsingName Shell parsing name
     * @return true 如果是快捷方式
     */
    bool isShortcut(const std::string& parsingName) const;
    
    /**
     * 获取文件扩展名（小写）
     * 
     * @param parsingName Shell parsing name
     * @return 扩展名（包含点号），如果没有则为空字符串
     */
    std::string getFileExtension(const std::string& parsingName) const;
    
    /**
     * 检查扩展名是否在数组中
     * 
     * @param extension 文件扩展名
     * @param extensions 扩展名数组
     * @param count 数组长度
     * @return true 如果匹配
     */
    bool isExtensionInList(
        const std::string& extension,
        const char* const extensions[],
        size_t count
    ) const;
};

} // namespace ccdesk::core

#endif // DESKTOP_ARRANGE_RULE_ENGINE_H
