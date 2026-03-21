#include "desktop_arrange_rule_engine.h"
#include "desktop_icon_accessor.h"
#include "logger.h"
#include <algorithm>
#include <cctype>

namespace ccdesk::core {

//=============================================================================
// 构造函数
//=============================================================================

DesktopArrangeRuleEngine::DesktopArrangeRuleEngine() {
    Logger::getInstance().info("DesktopArrangeRuleEngine: 初始化规则引擎");
}

//=============================================================================
// 分类核心逻辑
//=============================================================================

IconCategory DesktopArrangeRuleEngine::classifyIcon(const DesktopIconIdentity& identity) {
    // 1. 判断是否为文件夹
    if (isFolder(identity.parsingName)) {
        Logger::getInstance().debug(
            "DesktopArrangeRuleEngine: '%s' 分类为 Folder",
            identity.displayName.c_str()
        );
        return IconCategory::Folder;
    }
    
    // 2. 判断是否为快捷方式
    if (isShortcut(identity.parsingName)) {
        Logger::getInstance().debug(
            "DesktopArrangeRuleEngine: '%s' 分类为 Shortcut",
            identity.displayName.c_str()
        );
        return IconCategory::Shortcut;
    }
    
    // 3. 获取文件扩展名
    std::string extension = getFileExtension(identity.parsingName);
    if (extension.empty()) {
        Logger::getInstance().debug(
            "DesktopArrangeRuleEngine: '%s' 无扩展名，分类为 Other",
            identity.displayName.c_str()
        );
        return IconCategory::Other;
    }
    
    // 4. 判断扩展名类型
    if (isExtensionInList(extension, IMAGE_EXTENSIONS, sizeof(IMAGE_EXTENSIONS) / sizeof(IMAGE_EXTENSIONS[0]))) {
        Logger::getInstance().debug(
            "DesktopArrangeRuleEngine: '%s' 分类为 Image",
            identity.displayName.c_str()
        );
        return IconCategory::Image;
    }
    
    if (isExtensionInList(extension, DOCUMENT_EXTENSIONS, sizeof(DOCUMENT_EXTENSIONS) / sizeof(DOCUMENT_EXTENSIONS[0]))) {
        Logger::getInstance().debug(
            "DesktopArrangeRuleEngine: '%s' 分类为 Document",
            identity.displayName.c_str()
        );
        return IconCategory::Document;
    }
    
    if (isExtensionInList(extension, ARCHIVE_EXTENSIONS, sizeof(ARCHIVE_EXTENSIONS) / sizeof(ARCHIVE_EXTENSIONS[0]))) {
        Logger::getInstance().debug(
            "DesktopArrangeRuleEngine: '%s' 分类为 Archive",
            identity.displayName.c_str()
        );
        return IconCategory::Archive;
    }
    
    if (isExtensionInList(extension, EXECUTABLE_EXTENSIONS, sizeof(EXECUTABLE_EXTENSIONS) / sizeof(EXECUTABLE_EXTENSIONS[0]))) {
        Logger::getInstance().debug(
            "DesktopArrangeRuleEngine: '%s' 分类为 Executable",
            identity.displayName.c_str()
        );
        return IconCategory::Executable;
    }
    
    // 5. 未识别，归为 Other
    Logger::getInstance().debug(
        "DesktopArrangeRuleEngine: '%s' 扩展名 '%s' 未识别，分类为 Other",
        identity.displayName.c_str(),
        extension.c_str()
    );
    return IconCategory::Other;
}

//=============================================================================
// 辅助方法
//=============================================================================

std::string DesktopArrangeRuleEngine::getCategoryName(IconCategory category) {
    switch (category) {
        case IconCategory::Folder:     return "Folder";
        case IconCategory::Shortcut:   return "Shortcut";
        case IconCategory::Image:      return "Image";
        case IconCategory::Document:    return "Document";
        case IconCategory::Archive:    return "Archive";
        case IconCategory::Executable:  return "Executable";
        case IconCategory::Other:      return "Other";
        default:                      return "Unknown";
    }
}

std::vector<IconCategory> DesktopArrangeRuleEngine::getFixedCategories() {
    return {
        IconCategory::Folder,
        IconCategory::Shortcut,
        IconCategory::Image,
        IconCategory::Document,
        IconCategory::Archive,
        IconCategory::Executable
        // 不包含 Other
    };
}

bool DesktopArrangeRuleEngine::isFolder(const std::string& parsingName) const {
    // parsingName 以 "\" 或 "/" 结尾，或者不包含扩展名，通常是文件夹
    // 但最可靠的方式是通过 parsingName 的属性判断
    // v1 中使用简单启发式规则
    
    if (parsingName.empty()) {
        return false;
    }
    
    // 检查是否以路径分隔符结尾
    char lastChar = parsingName.back();
    if (lastChar == '\\' || lastChar == '/') {
        return true;
    }
    
    // 检查是否包含扩展名（没有扩展名可能是文件夹）
    size_t lastDot = parsingName.find_last_of('.');
    size_t lastSlash = parsingName.find_last_of('\\');
    
    if (lastSlash == std::string::npos) {
        lastSlash = parsingName.find_last_of('/');
    }
    
    // 如果最后一个点在最后一个斜杠之前，可能是文件夹
    if (lastDot != std::string::npos && lastSlash != std::string::npos && lastDot < lastSlash) {
        return true;
    }
    
    return false;
}

bool DesktopArrangeRuleEngine::isShortcut(const std::string& parsingName) const {
    std::string extension = getFileExtension(parsingName);
    return isExtensionInList(extension, SHORTCUT_EXTENSIONS, sizeof(SHORTCUT_EXTENSIONS) / sizeof(SHORTCUT_EXTENSIONS[0]));
}

std::string DesktopArrangeRuleEngine::getFileExtension(const std::string& parsingName) const {
    if (parsingName.empty()) {
        return "";
    }
    
    // 查找最后一个路径分隔符
    size_t lastSlash = parsingName.find_last_of('\\');
    if (lastSlash == std::string::npos) {
        lastSlash = parsingName.find_last_of('/');
    }
    
    // 查找最后一个点
    size_t lastDot = parsingName.find_last_of('.');
    
    // 如果没有点，或者点在最后一个斜杠之前，没有扩展名
    if (lastDot == std::string::npos || lastDot < lastSlash) {
        return "";
    }
    
    // 提取扩展名并转换为小写
    std::string extension = parsingName.substr(lastDot);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension;
}

bool DesktopArrangeRuleEngine::isExtensionInList(
    const std::string& extension,
    const char* const extensions[],
    size_t count
) const {
    if (extension.empty()) {
        return false;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (extension == extensions[i]) {
            return true;
        }
    }
    
    return false;
}

} // namespace ccdesk::core
