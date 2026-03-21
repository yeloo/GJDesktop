#include "desktop_icon_accessor.h"
#include "logger.h"

// Shell COM 头文件
#include <shobjidl.h>      // IFolderView, IShellFolder, IShellView
#include <shlguid.h>       // CLSID_ShellWindows, SID_STopLevelBrowser
#include <shlobj.h>        // IShellBrowser
#include <exdisp.h>        // IShellWindows

// ATL COM 智能指针（实现文件内部使用）
#include <atlbase.h>

// 通用头文件
#include <commctrl.h>       // ListView 消息
#include <shlwapi.h>       // StrRetToBufW

namespace ccdesk::core {

//=============================================================================
// COM 辅助函数（局部状态，不保存到成员）
//=============================================================================

/**
 * COMInitializer - RAII 类，管理 COM 初始化/清理
 * 
 * 设计：
 *   - 构造时调用 CoInitializeEx，保存初始化状态
 *   - usable: COM 是否可用（S_OK、S_FALSE 或 RPC_E_CHANGED_MODE 均可用）
 *   - needUninitialize: 本次是否需要在析构时调用 CoUninitialize（仅 S_OK 时为 true）
 *   - 析构时根据 needUninitialize 决定是否调用 CoUninitialize
 *   - 外部只读取 usable 状态，不再二次调用 CoInitializeEx
 */
struct COMInitializer {
    bool usable;             // COM 是否可用
    bool needUninitialize;   // 是否需要在析构时调用 CoUninitialize
    
    COMInitializer() : usable(false), needUninitialize(false) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (hr == S_OK) {
        // 第一次初始化成功，需要 CoUninitialize
        usable = true;
        needUninitialize = true;
        Logger::getInstance().debug("DesktopIconAccessor: COM initialized (STA)");
    } else if (hr == S_FALSE) {
        // COM 已在同线程初始化，可用，需要 CoUninitialize 配对
        usable = true;
        needUninitialize = true;
        Logger::getInstance().debug("DesktopIconAccessor: COM already initialized in this thread");
    } else if (hr == RPC_E_CHANGED_MODE) {
        // COM 已以不同线程模型初始化，v1 不冒险使用未验证的分支，标记为失败
        usable = false;
        needUninitialize = false;
        Logger::getInstance().error("DesktopIconAccessor: COM 线程模型不兼容 (RPC_E_CHANGED_MODE)");
    } else {
        // 初始化失败，不可用
        usable = false;
        needUninitialize = false;
        Logger::getInstance().error("DesktopIconAccessor: CoInitializeEx failed with HRESULT 0x%08lX", hr);
    }
    }
    
    ~COMInitializer() {
        if (needUninitialize) {
            CoUninitialize();
            Logger::getInstance().debug("DesktopIconAccessor: COM uninitialized");
        }
    }
    
    // 禁止拷贝和移动
    COMInitializer(const COMInitializer&) = delete;
    COMInitializer& operator=(const COMInitializer&) = delete;
    COMInitializer(COMInitializer&&) = delete;
    COMInitializer& operator=(COMInitializer&&) = delete;
};

//=============================================================================
// 主路线：IFolderView COM 接口读取
//=============================================================================

DesktopIconSnapshot DesktopIconAccessor::readUsingCOMInterface() {
    DesktopIconSnapshot snapshot;
    snapshot.method = AccessMethod::COM_IFolderView;
    
    // 1. 初始化 COM（局部状态管理）
    COMInitializer comInit;
    if (!comInit.usable) {
        snapshot.errorMessage = "COM 初始化失败";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 2. 创建 ShellWindows 对象
    CComPtr<IShellWindows> spShellWindows;
    HRESULT hr = spShellWindows.CoCreateInstance(CLSID_ShellWindows);
    if (FAILED(hr)) {
        snapshot.errorMessage = "创建 ShellWindows 对象失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 3. 查找桌面窗口（严格参考已验证成功的 PoC 调用链）
    CComVariant vtEmpty;
    vtEmpty.Clear();
    
    long hwndDesktop = 0;
    IDispatch* pDisp = nullptr;
    
    // 复用 PoC 中的调用方式：CComVariant + IDispatch**
    hr = spShellWindows->FindWindowSW(
        &vtEmpty,           // [in] 无特定位置
        &vtEmpty,           // [in] 无特定类
        SWC_DESKTOP,         // [in] 查找桌面
        &hwndDesktop,        // [out] 窗口句柄
        SWFO_NEEDDISPATCH,   // [in] 返回 IDispatch
        &pDisp               // [out] IDispatch 指针
    );
    
    if (hr == S_FALSE || hwndDesktop == 0) {
        snapshot.errorMessage = "未找到桌面窗口";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    if (FAILED(hr) || pDisp == nullptr) {
        snapshot.errorMessage = "查找桌面窗口失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    CComPtr<IDispatch> spDispatch;
    spDispatch.Attach(pDisp);  // 接管所有权
    
    // 4. 获取 IServiceProvider
    CComPtr<IServiceProvider> spServiceProvider;
    hr = spDispatch->QueryInterface(&spServiceProvider);
    if (FAILED(hr)) {
        snapshot.errorMessage = "获取 IServiceProvider 失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 5. 获取 IShellBrowser
    CComPtr<IShellBrowser> spShellBrowser;
    hr = spServiceProvider->QueryService(
        SID_STopLevelBrowser, 
        IID_IShellBrowser,
        reinterpret_cast<void**>(&spShellBrowser)
    );
    if (FAILED(hr)) {
        snapshot.errorMessage = "获取 IShellBrowser 失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 6. 获取当前激活的 Shell View
    CComPtr<IShellView> spShellView;
    hr = spShellBrowser->QueryActiveShellView(&spShellView);
    if (FAILED(hr)) {
        snapshot.errorMessage = "获取 IShellView 失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 7. 获取 IFolderView
    CComPtr<IFolderView> spFolderView;
    hr = spShellView->QueryInterface(&spFolderView);
    if (FAILED(hr)) {
        snapshot.errorMessage = "获取 IFolderView 失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 8. 获取 IShellFolder
    CComPtr<IShellFolder> spShellFolder;
    hr = spFolderView->GetFolder(IID_IShellFolder, reinterpret_cast<void**>(&spShellFolder));
    if (FAILED(hr)) {
        snapshot.errorMessage = "获取 IShellFolder 失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 9. 枚举所有桌面图标
    CComPtr<IEnumIDList> spEnumIDList;
    hr = spFolderView->Items(SVGIO_ALLVIEW, IID_IEnumIDList, reinterpret_cast<void**>(&spEnumIDList));
    if (FAILED(hr)) {
        snapshot.errorMessage = "枚举桌面图标失败 (HRESULT: " + std::to_string(hr) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 10. 遍历 PIDL 列表，读取名称和坐标
    PITEMID_CHILD pidl = nullptr;
    ULONG ulFetched = 0;
    
    while (SUCCEEDED(spEnumIDList->Next(1, &pidl, &ulFetched)) && ulFetched > 0) {
        // 获取显示名称（SHGDN_INFOLDER）
        STRRET strRet;
        ZeroMemory(&strRet, sizeof(strRet));

        std::string displayName;
        std::string parsingName;
        bool isFileSystemItem = false;

        // 1. 读取显示名称
        hr = spShellFolder->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &strRet);
        if (SUCCEEDED(hr)) {
            WCHAR wszName[MAX_PATH];
            hr = StrRetToBufW(&strRet, pidl, wszName, MAX_PATH);
            if (SUCCEEDED(hr)) {
                int len = WideCharToMultiByte(CP_UTF8, 0, wszName, -1, nullptr, 0, nullptr, nullptr);
                if (len > 0) {
                    displayName.resize(len - 1);
                    WideCharToMultiByte(CP_UTF8, 0, wszName, -1, &displayName[0], len, nullptr, nullptr);
                }
            }
        }

        // 2. 读取 parsing name（SHGDN_FORPARSING）
        ZeroMemory(&strRet, sizeof(strRet));
        hr = spShellFolder->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strRet);
        if (SUCCEEDED(hr)) {
            WCHAR wszName[MAX_PATH];
            hr = StrRetToBufW(&strRet, pidl, wszName, MAX_PATH);
            if (SUCCEEDED(hr)) {
                int len = WideCharToMultiByte(CP_UTF8, 0, wszName, -1, nullptr, 0, nullptr, nullptr);
                if (len > 0) {
                    parsingName.resize(len - 1);
                    WideCharToMultiByte(CP_UTF8, 0, wszName, -1, &parsingName[0], len, nullptr, nullptr);
                }
            }
        }

        // 3. 判断是否为文件系统项（通过 SFGAO_FILESYSTEM 属性）
        SFGAOF sfgao = SFGAO_FILESYSTEM;
        hr = spShellFolder->GetAttributesOf(1, &pidl, &sfgao);
        if (SUCCEEDED(hr)) {
            isFileSystemItem = ((sfgao & SFGAO_FILESYSTEM) != 0);
        }

        // 4. 获取坐标
        POINT pt;
        hr = spFolderView->GetItemPosition(pidl, &pt);
        if (SUCCEEDED(hr)) {
            DesktopIcon icon;
            icon.displayName = displayName;
            icon.parsingName = parsingName;
            icon.position = pt;
            icon.isFileSystemItem = isFileSystemItem;
            snapshot.icons.push_back(icon);

            Logger::getInstance().debug(
                "DesktopIconAccessor: Read icon - displayName: '%s', parsingName: '%s', position: (%d, %d), isFileSystemItem: %s",
                displayName.c_str(),
                parsingName.c_str(),
                pt.x,
                pt.y,
                isFileSystemItem ? "true" : "false"
            );
        } else {
            Logger::getInstance().warning(
                "DesktopIconAccessor: GetItemPosition failed for '%s' (HRESULT: 0x%08lX)",
                displayName.c_str(),
                hr
            );
        }

        // 释放 PIDL
        if (pidl) {
            CoTaskMemFree(pidl);
            pidl = nullptr;
        }
    }
    
    // 11. 检查是否成功读取到图标
    if (snapshot.icons.empty()) {
        snapshot.errorMessage = "未读取到任何桌面图标";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 统计 parsingName 获取情况
    size_t parsingNameCount = 0;
    for (const auto& icon : snapshot.icons) {
        if (!icon.parsingName.empty()) {
            parsingNameCount++;
        }
    }

    Logger::getInstance().info(
        "DesktopIconAccessor: COM route succeeded - read %zu icons, %zu with parsingName",
        snapshot.icons.size(),
        parsingNameCount
    );
    
    return snapshot;
}

//=============================================================================
// 备用路线：SysListView32 跨进程消息读取
//=============================================================================

DesktopIconSnapshot DesktopIconAccessor::readUsingListView() {
    DesktopIconSnapshot snapshot;
    snapshot.method = AccessMethod::ListViewCrossProcess;
    
    // 1. 查找 Progman 窗口
    HWND hwndProgman = FindWindowW(L"Progman", L"Program Manager");
    if (!hwndProgman) {
        snapshot.errorMessage = "未找到 Progman 窗口";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 2. 查找 DefView 窗口（参考 PoC：遍历所有 WorkerW 查找 DefView）
    HWND hwndDefView = FindWindowExW(hwndProgman, nullptr, L"SHELLDLL_DefView", nullptr);
    if (!hwndDefView) {
        // 遍历所有 WorkerW 窗口查找 DefView
        HWND hwndWorkerW = FindWindowW(L"WorkerW", nullptr);
        while (hwndWorkerW) {
            hwndDefView = FindWindowExW(hwndWorkerW, nullptr, L"SHELLDLL_DefView", nullptr);
            if (hwndDefView) {
                break;
            }
            hwndWorkerW = FindWindowExW(nullptr, hwndWorkerW, L"WorkerW", nullptr);
        }
        
        if (!hwndDefView) {
            snapshot.errorMessage = "未找到 DefView 窗口";
            snapshot.method = AccessMethod::Failed;
            return snapshot;
        }
    }
    
    // 3. 查找 SysListView32 窗口
    HWND hwndListView = FindWindowExW(hwndDefView, nullptr, L"SysListView32", L"FolderView");
    if (!hwndListView) {
        snapshot.errorMessage = "未找到 SysListView32 窗口";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 4. 获取图标总数
    LRESULT iconCount = SendMessageW(hwndListView, LVM_GETITEMCOUNT, 0, 0);
    if (iconCount == 0) {
        snapshot.errorMessage = "桌面图标数为 0";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 5. 获取 Explorer 进程句柄
    DWORD explorerPid = 0;
    GetWindowThreadProcessId(hwndListView, &explorerPid);
    if (explorerPid == 0) {
        snapshot.errorMessage = "无法获取 Explorer 进程 ID";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 6. 打开 Explorer 进程
    HANDLE hProcess = OpenProcess(
        PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
        FALSE,
        explorerPid
    );
    if (!hProcess) {
        DWORD dwError = GetLastError();
        snapshot.errorMessage = "无法打开 Explorer 进程 (错误代码: " + std::to_string(dwError) + ")";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 7. 在 Explorer 进程中分配 POINT 内存
    POINT* pRemotePoint = static_cast<POINT*>(
        VirtualAllocEx(hProcess, nullptr, sizeof(POINT), MEM_COMMIT, PAGE_READWRITE)
    );
    if (!pRemotePoint) {
        DWORD dwError = GetLastError();
        snapshot.errorMessage = "无法在 Explorer 进程中分配内存 (错误代码: " + std::to_string(dwError) + ")";
        CloseHandle(hProcess);
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    // 8. 读取每个图标的坐标
    int successCount = 0;
    for (int i = 0; i < static_cast<int>(iconCount); i++) {
        // 发送 LVM_GETITEMPOSITION 消息
        LRESULT result = SendMessageW(hwndListView, LVM_GETITEMPOSITION, i, reinterpret_cast<LPARAM>(pRemotePoint));
        if (result != 1) {
            Logger::getInstance().warning(
                "DesktopIconAccessor: LVM_GETITEMPOSITION failed for icon index %d",
                i
            );
            continue;
        }
        
        // 读取远程内存中的坐标
        POINT localPoint;
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess, pRemotePoint, &localPoint, sizeof(POINT), &bytesRead)) {
            Logger::getInstance().warning(
                "DesktopIconAccessor: ReadProcessMemory failed for icon index %d (error: %lu)",
                i, GetLastError()
            );
            continue;
        }
        
        // 验证坐标是否有效（非零且在合理范围内）
        if (localPoint.x == 0 && localPoint.y == 0) {
            Logger::getInstance().debug(
                "DesktopIconAccessor: Icon at index %d has invalid position (0, 0), skipping",
                i
            );
            continue;
        }
        
        successCount++;
    }
    
    // 9. 释放远程内存
    VirtualFreeEx(hProcess, pRemotePoint, 0, MEM_RELEASE);
    
    // 10. 关闭进程句柄
    CloseHandle(hProcess);
    
    // 11. 检查是否成功读取到图标
    // 
    // v1 契约明确：displayName 必须是"图标显示名称（UTF-8，已验证）"
    // ListView 路线无法获取真实 displayName，与契约不符
    // 因此本路线不会作为成功快照返回，仅用于诊断和部分能力验证
    if (successCount > 0) {
        snapshot.errorMessage = "ListView 路线可读取坐标但无法获取真实图标名称，不符合 v1 契约要求";
        snapshot.method = AccessMethod::Failed;
        return snapshot;
    }
    
    snapshot.errorMessage = "未读取到任何有效的桌面图标";
    snapshot.method = AccessMethod::Failed;
    return snapshot;
}

//=============================================================================
// 公开接口实现
//=============================================================================

DesktopIconSnapshot DesktopIconAccessor::readDesktopIcons() {
    Logger::getInstance().info("DesktopIconAccessor: Starting to read desktop icons");
    
    // 优先走 COM 路线
    DesktopIconSnapshot snapshot = readUsingCOMInterface();
    if (snapshot.success()) {
        return snapshot;
    }
    
    // COM 失败，保存错误信息
    std::string comError = snapshot.errorMessage;
    
    // 回退到 ListView 路线
    Logger::getInstance().warning(
        "DesktopIconAccessor: COM route failed, falling back to ListView route - %s",
        comError.c_str()
    );
    
    snapshot = readUsingListView();
    if (snapshot.success()) {
        return snapshot;
    }
    
    // 两种方法均失败，合并错误信息
    std::string listViewError = snapshot.errorMessage;
    std::string combinedError = "COM 路线失败: " + comError + " | ListView 路线失败: " + listViewError;
    
    Logger::getInstance().error(
        "DesktopIconAccessor: Both COM and ListView routes failed - %s",
        combinedError.c_str()
    );
    
    snapshot.errorMessage = combinedError;
    return snapshot;
}

bool DesktopIconAccessor::isDesktopAccessible() const {
    // 查找 Progman 窗口
    HWND hwndProgman = FindWindowW(L"Progman", L"Program Manager");
    if (!hwndProgman) {
        return false;
    }
    
    // 查找 DefView 窗口（参考 PoC：遍历所有 WorkerW 查找 DefView）
    HWND hwndDefView = FindWindowExW(hwndProgman, nullptr, L"SHELLDLL_DefView", nullptr);
    if (!hwndDefView) {
        // 遍历所有 WorkerW 窗口查找 DefView
        HWND hwndWorkerW = FindWindowW(L"WorkerW", nullptr);
        while (hwndWorkerW) {
            hwndDefView = FindWindowExW(hwndWorkerW, nullptr, L"SHELLDLL_DefView", nullptr);
            if (hwndDefView) {
                break;
            }
            hwndWorkerW = FindWindowExW(nullptr, hwndWorkerW, L"WorkerW", nullptr);
        }
        
        if (!hwndDefView) {
            return false;
        }
    }
    
    // 查找 SysListView32 窗口
    HWND hwndListView = FindWindowExW(hwndDefView, nullptr, L"SysListView32", L"FolderView");
    if (!hwndListView) {
        return false;
    }
    
    return true;
}

} // namespace ccdesk::core
