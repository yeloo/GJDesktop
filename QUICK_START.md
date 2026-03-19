# CCDesk 编译快速启动指南

## 🚀 5 分钟快速上手

### 前提条件检查

在编译前，请确保已安装以下工具：

```powershell
# 验证 CMake
cmake --version
# 预期: cmake version 3.20 或更高

# 验证 Visual Studio
msbuild --version
# 预期: MSBuild version 17.x 或更高

# 验证 Qt
echo $env:QT_PATH
# 预期: C:\Qt\6.4\msvc2019_64 (或类似路径)
```

如果任何工具缺失，请参考 `COMPILATION_STATUS.md` 进行安装。

---

## 方式 A: 使用 PowerShell 脚本（推荐）

### 第 1 步: 设置 Qt 路径

```powershell
# 设置环境变量 (仅需一次)
$env:QT_PATH = "C:\Qt\6.4\msvc2019_64"

# 或使用 setx 永久设置
setx QT_PATH "C:\Qt\6.4\msvc2019_64"

# 验证
echo $env:QT_PATH
```

### 第 2 步: 运行编译脚本

```powershell
cd e:\CCdesk
.\build.ps1
```

### 预期输出

```
╔══════════════════════════════════════════╗
║      CCDesk Compilation Script          ║
║      Qt Desktop Organizer (MVP v1.0)    ║
╚══════════════════════════════════════════╝

[HH:mm:ss] * Build Configuration
  Qt Path:          C:\Qt\6.4\msvc2019_64
  Configuration:    Release
  Parallel Jobs:    8
  Clean Build:      False

[HH:mm:ss] → Checking build tools...
[HH:mm:ss] ✓ CMake found: cmake version 3.30.0
[HH:mm:ss] ✓ MSBuild found: MSBuild version 17.8.3

[HH:mm:ss] → Preparing build environment...
[HH:mm:ss] → Running CMake configuration...
...
[HH:mm:ss] ✓ Build completed successfully!
  Executable:     bin\CCDesk.exe
  File Size:      12.34 MB
```

### 清理旧编译并重新编译

```powershell
.\build.ps1 -Clean
```

---

## 方式 B: 使用批处理脚本

### 第 1 步: 设置环境变量

```powershell
# 永久设置 (仅需一次)
setx QT_PATH "C:\Qt\6.4\msvc2019_64"

# 重启 PowerShell 或命令行窗口使设置生效
```

### 第 2 步: 运行编译脚本

```bash
cd e:\CCdesk
build.bat
```

### 或使用清理编译

```bash
build.bat --clean
```

---

## 方式 C: 手动编译（高级用户）

### 第 1 步: 进入项目目录

```powershell
cd e:\CCdesk
```

### 第 2 步: 创建编译目录

```powershell
mkdir build -Force
cd build
```

### 第 3 步: 运行 CMake 配置

```powershell
cmake .. -G "Visual Studio 17 2022" `
    -DCMAKE_PREFIX_PATH=C:\Qt\6.4\msvc2019_64 `
    -DCMAKE_BUILD_TYPE=Release
```

### 第 4 步: 执行编译

```powershell
cmake --build . --config Release --parallel 8
```

### 第 5 步: 验证结果

```powershell
ls bin\CCDesk.exe
# 应该显示文件，大小约 5-15 MB
```

---

## ✅ 编译成功标志

编译完成后，应该看到：

✓ 编译消息显示 "Build succeeded"  
✓ `e:\CCdesk\build\bin\CCDesk.exe` 文件存在  
✓ 文件大小为 5-15 MB  
✓ 文件时间戳为最近的编译时间  

---

## 🚀 编译后运行

### 第 1 步: 直接运行

```powershell
cd e:\CCdesk\build\bin
.\CCDesk.exe
```

### 第 2 步: 验证应用启动

应该看到：
- ✓ 蓝色主窗口显示 "CCDesk - Desktop Organizer"
- ✓ 右下角系统托盘显示蓝色图标
- ✓ logs/ccdesk.log 文件被创建
- ✓ config/ccdesk_config.json 文件被创建

### 第 3 步: 查看日志

```powershell
Get-Content e:\CCdesk\logs\ccdesk.log -Tail 20
```

应该显示：
```
[INFO] ==================================================
[INFO] CCDesk application started
[INFO] ==================================================
[INFO] AppManager: Initialization complete
...
```

---

## 🐛 编译出错快速修复

### 问题 1: "cmake: 无法将"cmake"项识别"

**原因**: CMake 不在 PATH 中  
**解决**:
```powershell
# 安装 CMake 后重启系统，或手动添加到 PATH
$env:PATH += ";C:\Program Files\CMake\bin"
cmake --version
```

### 问题 2: "Visual Studio 17 2022 not found"

**原因**: Visual Studio 2022 未安装或版本不对  
**解决**:
```powershell
# 检查已安装的 Visual Studio 版本
ls "C:\Program Files\Microsoft Visual Studio"

# 使用正确的生成器
# Visual Studio 2019: -G "Visual Studio 16 2019"
# Visual Studio 2022: -G "Visual Studio 17 2022"
```

### 问题 3: "Could not find Qt6 package"

**原因**: Qt 路径不正确或未安装  
**解决**:
```powershell
# 检查 Qt 路径
ls $env:QT_PATH
# 应该显示: bin, lib, include 等目录

# 设置正确的路径
$env:QT_PATH = "C:\Qt\6.4\msvc2019_64"

# 清除旧的编译缓存
rmdir build -Recurse -Force
# 重新编译
```

### 问题 4: "Build failed" (编译失败)

**解决步骤**:
```powershell
# 1. 查看详细错误
cd build
cmake --build . --config Release --verbose

# 2. 如果还有问题，清理后重新编译
cd ..
rmdir build -Recurse -Force
mkdir build
cd build

# 3. 重新配置和编译
cmake .. -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH=$env:QT_PATH
cmake --build . --config Release
```

---

## 📦 部署 Qt 运行库

编译成功后，需要部署 Qt 库才能在其他电脑上运行：

```powershell
cd e:\CCdesk\build\bin

# 使用 Qt 提供的部署工具
$QtBin = "C:\Qt\6.4\msvc2019_64\bin"
& "$QtBin\windeployqt.exe" CCDesk.exe --release

# 这会自动复制所有必需的 DLL:
# - Qt6Core.dll
# - Qt6Gui.dll
# - Qt6Widgets.dll
# - platforms\qwindows.dll
# - 其他依赖库
```

部署后的目录结构：
```
build\bin\
├── CCDesk.exe                    (主程序)
├── Qt6Core.dll                  (Qt 核心库)
├── Qt6Gui.dll                   (Qt GUI 库)
├── Qt6Widgets.dll               (Qt 控件库)
├── platforms\
│   └── qwindows.dll             (Windows 平台插件)
└── ... (其他依赖库)
```

---

## 📋 编译命令速查表

| 任务 | 命令 |
|------|------|
| **快速编译** | `.\build.ps1` |
| **清理后重新编译** | `.\build.ps1 -Clean` |
| **手动配置** | `cmake .. -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH=$env:QT_PATH` |
| **编译** | `cmake --build . --config Release --parallel 8` |
| **清理旧编译** | `rmdir build -Recurse -Force` |
| **查看编译输出** | `cmake --build . --verbose` |
| **部署库** | `$QtBin\windeployqt.exe bin\CCDesk.exe --release` |
| **运行程序** | `.\build\bin\CCDesk.exe` |
| **查看日志** | `Get-Content logs\ccdesk.log -Tail 50` |

---

## 🎯 编译清单

### 编译前
- [ ] CMake 3.20+ 已安装
- [ ] Visual Studio 2022 已安装
- [ ] Qt 6.4 已安装
- [ ] `$env:QT_PATH` 已设置
- [ ] 项目文件完整 (ls CMakeLists.txt, src\main.cpp)

### 编译中
- [ ] 无 CMake 配置错误
- [ ] 无编译错误 (Build succeeded)
- [ ] 编译时间合理 (< 1 分钟)

### 编译后
- [ ] CCDesk.exe 存在
- [ ] 文件大小 5-15 MB
- [ ] 应用可以运行
- [ ] 日志文件生成
- [ ] 配置文件生成

---

## 📞 需要帮助？

### 查看详细文档
- `COMPILATION_STATUS.md` - 完整编译指南和故障排查
- `PHASE6_BUILD_GUIDE.md` - 部署和运行指南
- `PHASE6_SUMMARY.md` - 项目完整总结

### 常见问题查询
1. 工具安装问题 → `COMPILATION_STATUS.md` 第 1 节
2. CMake 配置问题 → `COMPILATION_STATUS.md` 第 3 节
3. 编译错误 → `COMPILATION_STATUS.md` 第 4 节
4. 运行问题 → `PHASE6_BUILD_GUIDE.md` 第 7 节

---

## ⏱️ 预计时间

| 步骤 | 时间 |
|------|------|
| 设置环境变量 | 1 分钟 |
| CMake 配置 | 1-2 分钟 |
| 编译 | 10-30 秒 |
| Qt 部署 | 1-2 分钟 |
| **总计** | **5-10 分钟** |

---

**准备好了？** 🚀

```powershell
# 一行命令编译
cd e:\CCdesk; .\build.ps1
```

---

*最后更新: 2026-03-19*  
*项目版本: MVP v1.0-phase6*
