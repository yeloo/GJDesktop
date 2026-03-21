#include "desktop_auto_arrange_service.h"
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
}

//=============================================================================
// 公开接口实现
//=============================================================================

AutoArrangeResult DesktopAutoArrangeService::arrangeDesktop() {
    Logger::getInstance().info("DesktopAutoArrangeService: 开始自动整理");
    
    AutoArrangeResult result;
    
    // 1. 读取桌面图标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 1/5: 读取桌面图标");
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
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 2/5: 构建身份信息");
    std::vector<ArrangeableDesktopIcon> arrangeableIcons;
    
    for (const auto& icon : snapshot.icons) {
        ArrangeableDesktopIcon arrangeableIcon;
        arrangeableIcon.identity = buildIconIdentity(icon);
        arrangeableIcon.currentPosition = icon.position;
        arrangeableIcons.push_back(arrangeableIcon);
    }
    
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 构建身份信息完成",
        result.totalIcons
    );
    
    // 3. 分类图标
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 3/5: 分类图标");
    
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
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 4/5: 计算目标坐标");
    std::vector<DesktopLayoutTarget> targets = m_layoutPlanner->planLayout(arrangeableIcons);
    
    if (targets.empty()) {
        result.errorMessage = "布局规划结果为空";
        Logger::getInstance().error("DesktopAutoArrangeService: %s", result.errorMessage.c_str());
        return result;
    }
    
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 布局规划完成，%zu 个目标",
        targets.size()
    );
    
    // 打印布局规划（前 10 个）
    std::string layoutPlan = DesktopLayoutPlanner::printLayoutPlan(targets);
    Logger::getInstance().debug("DesktopAutoArrangeService: %s", layoutPlan.c_str());
    
    // 5. 写回位置
    Logger::getInstance().info("DesktopAutoArrangeService: 步骤 5/5: 写回位置");
    
    // 逐个移动图标
    for (const auto& target : targets) {
        std::string errorMessage;
        if (m_iconWriter->moveSingleIcon(target.identity, target.targetPosition, errorMessage)) {
            result.movedIcons++;
        } else {
            result.failedIcons++;
            Logger::getInstance().error(
                "DesktopAutoArrangeService: 移动图标 '%s' 失败: %s",
                target.identity.displayName.c_str(),
                errorMessage.c_str()
            );
        }
    }
    
    Logger::getInstance().info(
        "DesktopAutoArrangeService: 移动完成 - 成功: %zu, 失败: %zu",
        result.movedIcons,
        result.failedIcons
    );
    
    // 6. 返回结果
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
    
    // v1 简化实现：使用 displayName 作为 parsingName
    // 注意：这不够精确，因为 displayName 可能重复
    // v2 中需要通过 SHGDN_FORPARSING 获取完整文件系统路径
    identity.parsingName = icon.displayName;
    identity.isFileSystemItem = true;  // 假设都是文件系统项
    
    // TODO: v2 实现精确的 parsingName 获取
    // 需要通过 IShellFolder::GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strRet)
    // 然后用 StrRetToBufW 转换为字符串
    
    Logger::getInstance().debug(
        "DesktopAutoArrangeService: 构建图标身份 - displayName: '%s', parsingName: '%s'",
        identity.displayName.c_str(),
        identity.parsingName.c_str()
    );
    
    return identity;
}

} // namespace ccdesk::core
