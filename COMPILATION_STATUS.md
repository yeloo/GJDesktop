# CCDesk 编译状态报告

## 🔴 编译环境检查结果

### 当前系统状态
- **操作系统**: Windows 
- **编译工具检查**:
  - ❌ CMake - **未安装**（在 PATH 中找不到）
  - ❌ MSBuild - **未安装**（Visual Studio 2019/2022 编译器）
  - ❌ Ninja - **未安装**
  - ❌ GCC/MinGW - **未安装**

- **编译必需库**:
  - ❓ Qt 6.x - **未验证**（需要 CMAKE_PREFIX_PATH 指定）

---

## ⚠️ 编译前置要求

### 必须安装的工具

#### 1. CMake (3.20+)
```bash
# 选项 A: 使用 Windows 安装程序
# 下载: https://cmake.org/download/
# 选择: cmake-3.30.0-windows-x86_64.msi (或更新版本)
# 安装步骤:
#   1. 运行 .msi 文件
#   2. 选择 "Add CMake to the system PATH"
#   3. 完成安装

# 选项 B: 使用 Chocolatey
choco install cmake --install-arguments='"ADD_CMAKE_TO_PATH=System"'

# 安装后验证
cmake --version  # 应显示 3.20+
```

#### 2. Visual Studio 2019 或 2022
```bash
# 选项 A: 官方下载
# https://visualstudio.microsoft.com/downloads/
# 选择 "Visual Studio Community" (免费)
# 安装时勾选:
#   - Desktop development with C++
#   - MSVC v143 Compiler
#   - Windows SDK

# 选项 B: 使用 Visual Studio Build Tools
# https://visualstudio.microsoft.com/visual-cpp-build-tools/

# 安装后验证
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
msbuild --version  # 应显示版本信息
```

#### 3. Qt 6.x (6.2+)
```bash
# 选项 A: 使用 Qt 在线安装程序
# https://www.qt.io/download-qt-installer-oss
# 安装步骤:
#   1. 下载并运行 qt-unified-windows-x64-4.8.0-online.exe
#   2. 注册/登录 Qt 账户
#   3. 选择 "Custom installation"
#   4. 选择 Qt 6.4 LTS (或更新的 6.x 版本)
#   5. 勾选 MSVC 2019/2022 64-bit 编译工具包
#   6. 完成安装 (约 10-15 GB)

# 选项 B: 使用 Chocolatey (不推荐，版本可能旧)
choco install qt

# 安装后设置环境变量
setx QT_PATH "C:\Qt\6.4\msvc2019_64"
setx CMAKE_PREFIX_PATH "%QT_PATH%"

# 验证
echo %QT_PATH%
```

---

## 📋 编译步骤 (完整指南)

### 第 1 步: 验证所有工具已安装

```powershell
# 验证 CMake
cmake --version
# 预期输出: cmake version 3.20+

# 验证 Visual Studio
where msbuild
# 预期输出: C:\Program Files\...\MSBuild\...\bin\MSBuild.exe

# 验证 Qt
echo $env:QT_PATH
# 预期输出: C:\Qt\6.4\msvc2019_64
```

### 第 2 步: 准备编译目录

```powershell
cd e:\CCdesk

# 创建编译目录
mkdir build -Force
cd build
```

### 第 3 步: 运行 CMake 配置

```powershell
# 方法 A: Visual Studio 16 2019
cmake .. -G "Visual Studio 16 2019" `
    -DCMAKE_PREFIX_PATH=C:\Qt\6.4\msvc2019_64 `
    -DCMAKE_BUILD_TYPE=Release

# 方法 B: Visual Studio 17 2022 (推荐)
cmake .. -G "Visual Studio 17 2022" `
    -DCMAKE_PREFIX_PATH=C:\Qt\6.4\msvc2019_64 `
    -DCMAKE_BUILD_TYPE=Release

# 方法 C: 使用环境变量 (若已设置)
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
```

**预期输出**:
```
-- The CXX compiler identification is MSVC 19.XX.XXXXX
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Configuring done
-- Generating done
```

### 第 4 步: 执行编译

```powershell
# 使用 cmake 编译 (推荐，自动检测核心数)
cmake --build . --config Release --parallel 4

# 或使用 msbuild 直接编译
msbuild CCDesk.sln /p:Configuration=Release /m:4
```

**预期输出** (最后几行):
```
Build succeeded.
    Time Elapsed 00:00:XX
```

### 第 5 步: 验证编译成果

```powershell
# 检查可执行文件
dir bin\CCDesk.exe

# 预期输出:
#   Mode                 LastWriteTime         Length Name
#   ----                 -------------         ------ ----
#   -a----         2026/3/XX     XX:XX        XXXXX  CCDesk.exe

# 检查文件大小 (应该 5-15 MB)
# 检查时间戳 (应该是刚才的编译时间)
```

---

## 🔧 快速编译脚本

### build.bat (Windows 批处理)

将以下内容保存为 `build.bat`，放在项目根目录：

```batch
@echo off
REM CCDesk 编译脚本
setlocal enabledelayedexpansion

echo ========================================
echo   CCDesk Compilation Script
echo ========================================
echo.

REM 检查环境变量
if not defined QT_PATH (
    echo [!] Qt path not set. Please set QT_PATH environment variable first:
    echo     setx QT_PATH "C:\Qt\6.4\msvc2019_64"
    pause
    exit /b 1
)

echo [*] Qt Path: %QT_PATH%
echo [*] Checking for build directory...

if not exist build (
    echo [+] Creating build directory...
    mkdir build
) else (
    echo [*] build directory already exists
)

cd build

echo.
echo [*] Running CMake configuration...
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH=%QT_PATH% ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo [-] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [*] Starting build (Release mode, 4 jobs)...
cmake --build . --config Release --parallel 4

if errorlevel 1 (
    echo [-] Build failed!
    pause
    exit /b 1
)

echo.
echo [+] Build completed successfully!
echo [+] Executable: bin\CCDesk.exe
echo.
pause
```

### build.ps1 (PowerShell 脚本)

将以下内容保存为 `build.ps1`，放在项目根目录：

```powershell
# CCDesk 编译脚本 (PowerShell)
param(
    [string]$QtPath = $env:QT_PATH,
    [int]$Jobs = 4,
    [string]$Config = "Release"
)

if ([string]::IsNullOrEmpty($QtPath)) {
    Write-Host "[!] Qt path not provided. Use: build.ps1 -QtPath C:\Qt\6.4\msvc2019_64" -ForegroundColor Red
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  CCDesk Compilation Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "[*] Qt Path: $QtPath" -ForegroundColor Green
Write-Host "[*] Configuration: $Config" -ForegroundColor Green
Write-Host "[*] Parallel Jobs: $Jobs" -ForegroundColor Green

# 创建编译目录
if (-not (Test-Path build)) {
    Write-Host "[+] Creating build directory..."
    New-Item -ItemType Directory -Path build -Force | Out-Null
} else {
    Write-Host "[*] build directory already exists"
}

cd build

Write-Host ""
Write-Host "[*] Running CMake configuration..." -ForegroundColor Yellow
cmake .. -G "Visual Studio 17 2022" `
    -DCMAKE_PREFIX_PATH=$QtPath `
    -DCMAKE_BUILD_TYPE=$Config

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] CMake configuration failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[*] Starting build ($Config mode, $Jobs jobs)..." -ForegroundColor Yellow
cmake --build . --config $Config --parallel $Jobs

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[+] Build completed successfully!" -ForegroundColor Green
Write-Host "[+] Executable: bin\CCDesk.exe" -ForegroundColor Green
Write-Host ""

# 验证可执行文件
if (Test-Path "bin\CCDesk.exe") {
    $fileSize = (Get-Item "bin\CCDesk.exe").Length / 1MB
    Write-Host "[✓] Executable size: $([math]::Round($fileSize, 2)) MB" -ForegroundColor Green
} else {
    Write-Host "[-] Executable not found!" -ForegroundColor Red
    exit 1
}
```

**使用方式**:
```powershell
# 运行脚本
.\build.ps1 -QtPath "C:\Qt\6.4\msvc2019_64" -Jobs 4
```

---

## 🚨 常见编译问题排查

### 问题 1: CMake 找不到

```
CMake Error at CMakeLists.txt:11 (find_package):
  By not providing "FindQt6.cmake" in CMAKE_MODULE_PATH this project has
  asked CMake to find a package configuration file provided by "Qt6"...
```

**解决方案**:
```powershell
# 确保 Qt 路径正确
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.4\msvc2019_64

# 或设置环境变量
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.4\msvc2019_64"
cmake ..
```

### 问题 2: MSVC 编译器版本不匹配

```
CMake Error: Visual Studio 17 2022 not found
```

**解决方案**:
```powershell
# 检查已安装的 Visual Studio 版本
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

# 使用正确的生成器
# Visual Studio 2019: -G "Visual Studio 16 2019"
# Visual Studio 2022: -G "Visual Studio 17 2022"
cmake .. -G "Visual Studio 16 2019" -DCMAKE_PREFIX_PATH=...
```

### 问题 3: advapi32 链接错误

```
link : fatal error LNK1104: cannot open file 'advapi32.lib'
```

**解决方案**:
- 确保已安装 Windows SDK
- 在 Visual Studio 中确认 Windows SDK 版本
- 重新运行 CMake 配置

### 问题 4: Qt DLL 找不到

```
CCDesk.exe - 找不到指定的模块
```

**解决方案**:
```powershell
# 部署 Qt 运行库
cd build\bin
windeployqt.exe CCDesk.exe --release

# 这会自动复制必要的 DLL:
# - Qt6Core.dll
# - Qt6Gui.dll
# - Qt6Widgets.dll
# - platforms\qwindows.dll
```

---

## 📦 编译后步骤

### 1. 生成发布包

```powershell
cd e:\CCdesk\build\bin

# 使用 Qt 提供的部署工具
$QtBinPath = "C:\Qt\6.4\msvc2019_64\bin"
& "$QtBinPath\windeployqt.exe" CCDesk.exe --release --dir ..\release_package
```

### 2. 运行测试

```powershell
# 直接运行可执行文件
.\CCDesk.exe

# 查看日志
Get-Content ..\logs\ccdesk.log -Tail 50
```

### 3. 清理编译产物

```powershell
cd e:\CCdesk
Remove-Item -Recurse -Force build
# 这会删除编译临时文件，保留源代码
```

---

## 📋 编译检查清单

### 编译前
- [ ] CMake 3.20+ 已安装 (`cmake --version`)
- [ ] Visual Studio 2019/2022 已安装 (`msbuild --version` 或检查安装)
- [ ] Qt 6.x (6.2+) 已安装 (`%QT_PATH%` 环境变量已设置)
- [ ] 项目文件完整 (CMakeLists.txt, src/ 目录存在)

### 编译中
- [ ] CMake 配置成功 (无 "Error" 输出)
- [ ] 编译开始 (显示 "Starting build...")
- [ ] 无编译错误 (build succeeded)

### 编译后
- [ ] bin\CCDesk.exe 存在
- [ ] 文件大小 5-15 MB
- [ ] 时间戳是最近的编译时间
- [ ] 使用 windeployqt 部署依赖库

---

## 📞 需要帮助？

### 如果编译失败:
1. 记录完整的错误信息
2. 检查 COMPILATION_STATUS.md 中的 FAQ
3. 验证所有环境变量是否正确
4. 尝试清除编译目录后重新编译

### 推荐资源:
- CMake 官方文档: https://cmake.org/documentation/
- Qt 官方文档: https://doc.qt.io/qt-6/
- Visual Studio 文档: https://docs.microsoft.com/en-us/visualstudio/

---

**生成时间**: 2026-03-19  
**项目版本**: MVP v1.0-phase6  
**编译工具要求**: CMake 3.20+, Visual Studio 2019+, Qt 6.2+
