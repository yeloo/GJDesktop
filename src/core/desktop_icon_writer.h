#ifndef DESKTOP_ICON_WRITER_H
#define DESKTOP_ICON_WRITER_H

#include <string>
#include <vector>
#include <windows.h>

namespace ccdesk::core {

/**
 * 图标身份结构（v1）
 *
 * 设计原则：
 *   - parsingName 是主标识（稳定的身份）
 *   - displayName 仅用于日志展示（可能重复）
 *   - isFileSystemItem 用于判断写回方式
 */
struct DesktopIconIdentity {
    std::string displayName;     // 图标显示名称（用于日志）
    std::string parsingName;     // Shell parsing name（文件系统路径或虚拟项标识，主标识）
    bool isFileSystemItem;       // 是否为文件系统项

    DesktopIconIdentity()
        : displayName("")
        , parsingName("")
        , isFileSystemItem(false) {}

    /**
     * 判断两个身份是否相同
     *
     * @param other 另一个身份
     * @return true 如果 parsingName 相同
     */
    bool equals(const DesktopIconIdentity& other) const {
        return parsingName == other.parsingName;
    }
};

/**
 * 可整理图标结构（v1）
 *
 * 用于规则引擎和布局规划器的输入
 */
struct ArrangeableDesktopIcon {
    DesktopIconIdentity identity;     // 图标身份
    POINT currentPosition;            // 当前位置
    std::string category;             // 分类（Folder, Shortcut, Image, Document, Archive, Executable, Other）

    ArrangeableDesktopIcon() : currentPosition{0, 0}, category("Other") {}
};

/**
 * 目标布局项（v1）
 *
 * 用于布局规划器的输出和图标写回器的输入
 */
struct DesktopLayoutTarget {
    DesktopIconIdentity identity;     // 图标身份
    POINT targetPosition;            // 目标位置
    std::string category;             // 分类

    DesktopLayoutTarget() : targetPosition{0, 0}, category("Other") {}
};

/**
 * 桌面图标写回器 v1
 *
 * 能力边界（明确说明）：
 *   - COM 写回路线：IFolderView::SetItemPosition / SelectAndPositionItems - 已实现
 *   - ListView 写回路线：LVM_SETITEMPOSITION - 已删除（不可靠）
 *   - moveSingleIcon：调用 COM 路线，诚实返回成功/失败
 *
 * 职责：
 *   - 封装桌面图标位置写回逻辑（COM 路线）
 *   - 使用 parsingName 作为稳定身份标识
 *   - 通过 IFolderView 接口写回图标位置
 *   - 不伪造成功，精确记录失败原因
 */
class DesktopIconWriter {
public:
    DesktopIconWriter() = default;
    ~DesktopIconWriter() = default;

    // 禁止拷贝
    DesktopIconWriter(const DesktopIconWriter&) = delete;
    DesktopIconWriter& operator=(const DesktopIconWriter&) = delete;

    /**
     * 批量移动图标到目标位置
     *
     * @param targets 目标布局项列表
     * @param movedCount 输出：成功移动数量
     * @param failedCount 输出：失败数量
     * @param errorMessage 输出：错误信息（首个失败的错误）
     * @return true 如果全部成功，false 如果有任何失败
     *
     * 当前实现：
     *   - 逐个调用 moveSingleIcon()
     *   - moveSingleIcon 走 COM 路线，会如实返回成功/失败
     */
    bool moveIcons(
        const std::vector<DesktopLayoutTarget>& targets,
        size_t& movedCount,
        size_t& failedCount,
        std::string& errorMessage
    );

    /**
     * 移动单个图标到目标位置
     *
     * @param identity 图标身份（用于定位图标）
     * @param targetPosition 目标坐标
     * @param errorMessage 输出错误信息
     * @return true 如果成功，false 如果失败
     *
     * 当前实现：
     *   - 调用 writeUsingCOMInterface()（COM 路线）
     *   - 诚实返回成功/失败
     *   - 详细记录失败原因到 errorMessage
     *
     * 支持能力：
     *   - 文件系统图标（isFileSystemItem = true）
     *   - 部分虚拟项（如果 Shell Folder 支持）
     *   - 使用 parsingName 作为稳定身份标识
     *
     * 限制：
     *   - 需要 parsingName 不为空
     *   - 需要 Shell Windows 服务可用
     *   - 需要 IFolderView 接口支持
     */
    bool moveSingleIcon(
        const DesktopIconIdentity& identity,
        POINT targetPosition,
        std::string& errorMessage
    );

private:
    /**
     * 方法1：使用 IFolderView COM 接口写回（主路线 - 已实现）
     *
     * @param identity 图标身份
     * @param targetPosition 目标坐标
     * @param errorMessage 输出错误信息
     * @return true 如果成功，false 如果失败
     *
     * 实现逻辑：
     *   1. 检查 parsingName 是否有效
     *   2. 获取桌面 Shell Folder (SHGetDesktopFolder)
     *   3. 将 parsingName 转换为 PIDL (ParseDisplayName)
     *   4. 通过 IShellWindows 查找桌面窗口
     *   5. 获取 IFolderView 接口
     *   6. 尝试 SetItemPosition 或 SelectAndPositionItems
     *
     * 支持能力：
     *   - 文件系统图标（完整路径）
     *   - 部分虚拟项（如果 Shell Folder 支持）
     *
     * 错误处理：
     *   - parsingName 为空：返回 false
     *   - ParseDisplayName 失败：返回 false
     *   - 无法获取 IFolderView：返回 false
     *   - SetItemPosition 失败：尝试 SelectAndPositionItems
     *   - 两者都失败：返回 false，详细错误信息到 errorMessage
     */
    bool writeUsingCOMInterface(
        const DesktopIconIdentity& identity,
        POINT targetPosition,
        std::string& errorMessage
    );

    /**
     * 方法2：使用 SysListView32 跨进程消息写回（诊断性降级路线）
     *
     * @param identity 图标身份
     * @param targetPosition 目标坐标
     * @param errorMessage 输出错误信息
     * @return true 如果成功，false 如果失败
     *
     * 当前实现限制：
     *   - 仅按 displayName 查找图标，同名图标会出错
     *   - LVM_GETITEMW 跨进程传递本进程指针给 Explorer，不稳定
     *   - 只查找 Progman，没有 WorkerW 回退逻辑
     *   - LVM_SETITEMPOSITION 成功判定不可靠
     *
     * 当前状态：
     *   - 不推荐使用，直接返回 false
     *   - errorMessage = "ListView 写回路线不可靠"
     */
    bool writeUsingListView(
        const DesktopIconIdentity& identity,
        POINT targetPosition,
        std::string& errorMessage
    );
};

} // namespace ccdesk::core

#endif // DESKTOP_ICON_WRITER_H
