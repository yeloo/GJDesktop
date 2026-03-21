#ifndef DESKTOP_AUTO_ARRANGE_SERVICE_H
#define DESKTOP_AUTO_ARRANGE_SERVICE_H

#include <string>
#include <vector>
#include <memory>
#include "desktop_icon_accessor.h"
#include "desktop_icon_writer.h"
#include "desktop_arrange_rule_engine.h"
#include "desktop_layout_planner.h"

namespace ccdesk::core {

/**
 * 自动整理结果（v1）
 */
struct AutoArrangeResult {
    size_t totalIcons;          // 总图标数
    size_t categorizedIcons;     // 分类成功数
    size_t movedIcons;           // 移动成功数
    size_t failedIcons;          // 移动失败数
    std::string errorMessage;     // 错误信息
    
    AutoArrangeResult() 
        : totalIcons(0)
        , categorizedIcons(0)
        , movedIcons(0)
        , failedIcons(0)
        , errorMessage("") {}
    
    /**
     * 是否成功
     * 
     * @return true 如果 failedIcons == 0 且 errorMessage 为空
     */
    bool success() const {
        return failedIcons == 0 && errorMessage.empty();
    }
    
    /**
     * 获取结果摘要文本
     * 
     * @return 摘要文本
     */
    std::string getSummaryText() const {
        std::stringstream ss;
        ss << "自动整理结果:\n";
        ss << "----------------------------------------\n";
        ss << "总图标数: " << totalIcons << "\n";
        ss << "分类成功: " << categorizedIcons << "\n";
        ss << "移动成功: " << movedIcons << "\n";
        ss << "移动失败: " << failedIcons << "\n";
        if (!errorMessage.empty()) {
            ss << "错误信息: " << errorMessage << "\n";
        }
        ss << "----------------------------------------\n";
        ss << "整理" << (success() ? "成功" : "失败") << "\n";
        return ss.str();
    }
};

/**
 * 桌面自动整理服务 v1
 * 
 * 职责：
 *   - 提供统一的自动整理调用入口
 *   - 串联全流程：读取 -> 身份构建 -> 分类 -> 布局规划 -> 写回
 *   - 返回执行结果
 */
class DesktopAutoArrangeService {
public:
    DesktopAutoArrangeService();
    ~DesktopAutoArrangeService() = default;
    
    /**
     * 执行自动整理（核心接口）
     * 
     * @return AutoArrangeResult 整理结果
     * 
     * 流程：
     *   1. 读取桌面图标（DesktopIconAccessor）
     *   2. 构建身份信息（从 DesktopIcon 构建 DesktopIconIdentity）
     *   3. 分类图标（DesktopArrangeRuleEngine）
     *   4. 计算目标坐标（DesktopLayoutPlanner）
     *   5. 写回位置（DesktopIconWriter）
     *   6. 返回结果（AutoArrangeResult）
     * 
     * 错误处理：
     *   - 单个图标失败不会导致整体崩溃
     *   - 关键节点失败时返回 errorMessage
     *   - 所有关键操作都记录日志
     */
    AutoArrangeResult arrangeDesktop();
    
    /**
     * 获取桌面图标访问器
     * 
     * @return DesktopIconAccessor 指针
     */
    DesktopIconAccessor* getIconAccessor() const;
    
    /**
     * 获取桌面图标写回器
     * 
     * @return DesktopIconWriter 指针
     */
    DesktopIconWriter* getIconWriter() const;
    
    /**
     * 获取规则引擎
     * 
     * @return DesktopArrangeRuleEngine 指针
     */
    DesktopArrangeRuleEngine* getRuleEngine() const;
    
    /**
     * 获取布局规划器
     * 
     * @return DesktopLayoutPlanner 指针
     */
    DesktopLayoutPlanner* getLayoutPlanner() const;

private:
    std::unique_ptr<DesktopIconAccessor> m_iconAccessor;
    std::unique_ptr<DesktopIconWriter> m_iconWriter;
    std::unique_ptr<DesktopArrangeRuleEngine> m_ruleEngine;
    std::unique_ptr<DesktopLayoutPlanner> m_layoutPlanner;
    
    /**
     * 构建图标身份信息
     * 
     * @param icon DesktopIcon（读取结果）
     * @return DesktopIconIdentity（包含 parsingName）
     * 
     * 注意：v1 中 parsingName 需要通过额外方式获取
     *       当前简化实现使用 displayName 作为 parsingName
     *       v2 中需要通过 SHGDN_FORPARSING 获取完整路径
     */
    DesktopIconIdentity buildIconIdentity(const DesktopIcon& icon);
};

} // namespace ccdesk::core

#endif // DESKTOP_AUTO_ARRANGE_SERVICE_H
