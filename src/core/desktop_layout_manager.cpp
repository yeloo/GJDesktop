#include "desktop_layout_manager.h"
#include "logger.h"

#include <filesystem>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <QString> // 仅在Windows下需要转换
#endif

namespace fs = std::filesystem;

namespace ccdesk {
namespace core {

// 辅助函数：将 fs::path 转换为 UTF-8 编码的 std::string（仅用于日志输出）
#ifdef _WIN32
static std::string pathToUtf8String(const fs::path& path) {
    return QString::fromStdWString(path.wstring()).toUtf8().toStdString();
}
#else
static std::string pathToUtf8String(const fs::path& path) {
    return path.string();
}
#endif

DesktopLayoutManager::DesktopLayoutManager()
{
    Logger::getInstance().info("DesktopLayoutManager: 初始化（简化分类器版本）");
}

DesktopLayoutManager::~DesktopLayoutManager()
{
    Logger::getInstance().info("DesktopLayoutManager: 销毁");
}

void DesktopLayoutManager::setDesktopPath(const std::string& path)
{
    m_desktopPath = path;
    Logger::getInstance().info("DesktopLayoutManager: 桌面路径设置为: " + path);
}

std::string DesktopLayoutManager::getDesktopPath() const
{
    return m_desktopPath;
}

std::string DesktopLayoutManager::getFileExtension(const fs::path& filePath) const
{
    // 直接基于 fs::path::extension() 获取扩展名
    // 不再通过字符串查找，避免编码问题
    fs::path extPath = filePath.extension();
    if (extPath.empty()) {
        return "";
    }
    
    // 扩展名统一转小写
#ifdef _WIN32
    // Windows: 从 UTF-16 转换为 UTF-8，避免编码问题
    std::string ext = QString::fromStdWString(extPath.wstring()).toUtf8().toStdString();
#else
    // Linux/macOS: 直接使用 string()
    std::string ext = extPath.string();
#endif
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool DesktopLayoutManager::isExtensionInList(const std::string& extension,
                                            const char* const extensions[],
                                            size_t count) const
{
    for (size_t i = 0; i < count; ++i) {
        if (extension == extensions[i]) {
            return true;
        }
    }
    return false;
}

FileCategory DesktopLayoutManager::classifyFile(const fs::path& filePath) const
{
    if (filePath.empty()) {
        return CATEGORY_OTHER;
    }
    
    try {
        // 1. 检查是否为文件夹 - 直接使用 fs::path
        if (fs::is_directory(filePath)) {
            return CATEGORY_FOLDER;
        }
        
        // 2. 获取扩展名 - 直接基于 fs::path
        std::string extension = getFileExtension(filePath);
        if (extension.empty()) {
            return CATEGORY_OTHER;
        }
        
        // 3. 检查是否为快捷方式
        if (isExtensionInList(extension, SHORTCUT_EXTENSIONS, 
                            sizeof(SHORTCUT_EXTENSIONS) / sizeof(SHORTCUT_EXTENSIONS[0]))) {
            return CATEGORY_SHORTCUT;
        }
        
        // 4. 检查是否为文档
        if (isExtensionInList(extension, DOCUMENT_EXTENSIONS, 
                            sizeof(DOCUMENT_EXTENSIONS) / sizeof(DOCUMENT_EXTENSIONS[0]))) {
            return CATEGORY_DOCUMENT;
        }
        
        // 5. 检查是否为图片
        if (isExtensionInList(extension, IMAGE_EXTENSIONS, 
                            sizeof(IMAGE_EXTENSIONS) / sizeof(IMAGE_EXTENSIONS[0]))) {
            return CATEGORY_IMAGE;
        }
        
        // 6. 检查是否为视频
        if (isExtensionInList(extension, VIDEO_EXTENSIONS, 
                            sizeof(VIDEO_EXTENSIONS) / sizeof(VIDEO_EXTENSIONS[0]))) {
            return CATEGORY_VIDEO;
        }
        
        // 7. 检查是否为压缩包
        if (isExtensionInList(extension, ARCHIVE_EXTENSIONS, 
                            sizeof(ARCHIVE_EXTENSIONS) / sizeof(ARCHIVE_EXTENSIONS[0]))) {
            return CATEGORY_ARCHIVE;
        }
        
        // 8. 未匹配到任何固定分类
        return CATEGORY_OTHER;
        
    } catch (const std::exception& e) {
        // 日志输出时单独转换为 UTF-8
        Logger::getInstance().error("DesktopLayoutManager: 分类文件失败: " + 
                                   std::string(e.what()) + " path: " + pathToUtf8String(filePath));
        return CATEGORY_OTHER;
    }
}

std::string DesktopLayoutManager::getCategoryName(FileCategory category)
{
    switch (category) {
        case CATEGORY_SHORTCUT: return "快捷方式";
        case CATEGORY_FOLDER: return "文件夹";
        case CATEGORY_DOCUMENT: return "文档";
        case CATEGORY_IMAGE: return "图片";
        case CATEGORY_VIDEO: return "视频";
        case CATEGORY_ARCHIVE: return "压缩包";
        case CATEGORY_OTHER: return "其他";
        default: return "未知";
    }
}

std::string DesktopLayoutManager::getCategoryColorCode(FileCategory category)
{
    // 使用CSS颜色代码
    switch (category) {
        case CATEGORY_SHORTCUT: return "#ffb6c1"; // 浅粉色
        case CATEGORY_FOLDER: return "#add8e6";   // 浅蓝色
        case CATEGORY_DOCUMENT: return "#dddd88"; // 浅黄色
        case CATEGORY_IMAGE: return "#98fb98";    // 浅绿色
        case CATEGORY_VIDEO: return "#ba55d3";     // 紫色
        case CATEGORY_ARCHIVE: return "#ffa500";    // 橙色
        case CATEGORY_OTHER: return "#d3d3d3";    // 浅灰色
        default: return "#f0f0f0";                // 浅灰色
    }
}

std::vector<FileCategory> DesktopLayoutManager::getFixedCategories()
{
    return {
        CATEGORY_SHORTCUT,
        CATEGORY_FOLDER,
        CATEGORY_DOCUMENT,
        CATEGORY_IMAGE,
        CATEGORY_VIDEO,
        CATEGORY_ARCHIVE
    };
}

} // namespace core
} // namespace ccdesk
