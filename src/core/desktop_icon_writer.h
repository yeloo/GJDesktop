#ifndef DESKTOP_ICON_WRITER_H
#define DESKTOP_ICON_WRITER_H

#include <string>
#include <vector>
#include <windows.h>

namespace ccdesk::core {

/**
 * 图标身份结构（v1）
 * 
 * 为什么需要身份标识：
 *   - displayName 不唯一（可能存在同名文件）
 *   - position 会变化（移动后位置改变）
 *   - 写回需要稳定的标识符定位同一图标
 * 
 * v1 方案：
 *   - 使用 parsingName（Shell parsing name）作为主标识
 *   - parsingName 是文件系统的可解析路径（如 "C:\Users\...\Desktop\file.txt"）
 *   - isFileSystemItem 标识是否为文件系统项（非虚拟文件夹）
 */
struct DesktopIconIdentity {
    std::string displayName;     // 图标显示名称（用于日志）
    std::string parsingName;     // Shell parsing name（文件系统路径或虚拟项标识）
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
 * 职责：
 *   - 封装桌面图标位置写回逻辑
 *   - 使用 IFolderView::SetItemPosition（主路线）
 *   - 提供 ListView 备用路线（如果 COM 路线不可行）
 *   - 自主管理 COM 初始化/清理
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
     * 策略：
     *   1. 优先使用 writeUsingCOMInterface()
     *   2. 失败时回退到 writeUsingListView()
     *   3. 逐个图标处理，单个失败不影响其他
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
     */
    bool moveSingleIcon(
        const DesktopIconIdentity& identity,
        POINT targetPosition,
        std::string& errorMessage
    );

private:
    /**
     * 方法1：使用 IFolderView COM 接口写回（主路线）
     * 
     * @param identity 图标身份
     * @param targetPosition 目标坐标
     * @param errorMessage 输出错误信息
     * @return true 如果成功，false 如果失败
     * 
     * 实现思路：
     *   1. 通过 parsingName 构建 PIDL（使用 IShellFolder::ParseDisplayName）
     *   2. 通过 IShellWindows -> IShellBrowser -> IShellView -> IFolderView
     *   3. 调用 IFolderView::SetItemPosition(pidl, &pt)
     *   4. IFolderView::SelectAndPositionItems 应用更改
     */
    bool writeUsingCOMInterface(
        const DesktopIconIdentity& identity,
        POINT targetPosition,
        std::string& errorMessage
    );
    
    /**
     * 方法2：使用 SysListView32 跨进程消息写回（备用路线）
     * 
     * @param identity 图标身份（注意：此路线无法通过身份定位图标）
     * @param targetPosition 目标坐标
     * @param errorMessage 输出错误信息
     * @return true 如果成功，false 如果失败
     * 
     * 注意：
     *   - 此路线无法通过 parsingName 定位图标
     *   - 需要通过图标索引或显示名称查找
     *   - v1 中仅作为诊断和部分能力验证，不建议使用
     */
    bool writeUsingListView(
        const DesktopIconIdentity& identity,
        POINT targetPosition,
        std::string& errorMessage
    );
};

} // namespace ccdesk::core

#endif // DESKTOP_ICON_WRITER_H
