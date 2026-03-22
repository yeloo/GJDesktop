#include "desktop_icon_writer.h"
#include "logger.h"

// Windows 头文件
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <atlbase.h>

namespace ccdesk::core {

//=============================================================================
// 主路线：IFolderView COM 接口写回（实现中）
//=============================================================================

bool DesktopIconWriter::writeUsingCOMInterface(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    Logger::getInstance().info(
        "DesktopIconWriter: COM write route - displayName: '%s', parsingName: '%s', target: (%d, %d)",
        identity.displayName.c_str(),
        identity.parsingName.c_str(),
        targetPosition.x,
        targetPosition.y
    );

    // 检查 parsingName 是否有效
    if (identity.parsingName.empty()) {
        errorMessage = "parsingName 为空，无法定位图标";
        Logger::getInstance().error("DesktopIconWriter: %s", errorMessage.c_str());
        return false;
    }

    HRESULT hr = S_OK;

    // 1. 获取桌面 Shell Folder
    CComPtr<IShellFolder> pDesktopFolder;
    hr = SHGetDesktopFolder(&pDesktopFolder);
    if (FAILED(hr) || !pDesktopFolder) {
        errorMessage = "无法获取桌面 Shell Folder";
        Logger::getInstance().error(
            "DesktopIconWriter: %s, HRESULT: 0x%08lX",
            errorMessage.c_str(),
            hr
        );
        return false;
    }

    Logger::getInstance().debug("DesktopIconWriter: 获取桌面 Shell Folder 成功");

    // 2. 将 parsingName 转换为 PIDL
    // parsingName 已经是完整路径（例如 C:\Users\Administrator\Desktop\文档.docx）
    ULONG chEaten = 0;
    LPITEMIDLIST pidl = nullptr;
    DWORD dwAttributes = 0;

    // 【诊断】详细记录 parsingName 内容和长度
    Logger::getInstance().debug(
        "DesktopIconWriter: 准备解析 parsingName - 长度: %zu, 内容: '%s'",
        identity.parsingName.length(),
        identity.parsingName.c_str()
    );

    // 将 UTF-8 parsingName 转换为 UTF-16
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, identity.parsingName.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) {
        DWORD err = GetLastError();
        errorMessage = "parsingName 转换为 UTF-16 失败 (错误码: " + std::to_string(err) + ")";
        Logger::getInstance().error("DesktopIconWriter: %s, parsingName: '%s'", errorMessage.c_str(), identity.parsingName.c_str());
        return false;
    }

    std::wstring wideParsingName(wideLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, identity.parsingName.c_str(), -1, &wideParsingName[0], wideLen);

    Logger::getInstance().debug(
        "DesktopIconWriter: parsingName UTF-16 转换完成 - 宽字符长度: %d",
        wideLen
    );

    hr = pDesktopFolder->ParseDisplayName(
        nullptr,           // hwndOwner (nullptr 表示无父窗口)
        nullptr,           // pbc (绑定上下文，nullptr 表示无)
        &wideParsingName[0], // pszDisplayName (parsingName)
        &chEaten,           // pchEaten (已解析的字符数，输出参数)
        &pidl,             // ppidl (返回的 PIDL，输出参数)
        &dwAttributes      // pdwAttributes (属性，输入输出参数)
    );

    if (FAILED(hr) || !pidl) {
        char errorBuffer[256];
        snprintf(errorBuffer, sizeof(errorBuffer), "无法将 parsingName 转换为 PIDL, HRESULT: 0x%08lX", hr);
        errorMessage = errorBuffer;
        Logger::getInstance().error(
            "DesktopIconWriter: %s - displayName: '%s', parsingName: '%s'",
            errorMessage.c_str(),
            identity.displayName.c_str(),
            identity.parsingName.c_str()
        );
        return false;
    }

    Logger::getInstance().debug("DesktopIconWriter: parsingName 转换为 PIDL 成功");

    // 使用智能指针管理 PIDL 生命周期
    struct PIDLDeleter {
        void operator()(LPITEMIDLIST p) const { ILFree(p); }
    };
    std::unique_ptr<ITEMIDLIST, PIDLDeleter> pidlGuard(pidl);

    // 3. 获取桌面窗口的 IFolderView
    // 方法1: 通过 IShellWindows -> IShellBrowser -> IShellView -> IFolderView
    CComPtr<IShellWindows> pShellWindows;
    hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pShellWindows));
    if (FAILED(hr) || !pShellWindows) {
        char errorBuffer[256];
        snprintf(errorBuffer, sizeof(errorBuffer), "无法创建 IShellWindows, HRESULT: 0x%08lX", hr);
        errorMessage = errorBuffer;
        Logger::getInstance().error("DesktopIconWriter: %s", errorMessage.c_str());
        return false;
    }

    Logger::getInstance().debug("DesktopIconWriter: 创建 IShellWindows 成功");

    // 4. 查找桌面窗口
    long hwndCount = 0;
    hr = pShellWindows->get_Count(&hwndCount);
    if (FAILED(hr)) {
        char errorBuffer[256];
        snprintf(errorBuffer, sizeof(errorBuffer), "无法获取窗口数量, HRESULT: 0x%08lX", hr);
        errorMessage = errorBuffer;
        Logger::getInstance().error("DesktopIconWriter: %s", errorMessage.c_str());
        return false;
    }

    Logger::getInstance().debug("DesktopIconWriter: ShellWindows 窗口数量: %ld", hwndCount);

    CComPtr<IShellBrowser> pShellBrowser;
    CComPtr<IFolderView> pFolderView;
    VARIANT varIndex;
    VariantInit(&varIndex);

    // 遍历所有窗口查找桌面
    for (long i = 0; i < hwndCount; i++) {
        varIndex.vt = VT_I4;
        varIndex.lVal = i;

        CComPtr<IDispatch> spDispatch;
        hr = pShellWindows->Item(varIndex, &spDispatch);
        if (FAILED(hr) || !spDispatch) {
            continue;
        }

        CComPtr<IWebBrowserApp> spWebBrowser;
        hr = spDispatch->QueryInterface(IID_PPV_ARGS(&spWebBrowser));
        if (FAILED(hr) || !spWebBrowser) {
            continue;
        }

        CComPtr<IServiceProvider> spServiceProvider;
        hr = spWebBrowser->QueryInterface(IID_PPV_ARGS(&spServiceProvider));
        if (FAILED(hr) || !spServiceProvider) {
            continue;
        }

        hr = spServiceProvider->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&pShellBrowser));
        if (FAILED(hr) || !pShellBrowser) {
            continue;
        }

        CComPtr<IShellView> pShellView;
        hr = pShellBrowser->QueryActiveShellView(&pShellView);
        if (FAILED(hr) || !pShellView) {
            continue;
        }

        hr = pShellView->QueryInterface(IID_PPV_ARGS(&pFolderView));
        if (SUCCEEDED(hr) && pFolderView) {
            Logger::getInstance().debug("DesktopIconWriter: 找到 IFolderView，窗口索引: %ld", i);
            break;
        }
    }

    VariantClear(&varIndex);

    if (!pFolderView) {
        errorMessage = "无法获取桌面 IFolderView";
        Logger::getInstance().error("DesktopIconWriter: %s", errorMessage.c_str());
        return false;
    }

    Logger::getInstance().debug("DesktopIconWriter: 获取 IFolderView 成功");

    // 5. 尝试设置图标位置
    // 使用 IFolderView::SelectAndPositionItems
    // 参数：UINT cidl, PCUITEMID_CHILD_ARRAY apidl, POINT *ppt, DWORD dwFlags
    POINT pt = targetPosition;
    LPCITEMIDLIST pidlArray[1] = { pidl };

    Logger::getInstance().debug(
        "DesktopIconWriter: 调用 SelectAndPositionItems - 目标位置: (%d, %d), PIDL: 0x%p",
        targetPosition.x,
        targetPosition.y,
        (void*)pidl
    );

    hr = pFolderView->SelectAndPositionItems(1, pidlArray, &pt, SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_POSITIONITEM);

    Logger::getInstance().info(
        "DesktopIconWriter: SelectAndPositionItems 返回 - displayName: '%s', HRESULT: 0x%08lX (%s), 目标位置: (%d, %d)",
        identity.displayName.c_str(),
        hr,
        (hr == S_OK ? "S_OK" : (hr == S_FALSE ? "S_FALSE" : "失败")),
        targetPosition.x,
        targetPosition.y
    );

    // 【关键修复】只有 hr == S_OK 才算真正成功
    // SUCCEEDED() 宏包含 S_FALSE (HRESULT 0x00000001)，这会导致假成功
    if (hr == S_OK) {
        Logger::getInstance().info(
            "DesktopIconWriter: ✅ SelectAndPositionItems 成功 - displayName: '%s', position: (%d, %d)",
            identity.displayName.c_str(),
            targetPosition.x,
            targetPosition.y
        );
        return true;
    } else if (hr == S_FALSE) {
        errorMessage = "SelectAndPositionItems 返回 S_FALSE（部分成功或被忽略）";
        Logger::getInstance().error(
            "DesktopIconWriter: ⚠️ %s - displayName: '%s', parsingName: '%s', 这可能意味着桌面布局模式不匹配",
            errorMessage.c_str(),
            identity.displayName.c_str(),
            identity.parsingName.c_str()
        );
        return false;
    }

    // 如果方法失败
    char errorBuffer[512];
    snprintf(errorBuffer, sizeof(errorBuffer), "无法设置图标位置 (SelectAndPositionItems 失败, HRESULT: 0x%08lX)", hr);
    errorMessage = errorBuffer;
    Logger::getInstance().error(
        "DesktopIconWriter: ❌ %s - displayName: '%s', parsingName: '%s'",
        errorMessage.c_str(),
        identity.displayName.c_str(),
        identity.parsingName.c_str()
    );

    return false;
}

//=============================================================================
// 备用路线：SysListView32 跨进程消息写回（诊断性降级路线）
//=============================================================================

bool DesktopIconWriter::writeUsingListView(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    Logger::getInstance().error(
        "DesktopIconWriter: ListView write route not reliable - displayName: '%s', parsingName: '%s'",
        identity.displayName.c_str(),
        identity.parsingName.c_str()
    );

    errorMessage = "真实桌面图标写回尚未可靠实现 - ListView 写回路线不可靠（仅按 displayName 查找，同名图标会出错）";
    return false;

    // 原实现已删除，原因：
    // 1. LVM_GETITEMW 跨进程传递本进程指针给 Explorer，不成立
    // 2. 仅按 displayName 查找，同名图标会出错
    // 3. 只查找 Progman，没有 WorkerW 回退逻辑
    // 4. LVM_SETITEMPOSITION 成功判定不可靠
    // 5. 不允许假成功，必须诚实返回失败
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

    Logger::getInstance().info("DesktopIconWriter: 开始批量移动 %zu 个图标（当前为降级实现）", targets.size());

    for (const auto& target : targets) {
        std::string singleError;
        bool success = moveSingleIcon(target.identity, target.targetPosition, singleError);

        if (success) {
            movedCount++;
        } else {
            failedCount++;
            Logger::getInstance().error(
                "DesktopIconWriter: 移动图标 '%s' (parsingName: '%s') 失败: %s",
                target.identity.displayName.c_str(),
                target.identity.parsingName.c_str(),
                singleError.c_str()
            );

            // 将第一个错误记录到 errorMessage
            if (errorMessage.empty()) {
                errorMessage = "图标 '" + target.identity.displayName + "' 移动失败: " + singleError;
            }
        }
    }

    Logger::getInstance().info(
        "DesktopIconWriter: 批量移动完成 - 成功: %zu, 失败: %zu（当前为降级实现，失败预期）",
        movedCount,
        failedCount
    );

    return failedCount == 0;
}

//=============================================================================
// 移动单个图标
//=============================================================================

bool DesktopIconWriter::moveSingleIcon(
    const DesktopIconIdentity& identity,
    POINT targetPosition,
    std::string& errorMessage
) {
    Logger::getInstance().info(
        "DesktopIconWriter: 开始移动图标 - displayName: '%s', parsingName: '%s', target: (%d, %d)",
        identity.displayName.c_str(),
        identity.parsingName.c_str(),
        targetPosition.x,
        targetPosition.y
    );

    // 优先使用 COM 路线（IFolderView）
    bool success = writeUsingCOMInterface(identity, targetPosition, errorMessage);

    if (success) {
        Logger::getInstance().info(
            "DesktopIconWriter: 移动成功 - displayName: '%s', position: (%d, %d)",
            identity.displayName.c_str(),
            targetPosition.x,
            targetPosition.y
        );
        return true;
    } else {
        Logger::getInstance().error(
            "DesktopIconWriter: 移动失败 - displayName: '%s', error: %s",
            identity.displayName.c_str(),
            errorMessage.c_str()
        );
        return false;
    }
}

} // namespace ccdesk::core
