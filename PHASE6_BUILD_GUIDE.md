# CCDesk Phase 6 - 编译与发布检查

## 一、编译环境要求

### 操作系统
- ✅ Windows 10 及以上
- ⚠️ Windows 7 (不推荐，需测试)
- ❌ Linux/Mac (部分功能受限)

### 编译工具链
- Visual Studio 2019+ 或 MinGW-w64
- CMake 3.20 或更高
- Qt 6.2+ (Core, Gui, Widgets 模块)
- Windows SDK 或 Platform Toolset

### 磁盘空间
- 源代码: ~500 KB
- 编译输出: ~50-100 MB
- 日志目录: 可选

---

## 二、编译步骤 (完整指南)

### 2.1 使用 CMake + Visual Studio (推荐)

```bash
# 1. 打开 PowerShell 或 cmd，进入项目目录
cd E:\CCdesk

# 2. 创建编译目录
mkdir build
cd build

# 3. 运行 CMake 配置
# 确保 Qt 环境变量已设置，或指定 Qt 路径
cmake .. -G "Visual Studio 16 2019" ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.4\msvc2019_64 ^
    -DCMAKE_BUILD_TYPE=Release

# 或使用 Visual Studio 17 2022
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.4\msvc2019_64 ^
    -DCMAKE_BUILD_TYPE=Release

# 4. 编译项目
cmake --build . --config Release --parallel 4

# 5. 检查输出
# 成功: bin\CCDesk.exe 应该存在
dir bin\CCDesk.exe
```

### 2.2 使用 Qt Creator (图形化)

```
1. 打开 Qt Creator
2. File → Open File or Project → 选择 CMakeLists.txt
3. 配置 Kit: 选择 Desktop Qt 6.x MSVC 2019/2022 64-bit
4. Build → Build All (或 Ctrl+Shift+B)
5. 运行输出在 build 目录下的 bin 文件夹

快捷方式:
- F5 编译并运行
- Ctrl+Shift+B 仅编译
```

### 2.3 命令行编译 (使用 MinGW)

```bash
# 1. 进入项目目录
cd E:\CCdesk

# 2. 创建编译目录
mkdir build & cd build

# 3. 配置 CMake (MinGW)
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.4\mingw_64 ^
    -DCMAKE_BUILD_TYPE=Release

# 4. 编译
mingw32-make -j 4

# 5. 检查结果
dir bin\CCDesk.exe
```

---

## 三、常见编译问题排查

### 问题 1: Qt 路径找不到
**错误信息**: `CMake Error: Could not find Qt6 package`  
**解决方案**:
```bash
# 确认 Qt 路径
echo %QTDIR%  # 或直接检查安装目录

# 重新配置 CMake，指定正确路径
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.x\msvc2019_64
```

### 问题 2: MSVC 编译器版本不匹配
**错误信息**: `MSBuild.exe" not found` 或 compiler version mismatch  
**解决方案**:
```bash
# 使用正确的 Visual Studio 版本
# Visual Studio 2019: "Visual Studio 16 2019"
# Visual Studio 2022: "Visual Studio 17 2022"

cmake .. -G "Visual Studio 17 2022" ...
```

### 问题 3: advapi32 链接失败
**错误信息**: `link : fatal error LNK1104: cannot open file 'advapi32.lib'`  
**解决方案**:
- 确保已安装 Windows SDK
- 检查 CMakeLists.txt 中的 `target_link_libraries(CCDesk advapi32)`
- 对于 MinGW，可能需要 `-ladvapi32` 形式

### 问题 4: C++17 特性未支持
**错误信息**: `error C2863: cannot use if with attribute` 或类似  
**解决方案**:
```cmake
# 确保 CMakeLists.txt 中有以下设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### 问题 5: 源文件编码问题 (含中文)
**错误信息**: 文件名或中文字符乱码  
**解决方案** (Visual Studio):
```bash
# 添加编码标志
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_CXX_FLAGS="/utf-8"
```

---

## 四、编译结果验证

### 4.1 检查可执行文件

```bash
# 进入编译目录
cd E:\CCdesk\build

# 检查文件大小和时间戳
dir bin\CCDesk.exe

# 预期: 文件大小 5-15 MB, 时间戳为最近编译时间
```

### 4.2 检查依赖库

```bash
# 使用 Dependency Walker 或 objdump 检查依赖
# Windows 命令行:
dumpbin /DEPENDENTS bin\CCDesk.exe | find "Qt"

# 预期输出应包含:
# - Qt6Core.dll
# - Qt6Gui.dll  
# - Qt6Widgets.dll
```

### 4.3 符号表检查 (Debug info)

```bash
# 检查是否包含调试符号
dumpbin /HEADERS bin\CCDesk.exe | find "Debug"
```

---

## 五、部署前检查清单

### 5.1 功能测试 ✅

- [ ] 应用启动无崩溃
- [ ] UI 窗口正常显示
- [ ] 一键整理流程完整
- [ ] 托盘功能正常
- [ ] 设置窗口可打开
- [ ] 开机启动可配置
- [ ] 应用可正常关闭

### 5.2 日志检查 ✅

- [ ] logs/ 目录自动创建
- [ ] ccdesk.log 文件生成
- [ ] 应用启动日志记录
- [ ] 用户操作日志记录
- [ ] 异常情况有错误日志
- [ ] 应用关闭日志记录

### 5.3 配置检查 ✅

- [ ] config/ 目录可创建
- [ ] ccdesk_config.json 自动生成
- [ ] 配置默认值合理
- [ ] 配置改变后能保存
- [ ] 配置文件损坏时能恢复

### 5.4 异常处理 ✅

- [ ] 目标目录不存在时自动创建
- [ ] 文件冲突时正确跳过
- [ ] 文件被占用时有错误提示
- [ ] 无规则文件被标记为"无规则"
- [ ] 非 Windows 平台友好降级

### 5.5 性能指标 ✅

- [ ] 启动时间 < 3 秒
- [ ] 整理 100 个文件 < 5 秒
- [ ] 内存占用 < 100 MB
- [ ] CPU 占用平稳，无泄漏

### 5.6 兼容性检查 ✅

- [ ] Windows 10 正常运行
- [ ] Windows 11 正常运行
- [ ] 不同分辨率显示正确
- [ ] 不同 DPI 缩放正常
- [ ] 中文文件名处理正确
- [ ] 特殊字符文件名处理正确

---

## 六、发布包准备

### 6.1 文件清单

发布时应包含:
```
CCDesk-v1.0-release/
├── CCDesk.exe                    # 主程序
├── Qt6Core.dll                   # Qt 运行库
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── d3dcompiler_47.dll           # DirectX (如需要)
├── opengl32.dll                 # OpenGL (如需要)
├── vcruntime140.dll             # VC 运行库
├── README.md                     # 说明文档
├── CHANGELOG.md                  # 更新日志
└── config/                       # 配置目录 (空，首次运行自动生成)
```

### 6.2 生成发布包脚本

```batch
@echo off
REM 发布包生成脚本 (release.bat)

set VERSION=1.0.0
set RELEASE_DIR=CCDesk-v%VERSION%-release
set BUILD_DIR=build\bin

REM 创建发布目录
mkdir %RELEASE_DIR% 2>nul

REM 复制主程序
copy %BUILD_DIR%\CCDesk.exe %RELEASE_DIR%\

REM 复制 Qt 库 (假设 Qt 在 C:\Qt\6.4\msvc2019_64)
set QT_PATH=C:\Qt\6.4\msvc2019_64\bin
copy %QT_PATH%\Qt6Core.dll %RELEASE_DIR%\
copy %QT_PATH%\Qt6Gui.dll %RELEASE_DIR%\
copy %QT_PATH%\Qt6Widgets.dll %RELEASE_DIR%\

REM 复制平台插件
mkdir %RELEASE_DIR%\platforms
copy %QT_PATH%\..\plugins\platforms\qwindows.dll %RELEASE_DIR%\platforms\

REM 复制文档
copy README.md %RELEASE_DIR%\
copy CHANGELOG.md %RELEASE_DIR%\

REM 创建空的 config 目录
mkdir %RELEASE_DIR%\config 2>nul

REM 创建空的 logs 目录
mkdir %RELEASE_DIR%\logs 2>nul

echo.
echo Release package created successfully in: %RELEASE_DIR%
echo.
pause
```

### 6.3 发布包最小化

使用 Qt 部署工具：
```bash
# 使用 windeployqt 自动处理依赖
set QT_PATH=C:\Qt\6.4\msvc2019_64
%QT_PATH%\bin\windeployqt.exe build\bin\CCDesk.exe --release --dir release_output

# 最终发布包在 release_output 目录
```

---

## 七、安装与运行

### 7.1 首次运行前准备

```
1. 从发布包中提取所有文件到目标目录
   例如: C:\Program Files\CCDesk\

2. 确保以下文件存在:
   ✓ CCDesk.exe
   ✓ Qt6Core.dll
   ✓ Qt6Gui.dll
   ✓ Qt6Widgets.dll
   ✓ platforms\qwindows.dll

3. 创建空目录 (首次运行会自动创建，但提前创建更稳妥):
   - config\
   - logs\
```

### 7.2 运行应用

```bash
# 方式 1: 直接运行
CCDesk.exe

# 方式 2: 命令行运行 (便于查看输出)
cd C:\Program Files\CCDesk
CCDesk.exe 2>&1 | tee console.log

# 方式 3: 创建桌面快捷方式
# 右键 CCDesk.exe → 发送到 → 桌面 (创建快捷方式)
```

### 7.3 验证安装

启动后应该看到:
1. 蓝色主窗口显示 "CCDesk - Desktop Organizer"
2. 右下角系统托盘显示蓝色图标
3. logs/ccdesk.log 文件被创建
4. config/ccdesk_config.json 文件被创建

---

## 八、卸载清理

```bash
# 方式 1: 简单删除
# 直接删除 CCDesk 所在目录即可，不修改系统注册表
rmdir /S /Q "C:\Program Files\CCDesk"

# 方式 2: 清理注册表启动项 (如已启用开机启动)
# 打开注册表编辑器:
# 定位到: HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
# 删除 "CCDesk" 项

# 方式 3: 完全清理 (包括配置和日志)
# 删除以下目录:
# - C:\Program Files\CCDesk\
# - C:\Users\[Username]\AppData\Local\CCDesk\ (若存在)
# - 注册表中的 CCDesk 相关项
```

---

## 九、故障排查与支持

### 9.1 启动失败排查

**症状**: 点击 CCDesk.exe 无反应  
**排查步骤**:
```
1. 检查 Qt DLL 是否存在
   dir Qt6*.dll
   
2. 从命令行运行查看错误
   CCDesk.exe
   (若有错误信息会显示)
   
3. 检查依赖
   dumpbin /DEPENDENTS CCDesk.exe
   
4. 查看日志 (如能启动)
   查看 logs/ccdesk.log
```

**解决方案**:
- 确保所有 Qt DLL 都在同一目录
- 重新部署 Qt 库
- 检查 Windows 防火墙设置

### 9.2 整理失败排查

**症状**: 整理时显示 "Move Failed"  
**排查步骤**:
```
1. 查看详细日志
   type logs\ccdesk.log | find "FAILED"
   
2. 检查目标目录权限
   icacls D:\Pictures
   
3. 检查文件是否被占用
   tasklist /fi "IMAGENAME eq explorer.exe"
```

**解决方案**:
- 为用户赋予目标目录的写权限
- 关闭占用文件的程序
- 重试整理操作

### 9.3 日志查看

```bash
# 实时查看日志 (Windows 11+)
Get-Content logs\ccdesk.log -Wait

# 或使用记事本打开
notepad logs\ccdesk.log

# 搜索错误
type logs\ccdesk.log | find "ERROR"

# 统计信息
type logs\ccdesk.log | find /C "INFO"
```

---

## 十、版本信息

**软件名称**: CCDesk - Desktop Organizer  
**版本**: 1.0.0 (MVP Phase 6)  
**编译时间**: 2026-03-18  
**Qt 版本**: 6.2+  
**C++ 标准**: C++17  
**最低系统**: Windows 10 (build 19041)  
**推荐系统**: Windows 11 (build 22000+)  

---

## 十一、技术支持联系方式

### 日志位置
```
logs/ccdesk.log
```

### 配置位置
```
config/ccdesk_config.json
```

### 信息查询命令
```bash
# 查看应用版本
CCDesk.exe --version  (如支持)

# 查看帮助信息
CCDesk.exe --help  (如支持)

# 查看最近日志
type logs\ccdesk.log | tail -20
```

---

## 附录 A: CMake 完整配置示例

```cmake
# CMakeLists.txt (参考)
cmake_minimum_required(VERSION 3.20)
project(CCDesk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 添加编码支持 (Windows)
if(WIN32)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# ... 源文件列表 ...

add_executable(CCDesk ${SOURCES})

target_link_libraries(CCDesk
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(WIN32)
    target_link_libraries(CCDesk advapi32)
endif()

target_include_directories(CCDesk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(CCDesk PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
```

---

## 附录 B: 快速启动脚本

**build.bat** (Windows 编译脚本)
```batch
@echo off
setlocal

echo [*] CCDesk Compilation Script
echo.

REM 检查编译目录
if not exist build (
    echo [+] Creating build directory...
    mkdir build
) else (
    echo [*] build directory already exists
)

cd build

echo [*] Running CMake configuration...
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.4\msvc2019_64 ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo [-] CMake configuration failed!
    exit /b 1
)

echo.
echo [*] Starting build...
cmake --build . --config Release --parallel 4

if errorlevel 1 (
    echo [-] Build failed!
    exit /b 1
)

echo.
echo [+] Build completed successfully!
echo [+] Executable: bin\CCDesk.exe
echo.
pause
```

**run.bat** (快速运行脚本)
```batch
@echo off
if exist build\bin\CCDesk.exe (
    echo [*] Starting CCDesk...
    build\bin\CCDesk.exe
) else (
    echo [-] CCDesk.exe not found. Please build first.
    pause
)
```

---

完成于: 2026-03-18  
版本: MVP v1.0-phase6  
编译时间: ~10-30 秒 (取决于硬件)  
建议测试时间: 1-2 小时
