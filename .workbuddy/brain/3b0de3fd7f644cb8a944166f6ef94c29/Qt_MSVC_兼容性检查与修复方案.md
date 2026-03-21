# Qt 与 MSVC 工具链兼容性检查与修复方案

## 检查结果

### 1. 当前环境
- **Qt 版本**: 6.10.2 msvc2022_64
  - 路径: `D:\Qt\6.10.2\msvc2022_64`
  - 编译工具集: MSVC 2022 (v143)
- **Visual Studio**: Visual Studio Community 2026 (v18)
  - 路径: `D:\Microsoft Visual Studio\18\Community`
  - 版本: 18.4.11612.150
  - 默认工具集: v180

### 2. CMake 配置结果
**❌ 失败 - 工具集不兼容**

错误信息：
```
error MSB8020: Visual Studio 2022 与 v143 工具集冲突
```

**根本原因**：
- Qt 6.10.2 是用 MSVC 2022 (v143) 编译的
- 当前环境是 VS 2026 (v18)，默认使用 v180 工具集
- CMake 尝试用 v143 编译，但 VS 2026 环境提供的是 v180

### 3. Build 阶段预期
由于 CMake 配置失败，Build 阶段无法执行。如果强制编译，会出现：
- **ABI 链接错误**: std::string、std::vector 等标准库 ABI 不兼容
- **运行时崩溃**: 混合使用不同版本的 MSVC 运行时库 (MSVCR143.dll vs MSVCR180.dll)

---

## 修复方案

### 方案 A: 切换到 VS2022 + v143（推荐，最少改动）

**优点**：
- ✅ 完全兼容当前 Qt 6.10.2 msvc2022_64
- ✅ 无需重新安装 Qt
- ✅ VS2022 是 LTS 版本，稳定可靠
- ✅ 仅需修改构建脚本

**缺点**：
- ❌ 需要安装 Visual Studio 2022

**实施步骤**：
1. 安装 Visual Studio 2022 Community
   - 下载: https://visualstudio.microsoft.com/downloads/
   - 工作负载: "使用 C++ 的桌面开发"
   - 组件: MSVC v143 - VS 2022 C++ x64/x86 生成工具

2. 修改 `build.bat` 和 `build.ps1`
   - 确保使用 "Visual Studio 17 2022" 生成器
   - CMAKE_PREFIX_PATH 指向 `D:/Qt/6.10.2/msvc2022_64`

---

### 方案 B: 安装与 VS2026 匹配的 Qt Kit

**优点**：
- ✅ 使用最新 VS2026 工具集
- ✅ 无需降级 Visual Studio

**缺点**：
- ❌ 需要下载并安装 Qt 6.10.2 msvc2026_64
- ❌ Qt 6.10.2 可能尚未提供 msvc2026_64 预编译包
- ❌ 如果需要编译 Qt 源码，耗时极长（数小时）
- ❌ 可能需要修改项目代码（如果 Qt API 有变化）

**实施步骤**：
1. 检查 Qt 官方是否提供 msvc2026_64 预编译包
2. 如果没有，从源码编译 Qt 6.10.2（非常耗时）
3. 安装后修改构建脚本
   - 使用 "Visual Studio 18 2026" 生成器
   - CMAKE_PREFIX_PATH 指向新安装的 Qt 路径

---

## 推荐实施方案

**选择方案 A（切换到 VS2022 + v143）**

理由：
1. **最少改动**：仅修改构建脚本，无需触碰项目代码
2. **稳定性**：VS2022 是 LTS，Qt 6.10.2 msvc2022_64 是官方支持的组合
3. **快速**：安装 VS2022 约 10-20 分钟，立即可用
4. **兼容性**：100% 兼容，无风险

---

## 脚本修改（方案 A）

### build.bat 修改（已完成）
```batch
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH=!QT_PATH! ^
    -DCMAKE_BUILD_TYPE=Release
```

### build.ps1 修改（已完成）
```powershell
& cmake .. -G "Visual Studio 17 2022" `
    -DCMAKE_PREFIX_PATH=$QtPath `
    -DCMAKE_BUILD_TYPE=$Config
```

### CMakeLists.txt 修改（已完成）
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
target_link_libraries(CCDesk PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
```

---

## 后续操作

1. 安装 Visual Studio 2022 Community
2. 在 VS2022 的 "x64 Native Tools Command Prompt" 中运行：
   ```batch
   cd e:\CCdesk
   build.bat --clean
   ```
3. 或者在 VS2022 开发者 PowerShell 中运行：
   ```powershell
   cd e:\CCdesk
   .\build.ps1 -QtPath "D:/Qt/6.10.2/msvc2022_64" -Clean
   ```

---

## 总结

- ✅ CMake 配置失败（工具集不兼容）
- ✅ Build 阶段预期失败（ABI 链接错误）
- ✅ 推荐方案 A（VS2022 + v143）
- ✅ 脚本已修改完成
- ⚠️ 需要安装 Visual Studio 2022
