#ifndef DESKTOP_LAYOUT_MANAGER_H
#define DESKTOP_LAYOUT_MANAGER_H

#include <string>
#include <vector>

namespace ccdesk::core {

// 文件分类枚举 - 固定6大类 + 其他（内部未匹配标记）
enum FileCategory {
    CATEGORY_SHORTCUT,      // 快捷方式 (.lnk, .url)
    CATEGORY_FOLDER,        // 文件夹
    CATEGORY_DOCUMENT,      // 文档
    CATEGORY_IMAGE,         // 图片
    CATEGORY_VIDEO,         // 视频
    CATEGORY_ARCHIVE,       // 压缩包
    CATEGORY_OTHER,         // 其他（内部未匹配标记，不在UI中显示）
    CATEGORY_COUNT          // 分类总数
};

// 文件分类信息（简化版本，只用于分类统计）
struct FileClassification {
    std::string filePath;           // 文件完整路径
    std::string fileName;           // 文件名
    FileCategory category;          // 文件分类
    
    FileClassification() : category(CATEGORY_OTHER) {}
};

// 简单文件分类器（当前版本只做分类，不做布局）
class DesktopLayoutManager {
public:
    DesktopLayoutManager();
    ~DesktopLayoutManager();
    
    // 设置桌面路径
    void setDesktopPath(const std::string& path);
    
    // 获取桌面路径
    std::string getDesktopPath() const;
    
    // 分类文件（核心分类逻辑）
    FileCategory classifyFile(const std::string& filePath) const;
    
    // 获取分类名称
    static std::string getCategoryName(FileCategory category);
    
    // 获取分类对应的颜色（用于视觉区分）
    static std::string getCategoryColorCode(FileCategory category);
    
    // 获取所有固定分类（不包含CATEGORY_OTHER）
    static std::vector<FileCategory> getFixedCategories();

private:
    std::string m_desktopPath;
    
    // 快捷方式扩展名
    static constexpr const char* SHORTCUT_EXTENSIONS[] = {".lnk", ".url"};
    
    // 文档扩展名
    static constexpr const char* DOCUMENT_EXTENSIONS[] = {
        ".doc", ".docx", ".pdf", ".txt", ".ppt", ".pptx", 
        ".xls", ".xlsx", ".md", ".rtf", ".odt", ".wps"
    };
    
    // 图片扩展名
    static constexpr const char* IMAGE_EXTENSIONS[] = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", 
        ".svg", ".webp", ".tiff", ".ico", ".heic"
    };
    
    // 视频扩展名
    static constexpr const char* VIDEO_EXTENSIONS[] = {
        ".mp4", ".avi", ".mov", ".wmv", ".flv", 
        ".mkv", ".rmvb", ".m4v", ".mpg", ".webm"
    };
    
    // 压缩包扩展名
    static constexpr const char* ARCHIVE_EXTENSIONS[] = {
        ".zip", ".rar", ".7z", ".tar", ".gz", 
        ".bz2", ".xz", ".z", ".cab", ".iso"
    };
    
    // 检查扩展名是否在数组中
    bool isExtensionInList(const std::string& extension, 
                          const char* const extensions[], size_t count) const;
    
    // 获取文件扩展名（小写）
    std::string getFileExtension(const std::string& filePath) const;
};

} // namespace ccdesk::core

#endif // DESKTOP_LAYOUT_MANAGER_H