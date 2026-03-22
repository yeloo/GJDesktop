#include "desktop_auto_arrange_service.h"
#include "snapshot_manager.h"
#include "logger.h"
#include <sstream>

namespace ccdesk::core {

//=============================================================================
// 构造函数
//=============================================================================

DesktopAutoArrangeService::DesktopAutoArrangeService() {
    Logger::getInstance().info("DesktopAutoArrangeService: 初始化自动整理服务");

    // 创建各模块
    m_iconAccessor = std::make_unique<DesktopIconAccessor>();
    m_iconWriter = std::make_unique<DesktopIconWriter>();
    m_ruleEngine = std::make_unique<DesktopArrangeRuleEngine>();
    m_layoutPlanner = std::make_unique<DesktopLayoutPlanner>();
    m_snapshotManager = std::make_unique<SnapshotManager>();
}

DesktopAutoArrangeService::~DesktopAutoArrangeService() = default;

//=============================================================================
// 公开接口实现
//=============================================================================

// 【日志语义变更 + 行为变更】generateLayoutPlan：生成桌面布局规划（规划/预演，不执行真实写回）
LayoutPlanResult DesktopAutoArrangeService::generateLayoutPlan() {
    Logger::getInstance().info("DesktopAutoArrangeService: 开始生成桌面布局规划");
    
    LayoutPlanResult result;
    
    // 1. 读取桌面图标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 1/4: 读取桌面图标");
    DesktopIconSnapshot snapshot = m_iconAccessor->readDesktopIcons();
    
    if (!snapshot.success()) {
        result.errorMessage = "读取桌面图标失败: " + snapshot.errorMessage;
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }
    
    result.totalIcons = snapshot.icons.size();
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 成功读取 %zu 个桌面图标",
        result.totalIcons
    );
    
    // 2. 构建身份信息
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 2/4: 构建身份信息");
    std::vector<ArrangeableDesktopIcon> arrangeableIcons;
    
    for (const auto& icon : snapshot.icons) {
        ArrangeableDesktopIcon arrangeableIcon;
        arrangeableIcon.identity = buildIconIdentity(icon);
        arrangeableIcon.currentPosition = icon.position;
        arrangeableIcons.push_back(arrangeableIcon);
    }
    
    // 统计身份构建情况
    size_t validIdentityCount = 0;
    for (const auto& icon : arrangeableIcons) {
        if (!icon.identity.parsingName.empty()) {
            validIdentityCount++;
        }
    }

    Logger::getInstance().info(
        "DesktopAutoArrangeService: 构建身份信息完成 - %zu/%zu 图标有有效 parsingName",
        validIdentityCount,
        arrangeableIcons.size()
    );
    
    // 3. 分类图标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 3/4: 分类图标");
    
    for (auto& icon : arrangeableIcons) {
        IconCategory category = m_ruleEngine->classifyIcon(icon.identity);
        icon.category = DesktopArrangeRuleEngine::getCategoryName(category);
    }
    
    result.categorizedIcons = arrangeableIcons.size();
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 分类完成，%zu 个图标",
        result.categorizedIcons
    );
    
    // 打印分类统计
    std::map<std::string, int> categoryCounts;
    for (const auto& icon : arrangeableIcons) {
        categoryCounts[icon.category]++;
    }
    for (const auto& pair : categoryCounts) {
        Logger::getInstance().info(
            "DesktopAutoArrangeService: 分类 %s: %d 个",
            pair.first.c_str(),
            pair.second
        );
    }
    
    // 4. 计算目标坐标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 4/4: 计算目标坐标");
    std::vector<DesktopLayoutTarget> targets = m_layoutPlanner->planLayout(arrangeableIcons);
    
    if (targets.empty()) {
        result.errorMessage = "布局规划结果为空";
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }
    
    result.plannedIcons = targets.size();
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 布局规划完成，%zu 个目标",
        result.plannedIcons
    );
    
    // 打印布局规划（前 10 个）
    std::string layoutPlan = DesktopLayoutPlanner::printLayoutPlan(targets);
    Logger::getInstance().debug("DesktopAutoArrangeService: %s", layoutPlan.c_str());
    
    // 【行为变更】不执行写回，直接返回规划结果
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 桌面布局规划生成完成 - %zu 个图标已规划",
        result.plannedIcons
    );
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 【注意】当前版本仅支持规划生成，不执行真实桌面图标写回"
    );
    
    return result;
}

AutoArrangeResult DesktopAutoArrangeService::arrangeDesktop() {
    Logger::getInstance().info("DesktopAutoArrangeService: 开始执行桌面自动整理");

    AutoArrangeResult result;

    // 1. 读取桌面图标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 1/6: 读取桌面图标");
    DesktopIconSnapshot snapshot = m_iconAccessor->readDesktopIcons();

    if (!snapshot.success()) {
        result.errorMessage = "读取桌面图标失败: " + snapshot.errorMessage;
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }

    result.totalIcons = snapshot.icons.size();
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 成功读取 %zu 个桌面图标",
        result.totalIcons
    );

    // 安全检查：如果没有图标，直接返回
    if (result.totalIcons == 0) {
        result.errorMessage = "桌面上没有可整理的图标";
        Logger::getInstance().warning("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }

    // 2. 构建身份信息
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 2/6: 构建身份信息");
    std::vector<ArrangeableDesktopIcon> arrangeableIcons;

    for (const auto& icon : snapshot.icons) {
        ArrangeableDesktopIcon arrangeableIcon;
        arrangeableIcon.identity = buildIconIdentity(icon);
        arrangeableIcon.currentPosition = icon.position;
        arrangeableIcons.push_back(arrangeableIcon);
    }

    // 统计身份构建情况
    size_t validIdentityCount = 0;
    for (const auto& icon : arrangeableIcons) {
        if (!icon.identity.parsingName.empty()) {
            validIdentityCount++;
        }
    }

    Logger::getInstance().info(
        "DesktopAutoArrangeService: 构建身份信息完成 - %zu/%zu 图标有有效 parsingName",
        validIdentityCount,
        arrangeableIcons.size()
    );

    // 安全检查：如果没有有效身份，阻止执行
    if (validIdentityCount == 0) {
        result.errorMessage = "所有图标都缺少有效身份标识（parsingName），无法可靠执行整理";
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }

    // 3. 分类图标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 3/6: 分类图标");

    for (auto& icon : arrangeableIcons) {
        IconCategory category = m_ruleEngine->classifyIcon(icon.identity);
        icon.category = DesktopArrangeRuleEngine::getCategoryName(category);
    }

    result.categorizedIcons = arrangeableIcons.size();
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 分类完成，%zu 个图标",
        result.categorizedIcons
    );

    // 打印分类统计
    std::map<std::string, int> categoryCounts;
    for (const auto& icon : arrangeableIcons) {
        categoryCounts[icon.category]++;
    }
    for (const auto& pair : categoryCounts) {
        Logger::getInstance().info(
            "DesktopAutoArrangeService: 分类 %s: %d 个",
            pair.first.c_str(),
            pair.second
        );
    }

    // 4. 计算目标坐标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 4/6: 计算目标坐标");
    std::vector<DesktopLayoutTarget> targets = m_layoutPlanner->planLayout(arrangeableIcons);

    if (targets.empty()) {
        result.errorMessage = "布局规划结果为空";
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }

    result.plannedIcons = targets.size();
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 布局规划完成，%zu 个目标",
        targets.size()
    );

    // 打印布局规划（前 10 个）
    std::string layoutPlan = DesktopLayoutPlanner::printLayoutPlan(targets);
    Logger::getInstance().debug("DesktopAutoArrangeService: %s", layoutPlan.c_str());

    // 5. 创建执行前位置快照
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 5/6: 创建并保存执行前位置快照");
    result.preExecutionSnapshot = std::make_unique<DesktopLayoutSnapshot>(
        createPositionSnapshot(arrangeableIcons)
    );

    if (!result.preExecutionSnapshot->isEmpty()) {
        Logger::getInstance().info(
            "DesktopAutoArrangeService: 位置快照创建完成 - %zu 个图标",
            result.preExecutionSnapshot->totalCount
        );

        // 持久化保存快照
        if (!m_snapshotManager->saveSnapshot(*result.preExecutionSnapshot)) {
            Logger::getInstance().warning("DesktopAutoArrangeService: 保存快照到磁盘失败");
            // 注意：快照保存失败不应阻止执行，仅记录警告
        } else {
            Logger::getInstance().info("DesktopAutoArrangeService: 快照已持久化保存");
        }
    }

    // 6. 真实写回位置
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 6/6: 真实写回位置（将修改桌面图标位置）");

    // 逐个移动图标，记录详细失败信息
    for (const auto& target : targets) {
        std::string errorMessage;
        bool success = m_iconWriter->moveSingleIcon(target.identity, target.targetPosition, errorMessage);

        if (success) {
            result.movedIcons++;
        } else {
            result.failedIcons++;

            // 记录失败详情
            char posStr[32];
            snprintf(posStr, sizeof(posStr), "(%d, %d)", target.targetPosition.x, target.targetPosition.y);

            result.failures.push_back(ArrangeFailureDetail(
                target.identity.displayName,
                target.identity.parsingName,
                posStr,
                errorMessage
            ));

            Logger::getInstance().error(
                "DesktopAutoArrangeService: 移动图标 '%s' (parsingName: '%s') 失败: %s",
                target.identity.displayName.c_str(),
                target.identity.parsingName.c_str(),
                errorMessage.c_str()
            );
        }
    }

    Logger::getInstance().info(
        "DesktopAutoArrangeService: 移动完成 - 成功: %zu, 失败: %zu",
        result.movedIcons,
        result.failedIcons
    );

    // 7. 返回结果
    if (result.failedIcons > 0) {
        result.errorMessage = "部分图标移动失败";
        Logger::getInstance().warning(
            "DesktopAutoArrangeService: 整理部分失败，%zu/%zu 图标移动失败",
            result.failedIcons,
            result.totalIcons
        );
    } else {
        Logger::getInstance().info("DesktopAutoArrangeService: 自动整理全部成功");
    }

    return result;
}

DesktopIconAccessor* DesktopAutoArrangeService::getIconAccessor() const {
    return m_iconAccessor.get();
}

DesktopIconWriter* DesktopAutoArrangeService::getIconWriter() const {
    return m_iconWriter.get();
}

DesktopArrangeRuleEngine* DesktopAutoArrangeService::getRuleEngine() const {
    return m_ruleEngine.get();
}

DesktopLayoutPlanner* DesktopAutoArrangeService::getLayoutPlanner() const {
    return m_layoutPlanner.get();
}

//=============================================================================
// 私有方法实现
//=============================================================================

DesktopIconIdentity DesktopAutoArrangeService::buildIconIdentity(const DesktopIcon& icon) {
    DesktopIconIdentity identity;

    identity.displayName = icon.displayName;
    identity.parsingName = icon.parsingName;
    identity.isFileSystemItem = icon.isFileSystemItem;

    // 检查 parsingName 是否为空
    if (identity.parsingName.empty()) {
        Logger::getInstance().warning(
            "DesktopAutoArrangeService: 图标 '%s' 没有 parsingName，身份不稳定",
            identity.displayName.c_str()
        );
    } else {
        Logger::getInstance().debug(
            "DesktopAutoArrangeService: 构建图标身份 - displayName: '%s', parsingName: '%s', isFileSystemItem: %s",
            identity.displayName.c_str(),
            identity.parsingName.c_str(),
            identity.isFileSystemItem ? "true" : "false"
        );
    }

    return identity;
}

DesktopLayoutSnapshot DesktopAutoArrangeService::createPositionSnapshot(
    const std::vector<ArrangeableDesktopIcon>& icons
) {
    Logger::getInstance().info("DesktopAutoArrangeService: 开始创建桌面图标位置快照");

    DesktopLayoutSnapshot snapshot;
    snapshot.positions.reserve(icons.size());

    // 获取当前时间戳
    time_t now = time(nullptr);
    char timeBuffer[64];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    snapshot.timestamp = timeBuffer;

    // 保存每个图标的位置
    for (const auto& icon : icons) {
        if (!icon.identity.parsingName.empty()) {
            snapshot.positions.push_back(DesktopIconPositionSnapshot(
                icon.identity.displayName,
                icon.identity.parsingName,
                icon.currentPosition,
                icon.category
            ));
        }
    }

    snapshot.totalCount = snapshot.positions.size();

    Logger::getInstance().info(
        "DesktopAutoArrangeService: 位置快照创建完成 - 保存 %zu 个图标位置",
        snapshot.totalCount
    );

    return snapshot;
}

SnapshotManager* DesktopAutoArrangeService::getSnapshotManager() const {
    return m_snapshotManager.get();
}

bool DesktopAutoArrangeService::hasSnapshot() const {
    return m_snapshotManager->hasSnapshot();
}

RestoreLayoutResult DesktopAutoArrangeService::restoreOriginalLayout() {
    Logger::getInstance().info("DesktopAutoArrangeService: 开始恢复桌面原布局");

    RestoreLayoutResult result;

    // 1. 加载快照
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 1/4: 加载快照");
    DesktopLayoutSnapshot snapshot;
    if (!m_snapshotManager->loadLastSnapshot(snapshot)) {
        result.errorMessage = "没有可用的桌面布局快照。请先执行一次自动整理，系统会自动保存执行前快照。";
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }

    Logger::getInstance().info(
        "DesktopAutoArrangeService: 快照加载成功 - 时间: %s, 图标数: %zu",
        snapshot.timestamp.c_str(),
        snapshot.totalCount
    );

    // 2. 检查快照有效性
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 2/4: 检查快照有效性");
    if (snapshot.positions.empty()) {
        result.errorMessage = "快照为空，无法恢复";
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }

    result.totalIcons = snapshot.totalCount;
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 快照有效，准备恢复 %zu 个图标",
        snapshot.totalCount
    );

    // 3. 逐个恢复图标位置
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 3/4: 恢复图标位置");

    for (const auto& position : snapshot.positions) {
        // 构建图标身份
        DesktopIconIdentity identity;
        identity.displayName = position.displayName;
        identity.parsingName = position.parsingName;
        identity.isFileSystemItem = true;  // 假设都是文件系统项

        // 恢复到原始位置
        std::string errorMessage;
        bool success = m_iconWriter->moveSingleIcon(identity, position.originalPosition, errorMessage);

        if (success) {
            result.restoredIcons++;
        } else {
            result.failedIcons++;

            // 记录失败详情
            char posStr[32];
            snprintf(posStr, sizeof(posStr), "(%d, %d)",
                     position.originalPosition.x, position.originalPosition.y);

            result.failures.push_back(RestoreFailureDetail(
                position.displayName,
                position.parsingName,
                posStr,
                errorMessage
            ));

            Logger::getInstance().error(
                "DesktopAutoArrangeService: 恢复图标 '%s' 失败: %s",
                position.displayName.c_str(),
                errorMessage.c_str()
            );
        }
    }

    Logger::getInstance().info(
        "DesktopAutoArrangeService: 恢复完成 - 成功: %zu, 失败: %zu",
        result.restoredIcons,
        result.failedIcons
    );

    // 4. 返回结果
    if (result.failedIcons > 0) {
        result.errorMessage = "部分图标恢复失败";
        Logger::getInstance().warning(
            "DesktopAutoArrangeService: 恢复部分失败，%zu/%zu 图标恢复失败",
            result.failedIcons,
            result.totalIcons
        );
    } else {
        Logger::getInstance().info("DesktopAutoArrangeService: 恢复全部成功");
    }

    return result;
}

} // namespace ccdesk::core
