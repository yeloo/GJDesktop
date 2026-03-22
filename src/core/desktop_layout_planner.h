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
    int startX;       // 起始 X 坐标
    int startY;       // 起始 Y 坐标
    int spacingX;     // 横向间距（图标列间距）
    int spacingY;     // 纵向间距（图标行间距）
    int maxColumns;   // 最大列数（超过后警告）

    LayoutRegion()
        : startX(0), startY(0)
        , spacingX(80), spacingY(100)
        , maxColumns(6) {}
};

/**
 * 屏幕参数（P1-4: 供布局规划器使用）
 */
struct ScreenParams {
    int width;     // 屏幕宽度（像素）
    int height;    // 屏幕高度（像素）
    int iconSize;  // 图标间距基准（像素），默认 80

    ScreenParams() : width(1920), height(1080), iconSize(80) {}
    ScreenParams(int w, int h, int icon = 80)
        : width(w), height(h), iconSize(icon) {}
};

/**
 * 桌面布局规划器 v1
 *
 * 职责：
 *   - 按固定区域规则计算每个图标的目标坐标
 *   - 同类图标在区域内按简单网格排列
 *   - 纵向优先（列为主）
 *   - 基于屏幕分辨率动态计算区域坐标（P1-4）
 */
class DesktopLayoutPlanner {
public:
    /**
     * 构造函数
     *
     * @param params 屏幕参数（宽、高、图标间距）
     *               默认自动读取主屏分辨率
     */
    explicit DesktopLayoutPlanner(ScreenParams params = ScreenParams());
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
     * 获取分类的布局区域（基于当前屏幕参数）
     *
     * @param category 分类
     * @return 布局区域配置
     */
    LayoutRegion getLayoutRegion(IconCategory category) const;

    /**
     * 打印布局规划（用于调试）
     *
     * @param targets 目标布局项列表
     * @return 布局规划文本
     */
    static std::string printLayoutPlan(const std::vector<DesktopLayoutTarget>& targets);

    /**
     * 更新屏幕参数（例如分辨率变化时调用）
     */
    void setScreenParams(const ScreenParams& params);

private:
    ScreenParams m_screen;  // 当前屏幕参数

    /**
     * 计算单个分类内图标的目标坐标
     */
    std::vector<DesktopLayoutTarget> planCategoryLayout(
        const std::vector<ArrangeableDesktopIcon>& icons,
        IconCategory category
    );

    /**
     * 读取主屏分辨率（Windows GetSystemMetrics）
     */
    static ScreenParams detectScreenParams();
};

} // namespace ccdesk::core

#endif // DESKTOP_LAYOUT_PLANNER_H
