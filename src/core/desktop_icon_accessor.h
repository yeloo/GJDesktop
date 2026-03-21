#ifndef DESKTOP_ICON_ACCESSOR_H
#define DESKTOP_ICON_ACCESSOR_H

#include <string>
#include <vector>

// Windows SDK
#include <windows.h>

namespace ccdesk::core {

/**
 * v1 读取方法枚举
 */
enum class AccessMethod {
    COM_IFolderView,      // 使用 IFolderView::GetItemPosition（主路线）
    ListViewCrossProcess,  // 使用 LVM_GETITEMPOSITION + 跨进程（备用路线）
    Failed                 // 两种方法均失败
};

/**
 * v1 数据结构：桌面图标信息（仅包含已验证字段）
 */
struct DesktopIcon {
    std::string displayName;   // 图标显示名称（UTF-8，已验证）
    POINT position;            // 屏幕坐标（像素，已验证）
    
    DesktopIcon() : displayName(""), position{0, 0} {}
};

/**
 * v1 数据结构：读取结果快照
 */
struct DesktopIconSnapshot {
    std::vector<DesktopIcon> icons;  // 图标列表
    AccessMethod method;              // 实际使用的读取方法
    std::string errorMessage;         // 错误信息（空字符串表示成功）
    
    DesktopIconSnapshot() : method(AccessMethod::Failed), errorMessage("") {}
    
    /**
     * 是否成功读取
     * 
     * @return true 如果 method != AccessMethod::Failed
     */
    bool success() const {
        return method != AccessMethod::Failed;
    }
    
    /**
     * 当前快照中的图标数量
     * 
     * @return icons.size()
     */
    size_t size() const {
        return icons.size();
    }
};

/**
 * DesktopIconAccessor v1
 * 
 * 职责：
 *   - 封装桌面图标位置读取逻辑
 *   - 自主管理 COM 初始化/清理
 *   - 提供 COM 和 ListView 双路线回退机制
 */
class DesktopIconAccessor {
public:
    DesktopIconAccessor() = default;
    ~DesktopIconAccessor() = default;
    
    // 禁止拷贝（管理跨线程资源语义，即使当前实现不持有长期句柄）
    DesktopIconAccessor(const DesktopIconAccessor&) = delete;
    DesktopIconAccessor& operator=(const DesktopIconAccessor&) = delete;
    
    /**
     * 读取桌面图标位置信息（v1 核心接口）
     * 
     * @return DesktopIconSnapshot 包含图标列表和方法信息
     * 
     * 内部策略：
     *   1. 优先调用 readUsingCOMInterface()
     *   2. 失败时回退到 readUsingListView()
     *   3. 两种方法均失败时返回 method=Failed + errorMessage 非空
     * 
     * 错误约定：
     *   - 常规失败不抛异常（资源不足、权限错误、COM 错误等）
     *   - 通过 method=Failed 和 errorMessage 返回失败原因
     *   - 仅在严重错误（如内存分配失败）时可能抛 std::bad_alloc
     * 
     * 线程安全：
     *   - 调用线程会自动初始化 COM（COINIT_APARTMENTTHREADED）
     *   - 不假设外部已初始化 COM
     *   - 不保证跨线程并发安全，建议单线程调用
     */
    DesktopIconSnapshot readDesktopIcons();
    
    /**
     * 快速检查桌面是否可访问（诊断辅助）
     * 
     * @return true 如果 Explorer.exe 运行且桌面窗口存在
     * 
     * 用途：
     *   - 诊断工具：帮助判断为何 readDesktopIcons() 失败
     *   - 非必需：readDesktopIcons() 内部已有完整的错误处理
     *   - 非前置条件：即使返回 true，readDesktopIcons() 仍可能失败（权限、COM 错误等）
     */
    bool isDesktopAccessible() const;

private:
    /**
     * 方法1：使用 IFolderView COM 接口读取（主路线）
     * 
     * @return DesktopIconSnapshot 如果成功，method=COM_IFolderView
     * 
     * 已验证：用户本机 PoC 成功读取坐标
     */
    DesktopIconSnapshot readUsingCOMInterface();
    
    /**
     * 方法2：使用 SysListView32 跨进程消息读取（备用路线）
     * 
     * @return DesktopIconSnapshot 如果成功，method=ListViewCrossProcess
     * 
     * 注意：
     *   - 已验证：用户本机 PoC 成功读取坐标
     *   - v1 限制：由于无法获取真实 displayName，本路线不会作为成功快照返回
     *   - 仅当 COM 路线失败时，本路线用于诊断和部分能力验证
     */
    DesktopIconSnapshot readUsingListView();
};

} // namespace ccdesk::core

#endif // DESKTOP_ICON_ACCESSOR_H
