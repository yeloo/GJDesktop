#include "desktop_icon_writer.h"
#include "logger.h"
#include "desktop_icon_accessor.h"  // 用于复用 COM 初始化逻辑

// Shell COM 头文件
#include <shobjidl.h>      // IFolderView, IShellFolder, IShellView
#include <shlguid.h>       // CLSID_ShellWindows, SID_STopLevelBrowser
#include <shlobj.h>        // IShellBrowser, IShellItem
#include <exdisp.h>        // IShellWindows
#include <shobjidl_core.h> // IFileOpenDialog (SHGetKnownFolderPath)

// ATL COM 智能指针
#include <atlbase.h>

// 通用头文件
#include <commctrl.h>       // ListView 消息
#include <shlwapi.h>       // StrRetToBufW
#include <psapi.h>         // GetModuleFileNameEx

namespace ccdesk::core {

//=============================================================================
// COM 辅助函数（复用 DesktopIconAccessor 的实现）
//=============================================================================

// 复用 COMInitializer 结构（从 desktop_icon_accessor.cpp 复制）
struct COMInitializer {
    bool usable;
    bool needUninitialize;
    
    COMInitializer() : usable(false), needUninitialize(false) {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        if (hr == S_OK) {
            usable = true;
            needUninitialize = true;
            Logger::getInstance().debug("DesktopIconWriter: COM initialized (STA)");
        } else if (hr == S_FALSE) {
            usable = true;
            needUninitialize = true;
            Logger::getInstance().debug("DesktopIconWriter: COM already initialized in this thread");
        } else if (hr == RPC_E_CHANGED_MODE) {
            usable = false;
            needUninitialize = false;
            Logger::getInstance().error("DesktopIconWriter: COM 线程模型不兼容 (RPC_E_CHANGED_MODE)");
        } else {
            usable = false;
            needUninitialize = false;
            Logger::getInstance().error("DesktopIconWriter: CoInitializeEx failed with HRESULT 0x%08lX", hr);
        }
    }
    
    ~COMInitializer() {
        if (needUninitialize) {
            CoUninitialize();
            Logger::getInstance().debug("DesktopIconWriter: COM uninitialized");
        }
    }
    
    COMInitializer(const COMInitializer&) = delete;
    COMInitializer& operator=(const COMInitializer&) = delete;
    COMInitializer(COMInitializer&&) = delete;
    COMInitializer& operator=(COMInitializer&&) = delete;
};

//=============================================================================
// 主路线：IFolderView COM 接口写回
//=============================================================================

bool DesktopIconWriter::writeUsingCOMInterface(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    // 1. 初始化 COM
    COMInitializer comInit;
    if (!comInit.usable) {
        errorMessage = "COM 初始化失败";
        return false;
    }
    
    // 2. 创建 ShellWindows 对象
    CComPtr<IShellWindows> spShellWindows;
    HRESULT hr = spShellWindows.CoCreateInstance(CLSID_ShellWindows);
    if (FAILED(hr)) {
        errorMessage = "创建 ShellWindows 对象失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    // 3. 查找桌面窗口
    CComVariant vtEmpty;
    vtEmpty.Clear();
    
    long hwndDesktop = 0;
    IDispatch* pDisp = nullptr;
    
    hr = spShellWindows->FindWindowSW(
        &vtEmpty,
        &vtEmpty,
        SWC_DESKTOP,
        &hwndDesktop,
        SWFO_NEEDDISPATCH,
        &pDisp
    );
    
    if (FAILED(hr) || pDisp == nullptr) {
        errorMessage = "查找桌面窗口失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    CComPtr<IDispatch> spDispatch;
    spDispatch.Attach(pDisp);
    
    // 4. 获取 IServiceProvider
    CComPtr<IServiceProvider> spServiceProvider;
    hr = spDispatch->QueryInterface(&spServiceProvider);
    if (FAILED(hr)) {
        errorMessage = "获取 IServiceProvider 失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    // 5. 获取 IShellBrowser
    CComPtr<IShellBrowser> spShellBrowser;
    hr = spServiceProvider->QueryService(
        SID_STopLevelBrowser,
        IID_IShellBrowser,
        reinterpret_cast<void**>(&spShellBrowser)
    );
    if (FAILED(hr)) {
        errorMessage = "获取 IShellBrowser 失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    // 6. 获取当前激活的 Shell View
    CComPtr<IShellView> spShellView;
    hr = spShellBrowser->QueryActiveShellView(&spShellView);
    if (FAILED(hr)) {
        errorMessage = "获取 IShellView 失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    // 7. 获取 IFolderView
    CComPtr<IFolderView> spFolderView;
    hr = spShellView->QueryInterface(&spFolderView);
    if (FAILED(hr)) {
        errorMessage = "获取 IFolderView 失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    // 8. 获取 IShellFolder（用于解析 parsingName）
    CComPtr<IShellFolder> spShellFolder;
    hr = spFolderView->GetFolder(IID_IShellFolder, reinterpret_cast<void**>(&spShellFolder));
    if (FAILED(hr)) {
        errorMessage = "获取 IShellFolder 失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    // 9. 解析 parsingName 为 PIDL
    // 将 UTF-8 parsingName 转换为 UTF-16
    int len = MultiByteToWideChar(CP_UTF8, 0, identity.parsingName.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        errorMessage = "转换 parsingName 到 UTF-16 失败";
        return false;
    }
    
    std::wstring parsingNameWide(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, identity.parsingName.c_str(), -1, &parsingNameWide[0], len);
    
    // 解析为 PIDL
    ULONG chEaten = 0;
    LPITEMIDLIST pidl = nullptr;
    hr = spShellFolder->ParseDisplayName(
        nullptr,
        nullptr,
        const_cast<LPWSTR>(parsingNameWide.c_str()),
        &chEaten,
        &pidl,
        nullptr
    );
    
    if (FAILED(hr) || pidl == nullptr) {
        errorMessage = "解析 parsingName 为 PIDL 失败 (HRESULT: " + std::to_string(hr) + ")";
        return false;
    }
    
    // 10. 设置图标位置
    hr = spFolderView->SetItemPosition(pidl, &targetPosition);
    
    if (FAILED(hr)) {
        errorMessage = "设置图标位置失败 (HRESULT: " + std::to_string(hr) + ")";
        CoTaskMemFree(pidl);
        return false;
    }
    
    // 11. 应用更改（可选，某些情况下需要 SelectAndPositionItems）
    // hr = spFolderView->SelectAndPositionItems(1, const_cast<LPCITEMIDLIST*>(&pidl), nullptr, SVSI_POSITION, nullptr);
    // if (FAILED(hr)) {
    //     Logger::getInstance().warning("DesktopIconWriter: SelectAndPositionItems 失败 (HRESULT: 0x%08lX)", hr);
    //     // 不视为致命错误，继续执行
    // }
    
    // 12. 释放 PIDL
    CoTaskMemFree(pidl);
    
    Logger::getInstance().info(
        "DesktopIconWriter: 成功移动图标 '%s' 到 (%d, %d)",
        identity.displayName.c_str(),
        targetPosition.x,
        targetPosition.y
    );
    
    return true;
}

//=============================================================================
// 备用路线：SysListView32 跨进程消息写回
//=============================================================================

bool DesktopIconWriter::writeUsingListView(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    // v1 中此路线无法可靠实现，因为：
    // 1. 无法通过 parsingName 定位图标
    // 2. 通过 displayName 查找可能重复
    // 3. 需要维护图标索引映射
    // 
    // 因此 v1 不建议使用此路线，仅作为诊断和部分能力验证
    
    errorMessage = "ListView 写回路线未实现（v1 不支持）";
    return false;
}

//=============================================================================
// 公开接口实现
//=============================================================================

bool DesktopIconWriter::moveSingleIcon(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    Logger::getInstance().info(
        "DesktopIconWriter: 开始移动图标 '%s' 到 (%d, %d)",
        identity.displayName.c_str(),
        targetPosition.x,
        targetPosition.y
    );
    
    // 优先使用 COM 路线
    bool success = writeUsingCOMInterface(identity, targetPosition, errorMessage);
    
    if (success) {
        Logger::getInstance().info(
            "DesktopIconWriter: 图标 '%s' 移动成功",
            identity.displayName.c_str()
        );
        return true;
    } else {
        Logger::getInstance().error(
            "DesktopIconWriter: 图标 '%s' 移动失败: %s",
            identity.displayName.c_str(),
            errorMessage.c_str()
        );
        return false;
    }
}

bool DesktopIconWriter::moveIcons(const std::vector<DesktopLayoutTarget>& targets) {
    if (targets.empty()) {
        Logger::getInstance().warning("DesktopIconWriter: 目标图标列表为空");
        return true;
    }
    
    Logger::getInstance().info(
        "DesktopIconWriter: 开始批量移动 %zu 个图标",
        targets.size()
    );
    
    size_t successCount = 0;
    size_t failCount = 0;
    
    for (const auto& target : targets) {
        std::string errorMessage;
        if (moveSingleIcon(target.identity, target.targetPosition, errorMessage)) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    Logger::getInstance().info(
        "DesktopIconWriter: 批量移动完成 - 成功: %zu, 失败: %zu",
        successCount,
        failCount
    );
    
    return failCount == 0;
}

} // namespace ccdesk::core
