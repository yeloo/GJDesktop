#include "desktop_layout_planner.h"
#include "logger.h"
#include <sstream>

namespace ccdesk::core {

//=============================================================================
// 构造函数
//=============================================================================

DesktopLayoutPlanner::DesktopLayoutPlanner() {
    Logger::getInstance().info("DesktopLayoutPlanner: 初始化布局规划器");
}

//=============================================================================
// 公开接口实现
//=============================================================================

std::vector<DesktopLayoutTarget> DesktopLayoutPlanner::planLayout(
    const std::vector<ArrangeableDesktopIcon>& icons
) {
    Logger::getInstance().info(
        "DesktopLayoutPlanner: 开始规划布局，总图标数: %zu",
        icons.size()
    );
    
    std::vector<DesktopLayoutTarget> targets;
    
    // 按分类分组
    std::map<IconCategory, std::vector<ArrangeableDesktopIcon>> categorizedIcons;
    for (const auto& icon : icons) {
        // 将 std::string category 转换为 IconCategory
        IconCategory category = IconCategory::Other;  // 默认值
        if (icon.category == "Folder") {
            category = IconCategory::Folder;
        } else if (icon.category == "Shortcut") {
            category = IconCategory::Shortcut;
        } else if (icon.category == "Image") {
            category = IconCategory::Image;
        } else if (icon.category == "Document") {
            category = IconCategory::Document;
        } else if (icon.category == "Archive") {
            category = IconCategory::Archive;
        } else if (icon.category == "Executable") {
            category = IconCategory::Executable;
        }
        
        categorizedIcons[category].push_back(icon);
    }
    
    // 打印分类统计
    for (const auto& pair : categorizedIcons) {
        Logger::getInstance().info(
            "DesktopLayoutPlanner: 分类 %s: %zu 个图标",
            DesktopArrangeRuleEngine::getCategoryName(pair.first).c_str(),
            pair.second.size()
        );
    }
    
    // 为每个分类规划布局
    for (const auto& pair : categorizedIcons) {
        auto categoryTargets = planCategoryLayout(pair.second, pair.first);
        targets.insert(targets.end(), categoryTargets.begin(), categoryTargets.end());
    }
    
    Logger::getInstance().info(
        "DesktopLayoutPlanner: 布局规划完成，目标图标数: %zu",
        targets.size()
    );
    
    return targets;
}

LayoutRegion DesktopLayoutPlanner::getLayoutRegion(IconCategory category) {
    // v1 固定布局区域（1920x1080 屏幕为例）
    // 可根据实际情况调整
    
    LayoutRegion region;
    
    switch (category) {
        case IconCategory::Folder:
            // 文件夹：左上角，宽间距
            region.startX = 10;
            region.startY = 10;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 8;
            break;
            
        case IconCategory::Shortcut:
            // 快捷方式：右上角
            region.startX = 800;
            region.startY = 10;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 6;
            break;
            
        case IconCategory::Image:
            // 图片：左中
            region.startX = 10;
            region.startY = 400;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 8;
            break;
            
        case IconCategory::Document:
            // 文档：右中
            region.startX = 800;
            region.startY = 400;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 6;
            break;
            
        case IconCategory::Archive:
            // 压缩包：左下角
            region.startX = 10;
            region.startY = 700;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 8;
            break;
            
        case IconCategory::Executable:
            // 可执行文件：右下角
            region.startX = 800;
            region.startY = 700;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 6;
            break;
            
        case IconCategory::Other:
            // 其他：默认区域（可放在左下角后）
            region.startX = 10;
            region.startY = 900;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 8;
            break;
            
        default:
            // 默认区域
            region.startX = 10;
            region.startY = 10;
            region.spacingX = 80;
            region.spacingY = 100;
            region.maxColumns = 8;
            break;
    }
    
    return region;
}

std::string DesktopLayoutPlanner::printLayoutPlan(const std::vector<DesktopLayoutTarget>& targets) {
    std::stringstream ss;
    
    ss << "桌面布局规划:\n";
    ss << "----------------------------------------\n";
    
    // 按分类分组打印
    std::map<std::string, std::vector<DesktopLayoutTarget>> categorizedTargets;
    for (const auto& target : targets) {
        categorizedTargets[target.category].push_back(target);
    }
    
    for (const auto& pair : categorizedTargets) {
        ss << "\n分类: " << pair.first << "\n";
        ss << "----------------------------------------\n";
        
        for (const auto& target : pair.second) {
            ss << "  " << target.identity.displayName
               << " -> (" << target.targetPosition.x
               << ", " << target.targetPosition.y << ")\n";
        }
    }
    
    ss << "\n----------------------------------------\n";
    ss << "总计: " << targets.size() << " 个图标\n";
    
    return ss.str();
}

//=============================================================================
// 私有方法实现
//=============================================================================

std::vector<DesktopLayoutTarget> DesktopLayoutPlanner::planCategoryLayout(
    const std::vector<ArrangeableDesktopIcon>& icons,
    IconCategory category
) {
    std::vector<DesktopLayoutTarget> targets;
    
    if (icons.empty()) {
        return targets;
    }
    
    // 获取分类的布局区域
    LayoutRegion region = getLayoutRegion(category);
    
    // 纵向优先排列
    int x = region.startX;
    int y = region.startY;
    int currentColumn = 0;
    
    for (size_t i = 0; i < icons.size(); i++) {
        DesktopLayoutTarget target;
        target.identity = icons[i].identity;
        target.category = icons[i].category;
        target.targetPosition.x = x;
        target.targetPosition.y = y;
        
        targets.push_back(target);
        
        // 计算下一个图标位置（纵向优先）
        y += region.spacingY;
        
        // 检查是否需要换列
        int maxRows = 8;  // 每列最多8行
        if ((i + 1) % maxRows == 0) {
            y = region.startY;
            x += region.spacingX;
            currentColumn++;
            
            // 检查是否超过最大列数
            if (currentColumn >= region.maxColumns) {
                Logger::getInstance().warning(
                    "DesktopLayoutPlanner: 分类 %s 图标过多，超出布局区域",
                    DesktopArrangeRuleEngine::getCategoryName(category).c_str()
                );
            }
        }
    }
    
    Logger::getInstance().info(
        "DesktopLayoutPlanner: 分类 %s 布局规划完成，%zu 个图标",
        DesktopArrangeRuleEngine::getCategoryName(category).c_str(),
        targets.size()
    );
    
    return targets;
}

} // namespace ccdesk::core
