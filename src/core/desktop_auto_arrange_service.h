#ifndef DESKTOP_AUTO_ARRANGE_SERVICE_H
#define DESKTOP_AUTO_ARRANGE_SERVICE_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include "desktop_icon_accessor.h"
#include "desktop_icon_writer.h"
#include "desktop_arrange_rule_engine.h"
#include "desktop_layout_planner.h"
#include "snapshot_manager.h"

namespace ccdesk::core {

/**
 * 单个图标执行失败详情（用于UI展示）
 */
struct ArrangeFailureDetail {
    std::string displayName;       // 图标显示名称
    std::string parsingName;       // 图标身份标识
    std::string targetPosition;    // 目标坐标 "(x, y)"
    std::string errorMessage;      // 失败原因

    ArrangeFailureDetail()
        : displayName("")
        , parsingName("")
        , targetPosition("")
        , errorMessage("") {}

    ArrangeFailureDetail(const std::string& displayName_, const std::string& parsingName_,
                        const std::string& targetPosition_, const std::string& errorMessage_)
        : displayName(displayName_)
        , parsingName(parsingName_)
        , targetPosition(targetPosition_)
        , errorMessage(errorMessage_) {}
};

/**
 * 执行前桌面图标位置快照项
 */
struct DesktopIconPositionSnapshot {
    std::string displayName;       // 图标显示名称
    std::string parsingName;       // 图标身份标识（主标识）
    POINT originalPosition;        // 原始位置
    std::string category;          // 分类

    DesktopIconPositionSnapshot()
        : displayName("")
        , parsingName("")
        , originalPosition{0, 0}
        , category("Other") {}

    DesktopIconPositionSnapshot(const std::string& displayName_, const std::string& parsingName_,
                               POINT position_, const std::string& category_)
        : displayName(displayName_)
        , parsingName(parsingName_)
        , originalPosition(position_)
        , category(category_) {}
};

/**
 * 单个图标恢复失败详情（用于UI展示）
 */
struct RestoreFailureDetail {
    std::string displayName;       // 图标显示名称
    std::string parsingName;       // 图标身份标识
    std::string originalPosition;  // 原始坐标 "(x, y)"
    std::string errorMessage;      // 失败原因

    RestoreFailureDetail()
        : displayName("")
        , parsingName("")
        , originalPosition("")
        , errorMessage("") {}

    RestoreFailureDetail(const std::string& displayName_, const std::string& parsingName_,
                     const std::string& originalPosition_, const std::string& errorMessage_)
        : displayName(displayName_)
        , parsingName(parsingName_)
        , originalPosition(originalPosition_)
        , errorMessage(errorMessage_) {}
};

/**
 * 恢复桌面布局结果（恢复到原位置）
 */
struct RestoreLayoutResult {
    size_t totalIcons;              // 总图标数
    size_t restoredIcons;            // 恢复成功数
    size_t failedIcons;             // 恢复失败数
    std::string errorMessage;         // 全局错误信息
    std::vector<RestoreFailureDetail> failures;  // 失败详情列表

    RestoreLayoutResult()
        : totalIcons(0)
        , restoredIcons(0)
        , failedIcons(0)
        , errorMessage("")
        {}

    /**
     * 是否完全成功
     */
    bool success() const {
        return failedIcons == 0 && errorMessage.empty();
    }

    /**
     * 是否部分成功
     */
    bool partialSuccess() const {
        return restoredIcons > 0;
    }

    /**
     * 获取UI展示用的摘要文本
     */
    std::string getSummaryText() const {
        std::stringstream ss;
        ss << "恢复桌面原布局执行结果:\n";
        ss << "========================================\n";
        ss << "总图标数: " << totalIcons << "\n";
        ss << "恢复成功: " << restoredIcons << "\n";
        ss << "恢复失败: " << failedIcons << "\n";

        if (failedIcons > 0) {
            ss << "\n失败详情:\n";
            ss << "----------------------------------------\n";
            for (size_t i = 0; i < failures.size() && i < 10; i++) {
                const auto& failure = failures[i];
                ss << "[" << (i + 1) << "] " << failure.displayName << "\n";
                ss << "    原始位置: " << failure.originalPosition << "\n";
                ss << "    失败原因: " << failure.errorMessage << "\n";
            }
            if (failures.size() > 10) {
                ss << "... 还有 " << (failures.size() - 10) << " 个失败项\n";
            }
        }

        if (!errorMessage.empty()) {
            ss << "\n全局错误: " << errorMessage << "\n";
        }

        ss << "========================================\n";

        if (success()) {
            ss << "✓ 恢复全部成功\n";
        } else if (partialSuccess()) {
            ss << "⚠ 恢复部分成功（" << failedIcons << " 个图标恢复失败）\n";
        } else {
            ss << "✗ 恢复失败\n";
        }

        return ss.str();
    }
};

/**
 * 桌面图标位置快照
 */
struct DesktopLayoutSnapshot {
    std::vector<DesktopIconPositionSnapshot> positions;  // 所有图标位置快照
    std::string timestamp;                                 // 快照时间戳
    size_t totalCount;                                    // 总图标数

    DesktopLayoutSnapshot()
        : totalCount(0)
        , timestamp("") {}

    /**
     * 是否为空
     */
    bool isEmpty() const {
        return positions.empty();
    }

    /**
     * 获取摘要文本
     */
    std::string getSummaryText() const {
        std::stringstream ss;
        ss << "桌面图标位置快照:\n";
        ss << "----------------------------------------\n";
        ss << "快照时间: " << timestamp << "\n";
        ss << "图标总数: " << totalCount << "\n";
        ss << "----------------------------------------\n";
        return ss.str();
    }
};

/**
 * 自动整理结果（v2 - 支持真实执行）
 */
struct AutoArrangeResult {
    size_t totalIcons;                      // 总图标数
    size_t categorizedIcons;               // 分类成功数
    size_t plannedIcons;                   // 规划成功数
    size_t movedIcons;                     // 移动成功数
    size_t failedIcons;                    // 移动失败数
    std::string errorMessage;              // 全局错误信息
    std::vector<ArrangeFailureDetail> failures;  // 失败详情列表

    // 执行前快照（用于后续恢复）
    std::unique_ptr<DesktopLayoutSnapshot> preExecutionSnapshot;

    AutoArrangeResult()
        : totalIcons(0)
        , categorizedIcons(0)
        , plannedIcons(0)
        , movedIcons(0)
        , failedIcons(0)
        , errorMessage("")
        , preExecutionSnapshot(nullptr) {}

    /**
     * 是否完全成功
     *
     * @return true 如果 failedIcons == 0 且 errorMessage 为空
     */
    bool success() const {
        return failedIcons == 0 && errorMessage.empty();
    }

    /**
     * 是否部分成功
     *
     * @return true 如果有部分成功（movedIcons > 0）
     */
    bool partialSuccess() const {
        return movedIcons > 0;
    }

    /**
     * 获取UI展示用的摘要文本
     *
     * @return 摘要文本
     */
    std::string getSummaryText() const {
        std::stringstream ss;
        ss << "桌面自动整理执行结果:\n";
        ss << "========================================\n";
        ss << "总图标数: " << totalIcons << "\n";
        ss << "分类成功: " << categorizedIcons << "\n";
        ss << "规划成功: " << plannedIcons << "\n";
        ss << "移动成功: " << movedIcons << "\n";
        ss << "移动失败: " << failedIcons << "\n";

        if (failedIcons > 0) {
            ss << "\n失败详情:\n";
            ss << "----------------------------------------\n";
            for (size_t i = 0; i < failures.size() && i < 10; i++) {
                const auto& failure = failures[i];
                ss << "[" << (i + 1) << "] " << failure.displayName << "\n";
                ss << "    目标位置: " << failure.targetPosition << "\n";
                ss << "    失败原因: " << failure.errorMessage << "\n";
            }
            if (failures.size() > 10) {
                ss << "... 还有 " << (failures.size() - 10) << " 个失败项\n";
            }
        }

        if (!errorMessage.empty()) {
            ss << "\n全局错误: " << errorMessage << "\n";
        }

        ss << "========================================\n";

        if (success()) {
            ss << "✓ 整理全部成功\n";
        } else if (partialSuccess()) {
            ss << "⚠ 整理部分成功（" << failedIcons << " 个图标移动失败）\n";
        } else {
            ss << "✗ 整理失败\n";
        }

        return ss.str();
    }
};

/**
 * 桌面布局规划结果（规划/预演，不执行真实写回）
 */
struct LayoutPlanResult {
    size_t totalIcons;          // 总图标数
    size_t categorizedIcons;     // 分类成功数
    size_t plannedIcons;         // 规划成功数
    std::string errorMessage;    // 错误信息
    
    LayoutPlanResult() 
        : totalIcons(0)
        , categorizedIcons(0)
        , plannedIcons(0)
        , errorMessage("") {}
    
    /**
     * 是否成功
     * 
     * @return true 如果 errorMessage 为空
     */
    bool success() const {
        return errorMessage.empty();
    }
    
    /**
     * 获取结果摘要文本
     * 
     * @return 摘要文本
     */
    std::string getSummaryText() const {
        std::stringstream ss;
        ss << "桌面布局规划结果:\n";
        ss << "----------------------------------------\n";
        ss << "总图标数: " << totalIcons << "\n";
        ss << "分类成功: " << categorizedIcons << "\n";
        ss << "规划成功: " << plannedIcons << "\n";
        if (!errorMessage.empty()) {
            ss << "错误信息: " << errorMessage << "\n";
        }
        ss << "----------------------------------------\n";
        ss << "规划" << (success() ? "成功" : "失败") << "\n";
        return ss.str();
    }
};

/**
 * 桌面自动整理服务 v2（支持真实执行）
 *
 * 职责：
 *   - 提供统一的自动整理调用入口（规划模式 + 执行模式）
 *   - 串联全流程：读取 -> 身份构建 -> 分类 -> 布局规划 -> 快照 -> 写回
 *   - 返回详细执行结果（包含失败详情）
 *   - 支持执行前快照保存
 */
class DesktopAutoArrangeService {
public:
    DesktopAutoArrangeService();
    ~DesktopAutoArrangeService() = default;

    /**
     * 执行自动整理（真实执行模式 - 核心接口）
     *
     * @return AutoArrangeResult 整理结果（包含失败详情）
     *
     * 流程：
     *   1. 读取桌面图标（DesktopIconAccessor）
     *   2. 构建身份信息（从 DesktopIcon 构建 DesktopIconIdentity）
     *   3. 分类图标（DesktopArrangeRuleEngine）
     *   4. 计算目标坐标（DesktopLayoutPlanner）
     *   5. 保存执行前位置快照（DesktopLayoutSnapshot）
     *   6. 写回位置（DesktopIconWriter）
     *   7. 返回结果（AutoArrangeResult，含失败详情）
     *
     * 错误处理：
     *   - 单个图标失败不会导致整体崩溃
     *   - 每个失败图标都记录详细失败原因
     *   - 关键节点失败时返回 errorMessage
     *   - 所有关键操作都记录日志
     *
     * UI提示：
     *   - 执行前应确认用户意图
     *   - 执行后展示详细结果摘要
     *   - 失败时给出用户可理解的提示
     */
    AutoArrangeResult arrangeDesktop();

    /**
     * 恢复桌面原布局（恢复模式 - 核心接口）
     *
     * @return RestoreLayoutResult 恢复结果（包含失败详情）
     *
     * 流程：
     *   1. 从快照文件加载最近一次快照
     *   2. 检查快照有效性
     *   3. 逐个恢复图标到原始位置
     *   4. 返回结果（RestoreLayoutResult，含失败详情）
     *
     * 错误处理：
     *   - 无快照时返回错误
     *   - 单个图标失败不影响整体流程
     *   - 每个失败图标都记录详细失败原因
     *   - 关键节点失败时返回 errorMessage
     *
     * UI提示：
     *   - 无快照时提示用户
     *   - 恢复后展示详细结果摘要
     *   - 失败时给出用户可理解的提示
     */
    RestoreLayoutResult restoreOriginalLayout();

    /**
     * 生成桌面布局规划（规划/预演模式，不执行真实写回）
     *
     * @return LayoutPlanResult 规划结果
     *
     * 流程：
     *   1. 读取桌面图标（DesktopIconAccessor）
     *   2. 构建身份信息（从 DesktopIcon 构建 DesktopIconIdentity）
     *   3. 分类图标（DesktopArrangeRuleEngine）
     *   4. 计算目标坐标（DesktopLayoutPlanner）
     *   5. 【不执行写回】返回规划结果
     *
     * 注意：
     *   - 调用此方法不会修改桌面图标位置
     *   - 用于预览和分析桌面整理方案
     *   - 与 arrangeDesktop() 共享读取/分类/规划逻辑
     */
    LayoutPlanResult generateLayoutPlan();

    /**
     * 创建执行前位置快照
     *
     * @return DesktopLayoutSnapshot 位置快照
     *
     * 用途：
     *   - 执行前保存当前桌面图标位置
     *   - 用于后续恢复功能（预留）
     *   - 包含图标身份和原始坐标
     */
    DesktopLayoutSnapshot createPositionSnapshot(const std::vector<ArrangeableDesktopIcon>& icons);

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

    /**
     * 获取快照管理器
     *
     * @return SnapshotManager 指针
     */
    SnapshotManager* getSnapshotManager() const;

    /**
     * 检查是否有可用快照
     *
     * @return true 如果存在快照
     */
    bool hasSnapshot() const;

private:
    std::unique_ptr<DesktopIconAccessor> m_iconAccessor;
    std::unique_ptr<DesktopIconWriter> m_iconWriter;
    std::unique_ptr<DesktopArrangeRuleEngine> m_ruleEngine;
    std::unique_ptr<DesktopLayoutPlanner> m_layoutPlanner;
    std::unique_ptr<SnapshotManager> m_snapshotManager;  // 快照管理器

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
