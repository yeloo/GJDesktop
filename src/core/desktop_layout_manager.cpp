#include "desktop_layout_manager.h"
#include "logger.h"

#include <filesystem>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

namespace ccdesk {
namespace core {

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

std::string DesktopLayoutManager::getFileExtension(const std::string& filePath) const
{
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos && dotPos > 0 && dotPos < filePath.length() - 1) {
        std::string ext = filePath.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

bool DesktopLayoutManager::isExtensionInList(const std::string& extension,
                                            const char* extensions[], 
                                            size_t count) const
{
    for (size_t i = 0; i < count; ++i) {
        if (extension == extensions[i]) {
            return true;
        }
    }
    return false;
}

FileCategory DesktopLayoutManager::classifyFile(const std::string& filePath) const
{
    if (filePath.empty()) {
        return CATEGORY_OTHER;
    }
    
    try {
        // 1. 检查是否为文件夹
        if (fs::is_directory(filePath)) {
            return CATEGORY_FOLDER;
        }
        
        // 2. 获取扩展名
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
        Logger::getInstance().error("DesktopLayoutManager: 分类文件失败: " + 
                                   std::string(e.what()) + " path: " + filePath);
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