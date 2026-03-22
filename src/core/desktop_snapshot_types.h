#ifndef DESKTOP_SNAPSHOT_TYPES_H
#define DESKTOP_SNAPSHOT_TYPES_H

#include <string>
#include <vector>

namespace ccdesk::core {

/**
 * 桌面图标位置快照项
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

} // namespace ccdesk::core

#endif // DESKTOP_SNAPSHOT_TYPES_H
