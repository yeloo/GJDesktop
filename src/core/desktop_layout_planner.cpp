#include "desktop_layout_planner.h"
#include "logger.h"
#include <sstream>
#include <algorithm>
#include <windows.h>

namespace ccdesk::core {

//=============================================================================
// 静态辅助：读取主屏分辨率
//=============================================================================

ScreenParams DesktopLayoutPlanner::detectScreenParams() {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    if (w <= 0 || h <= 0) {
        Logger::getInstance().warning(
            "DesktopLayoutPlanner: GetSystemMetrics 返回无效值，使用默认 1920x1080"
        );
        return ScreenParams(1920, 1080);
    }
    Logger::getInstance().debug(
        "DesktopLayoutPlanner: 检测到屏幕分辨率 %d x %d", w, h
    );
    return ScreenParams(w, h);
}

//=============================================================================
// 构造函数
//=============================================================================

DesktopLayoutPlanner::DesktopLayoutPlanner(ScreenParams params)
    : m_screen(params)
{
    // 若传入的是默认值（1920×1080），尝试自动检测
    if (m_screen.width == 1920 && m_screen.height == 1080) {
        m_screen = detectScreenParams();
    }
    Logger::getInstance().info(
        "DesktopLayoutPlanner: 初始化布局规划器，屏幕 %d x %d",
        m_screen.width, m_screen.height
    );
}

void DesktopLayoutPlanner::setScreenParams(const ScreenParams& params) {
    m_screen = params;
    Logger::getInstance().info(
        "DesktopLayoutPlanner: 屏幕参数已更新 %d x %d",
        m_screen.width, m_screen.height
    );
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
        IconCategory category = IconCategory::Other;
        if      (icon.category == "Folder")     category = IconCategory::Folder;
        else if (icon.category == "Shortcut")   category = IconCategory::Shortcut;
        else if (icon.category == "Image")      category = IconCategory::Image;
        else if (icon.category == "Document")   category = IconCategory::Document;
        else if (icon.category == "Archive")    category = IconCategory::Archive;
        else if (icon.category == "Executable") category = IconCategory::Executable;

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

LayoutRegion DesktopLayoutPlanner::getLayoutRegion(IconCategory category) const {
    // P1-4: 动态基于屏幕分辨率划分 6 个区域（左/右 × 上/中/下）
    // 布局示意（横向）：
    //   左半屏 [0, w/2)      右半屏 [w/2, w)
    //   Folder / Shortcut
    //   Image  / Document
    //   Archive/ Executable
    // 图标间距 = iconSize（默认 80px），边距 = 10px

    const int margin   = 10;
    const int spacing  = m_screen.iconSize;
    const int halfW    = m_screen.width / 2;
    const int thirdH   = m_screen.height / 3;
    // 每列最多行数 = (区域高度 - margin) / spacing，至少 4
    const int maxRows  = std::max(4, (thirdH - margin) / spacing);
    // 每区域最大列数 = (半屏宽 - margin) / spacing，至少 4
    const int maxCols  = std::max(4, (halfW - margin) / spacing);

    LayoutRegion region;
    region.spacingX  = spacing;
    region.spacingY  = spacing;
    region.maxColumns = maxCols;

    switch (category) {
        case IconCategory::Folder:
            region.startX = margin;
            region.startY = margin;
            break;
        case IconCategory::Shortcut:
            region.startX = halfW + margin;
            region.startY = margin;
            break;
        case IconCategory::Image:
            region.startX = margin;
            region.startY = thirdH + margin;
            break;
        case IconCategory::Document:
            region.startX = halfW + margin;
            region.startY = thirdH + margin;
            break;
        case IconCategory::Archive:
            region.startX = margin;
            region.startY = thirdH * 2 + margin;
            break;
        case IconCategory::Executable:
            region.startX = halfW + margin;
            region.startY = thirdH * 2 + margin;
            break;
        case IconCategory::Other:
        default:
            // 其他类型放在右下角之后（尽量不覆盖已有区域）
            region.startX = margin;
            region.startY = m_screen.height - spacing * 2;
            break;
    }

    return region;
}

std::string DesktopLayoutPlanner::printLayoutPlan(const std::vector<DesktopLayoutTarget>& targets) {
    std::stringstream ss;

    ss << "桌面布局规划:\n";
    ss << "----------------------------------------\n";

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

    LayoutRegion region = getLayoutRegion(category);

    // P1-4: maxRows 基于屏幕高度动态计算（每个区域约占 1/3 屏高）
    const int thirdH = m_screen.height / 3;
    const int maxRows = std::max(4, (thirdH - 10) / region.spacingY);

    int x = region.startX;
    int y = region.startY;
    int currentColumn = 0;

    for (size_t i = 0; i < icons.size(); i++) {
        DesktopLayoutTarget target;
        target.identity        = icons[i].identity;
        target.category        = icons[i].category;
        target.targetPosition.x = x;
        target.targetPosition.y = y;

        targets.push_back(target);

        y += region.spacingY;

        if ((static_cast<int>(i) + 1) % maxRows == 0) {
            y = region.startY;
            x += region.spacingX;
            currentColumn++;

            if (currentColumn >= region.maxColumns) {
                Logger::getInstance().warning(
                    "DesktopLayoutPlanner: 分类 %s 图标过多，超出布局区域",
                    DesktopArrangeRuleEngine::getCategoryName(category).c_str()
                );
            }
        }
    }

    Logger::getInstance().info(
        "DesktopLayoutPlanner: 分类 %s 布局规划完成，%zu 个图标（maxRows=%d）",
        DesktopArrangeRuleEngine::getCategoryName(category).c_str(),
        targets.size(),
        maxRows
    );

    return targets;
}

} // namespace ccdesk::core
