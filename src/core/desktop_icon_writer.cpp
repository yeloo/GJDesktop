#include "desktop_icon_writer.h"
#include "logger.h"

// Windows 头文件
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>

namespace ccdesk::core {

//=============================================================================
// 主路线：SysListView32 跨进程消息写回（v1 使用）
//=============================================================================

bool DesktopIconWriter::writeUsingCOMInterface(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    // v1 中使用 SysListView32 路线，因为 IFolderView::SetItemPosition 不存在
    return writeUsingListView(identity, targetPosition, errorMessage);
}

bool DesktopIconWriter::writeUsingListView(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    // 1. 查找桌面窗口
    HWND hwndDesktop = FindWindow(L"Progman", L"Program Manager");
    if (hwndDesktop == nullptr) {
        errorMessage = "找不到桌面窗口 (Progman)";
        return false;
    }
    
    // 2. 查找 ShellView 窗口
    HWND hwndShellView = FindWindowEx(hwndDesktop, nullptr, L"SHELLDLL_DefView", nullptr);
    if (hwndShellView == nullptr) {
        errorMessage = "找不到 ShellView 窗口";
        return false;
    }
    
    // 3. 查找 SysListView32 窗口（桌面图标列表）
    HWND hwndListView = FindWindowEx(hwndShellView, nullptr, L"SysListView32", L"FolderView");
    if (hwndListView == nullptr) {
        errorMessage = "找不到桌面图标列表窗口 (SysListView32)";
        return false;
    }
    
    Logger::getInstance().debug("DesktopIconWriter: 找到桌面 ListView 句柄 0x%p", hwndListView);
    
    // 4. 获取图标数量
    int itemCount = SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
    if (itemCount <= 0) {
        errorMessage = "桌面图标列表为空";
        return false;
    }
    
    Logger::getInstance().debug("DesktopIconWriter: 桌面图标总数: %d", itemCount);
    
    // 5. 在 ListView 中查找目标图标
    int foundIndex = -1;
    std::wstring targetDisplayName;
    
    // 将 displayName 从 UTF-8 转换为 UTF-16
    int len = MultiByteToWideChar(CP_UTF8, 0, identity.displayName.c_str(), -1, nullptr, 0);
    if (len > 0) {
        targetDisplayName.resize(len - 1);
        MultiByteToWideChar(CP_UTF8, 0, identity.displayName.c_str(), -1, &targetDisplayName[0], len);
    }
    
    if (targetDisplayName.empty()) {
        errorMessage = "displayName 为空";
        return false;
    }
    
    // 遍历所有图标查找匹配项
    for (int i = 0; i < itemCount; i++) {
        // 分配缓冲区
        const int bufferSize = 260;  // MAX_PATH
        WCHAR buffer[bufferSize] = {0};
        
        LVITEMW lvi = {0};
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = buffer;
        lvi.cchTextMax = bufferSize;
        
        // 获取图标名称
        if (SendMessage(hwndListView, LVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&lvi))) {
            if (wcscmp(buffer, targetDisplayName.c_str()) == 0) {
                foundIndex = i;
                Logger::getInstance().debug("DesktopIconWriter: 找到图标 '%S' 在索引 %d", buffer, i);
                break;
            }
        }
    }
    
    if (foundIndex < 0) {
        errorMessage = "找不到图标 '" + identity.displayName + "'";
        return false;
    }
    
    // 6. 设置图标位置
    POINT point = targetPosition;
    BOOL result = SendMessage(hwndListView, LVM_SETITEMPOSITION, foundIndex, MAKELPARAM(point.x, point.y));
    
    if (!result) {
        errorMessage = "设置图标位置失败 (SendMessage 返回 FALSE)";
        return false;
    }
    
    // 7. 刷新视图
    InvalidateRect(hwndListView, nullptr, TRUE);
    UpdateWindow(hwndListView);
    
    Logger::getInstance().info(
        "DesktopIconWriter: 成功移动图标 '%s' (索引 %d) 到 (%d, %d)",
        identity.displayName.c_str(),
        foundIndex,
        targetPosition.x,
        targetPosition.y
    );
    
    return true;
}

//=============================================================================
// 批量移动图标
//=============================================================================

bool DesktopIconWriter::moveIcons(
    const std::vector<DesktopLayoutTarget>& targets,
    size_t& movedCount,
    size_t& failedCount,
    std::string& errorMessage
) {
    movedCount = 0;
    failedCount = 0;
    errorMessage = "";
    
    Logger::getInstance().info("DesktopIconWriter: 开始批量移动 %zu 个图标", targets.size());
    
    for (const auto& target : targets) {
        std::string singleError;
        bool success = writeUsingCOMInterface(target.identity, target.targetPosition, singleError);
        
        if (success) {
            movedCount++;
        } else {
            failedCount++;
            Logger::getInstance().error(
                "DesktopIconWriter: 移动图标 '%s' 失败: %s",
                target.identity.displayName.c_str(),
                singleError.c_str()
            );
            
            // 将第一个错误记录到 errorMessage
            if (errorMessage.empty()) {
                errorMessage = "图标 '" + target.identity.displayName + "' 移动失败: " + singleError;
            }
        }
    }
    
    Logger::getInstance().info(
        "DesktopIconWriter: 批量移动完成 - 成功: %zu, 失败: %zu",
        movedCount,
        failedCount
    );
    
    return failedCount == 0;
}

} // namespace ccdesk::core
