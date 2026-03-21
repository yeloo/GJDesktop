#ifndef DESKTOP_LAYOUT_PLANNER_H
#define DESKTOP_LAYOUT_PLANNER_H

#include <string>
#include <vector>
#include <map>
#include "desktop_arrange_rule_engine.h"
#include "desktop_icon_writer.h"

namespace ccdesk::core {

/**
 * 布局区域配置（v1）
 * 
 * 每个分类对应一个固定的布局区域
 */
struct LayoutRegion {
    int startX;           // 起始 X 坐标
    int startY;           // 起始 Y 坐标
    int spacingX;         // 横向间距
    int spacingY;         // 纵向间距
    int maxColumns;       // 最大列数（超过后换列）
    
    LayoutRegion() 
        : startX(0)
        , startY(0)
        , spacingX(0)
        , spacingY(0)
        , maxColumns(0) {}
};

/**
 * 桌面布局规划器 v1
 * 
 * 职责：
 *   - 按固定区域规则计算每个图标的目标坐标
 *   - 同类图标在区域内按简单网格排列
 *   - 纵向优先（列为主）
 *   - 超过最大行数后换列
 */
class DesktopLayoutPlanner {
public:
    DesktopLayoutPlanner();
    ~DesktopLayoutPlanner() = default;
    
    /**
     * 规划布局
     * 
     * @param icons 可整理图标列表（已分类）
     * @return 目标布局项列表
     */
    std::vector<DesktopLayoutTarget> planLayout(
        const std::vector<ArrangeableDesktopIcon>& icons
    );
    
    /**
     * 获取分类的布局区域
     * 
     * @param category 分类
     * @return 布局区域配置
     */
    static LayoutRegion getLayoutRegion(IconCategory category);
    
    /**
     * 打印布局规划（用于调试）
     * 
     * @param targets 目标布局项列表
     * @return 布局规划文本
     */
    static std::string printLayoutPlan(const std::vector<DesktopLayoutTarget>& targets);

private:
    /**
     * 计算单个分类内图标的目标坐标
     * 
     * @param icons 图标列表（已过滤为单个分类）
     * @param category 分类
     * @return 目标布局项列表
     */
    std::vector<DesktopLayoutTarget> planCategoryLayout(
        const std::vector<ArrangeableDesktopIcon>& icons,
        IconCategory category
    );
};

} // namespace ccdesk::core

#endif // DESKTOP_LAYOUT_PLANNER_H
