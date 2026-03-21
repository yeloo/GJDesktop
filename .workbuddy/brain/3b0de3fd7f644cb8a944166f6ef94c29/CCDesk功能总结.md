# CCDesk 程序功能总结

## 一、核心定位

**CCDesk (CC Desktop Organizer)** 是一个**桌面文件整理规划器**,主要功能是对桌面文件进行智能分类分析和整理规划,而非实际移动文件。

**产品定位**: 最小可用规划器 (MVP)
- ✅ 扫描桌面文件
- ✅ 智能分类统计
- ✅ 整理规划预览
- ❌ 不实际移动文件
- ❌ 不创建虚拟文件夹
- ❌ 不修改桌面布局

---

## 二、核心功能模块

### 2.1 文件分类器 (DesktopLayoutManager)

**功能**: 智能识别文件类型,进行6大固定分类

**支持的分类**:
1. **快捷方式** (.lnk, .url)
2. **文件夹** (所有目录)
3. **文档** (.doc, .docx, .pdf, .txt, .ppt, .pptx, .xls, .xlsx, .md, .rtf, .odt, .wps)
4. **图片** (.jpg, .jpeg, .png, .gif, .bmp, .svg, .webp, .tiff, .ico, .heic)
5. **视频** (.mp4, .avi, .mov, .wmv, .flv, .mkv, .rmvb, .m4v, .mpg, .webm)
6. **压缩包** (.zip, .rar, .7z, .tar, .gz, .bz2, .xz, .z, .cab, .iso)

**实现方式**: 基于文件扩展名匹配,使用 UTF-8 安全编码转换(已修复 Windows 中文文件名乱码问题)

---

### 2.2 文件整理器 (FileOrganizer)

**功能**: 扫描桌面文件,生成分类预览和整理规划

**核心功能**:

#### 2.2.1 桌面文件扫描
- 扫描桌面路径下的所有文件和文件夹(仅一级,不递归)
- 使用 `std::filesystem::path` 安全处理路径
- 支持中文文件名(Windows UTF-8 编码已修复)
- 返回 `std::vector<fs::path>` 避免编码往返问题

#### 2.2.2 分类预览生成
- 为每个文件匹配分类规则
- 生成 `OrganizePreviewItem` 预览项,包含:
  - 文件路径
  - 文件名
  - 匹配的分类名称
  - 预览状态(可移动/冲突/无匹配规则)

#### 2.2.3 整理规划生成
- 生成 `OrganizePlan` 整理规划,包含:
  - 桌面路径
  - 总文件数(文件+文件夹)
  - 各分类文件数统计
  - 文件明细列表
- **关键**: 只做分类统计,不实际移动文件

#### 2.2.4 整理执行
- 当前版本仅返回分类结果,不移动文件
- 支持取消操作(线程安全)
- 提供 `OrganizeSummary` 结果报告

---

### 2.3 配置管理器 (ConfigManager)

**功能**: 管理应用程序配置和整理规则

**支持的功能**:
- 整理规则的增删查改
- 配置文件持久化存储
- 分区配置加载(仅兼容残留,不代表已实现分区系统)
- 自动扫描桌面路径

**配置存储**: JSON 格式,位于用户 AppData 目录

---

### 2.4 托盘管理器 (TrayManager)

**功能**: 系统托盘图标和右键菜单

**提供的功能**:
- 显示/隐藏主窗口
- 生成整理规划
- 打开设置对话框
- 退出程序
- 双击托盘图标显示主窗口
- 关闭主窗口时隐藏到托盘(不退出程序)

**托盘菜单项**:
1. 显示主窗口
2. 生成整理规划
3. 设置
4. 退出

---

### 2.5 主窗口 (MainWindow)

**功能**: 主界面UI,提供一键整理和设置入口

**UI组件**:
- **一键整理按钮**: 触发桌面文件整理规划
- **设置按钮**: 打开设置对话框
- **状态标签**: 显示当前状态和操作结果
- **预览对话框**: 显示整理预览列表
- **结果报告**: 显示整理结果统计

**工作流程**:
1. 点击"一键整理"按钮
2. 扫描桌面文件
3. 生成整理预览
4. 用户确认预览
5. 显示整理规划(分类统计)
6. 显示结果报告

**关闭行为**: 关闭窗口时隐藏到系统托盘,而非退出程序

---

### 2.6 设置对话框 (SettingsDialog)

**功能**: 应用程序设置

**支持的设置**:
- **开机启动**: 通过 Windows 注册表管理开机自启动
- **规则管理**: 查看当前整理规则(仅显示,不支持编辑)
- **设置保存**: 应用配置修改

**注意**: 分区管理标签页已移除,当前版本不支持虚拟分区

---

### 2.7 桌面图标访问器 (DesktopIconAccessor) - v1

**功能**: 读取桌面图标的显示名称和坐标位置

**实现路线**:
1. **主要路线**: IFolderView COM 接口
   - 获取 IShellWindows
   - 查找桌面窗口
   - 获取 IShellFolder
   - 枚举桌面图标
   - 读取 displayName 和 position

2. **备用路线**: SysListView32 跨进程消息
   - 查找 Progman/WorkerW 窗口
   - 获取 SysListView32 句柄
   - 跨进程读取图标坐标
   - **注意**: v1 中此路线无法获取 displayName,符合设计契约

**v1 数据契约**:
```cpp
struct DesktopIcon {
    std::string displayName;  // 图标显示名称(UTF-8)
    POINT position;           // 图标坐标 {x, y}
};
```

**v1 限制**:
- 仅读-only,不支持写回坐标
- 不获取文件路径
- 不获取唯一标识符
- 不获取 z-index 层级

**v2 规划**:
- 支持写回图标位置
- 获取文件完整路径
- 支持唯一标识符
- 支持布局记忆和恢复

**状态**: 已完成实现,等待编译验证(当前环境缺少 CMake 和 Qt SDK)

---

## 三、完整工作流程

### 3.1 用户启动程序
1. 程序启动,初始化所有核心模块
2. 自动扫描桌面路径
3. 加载用户配置和整理规则
4. 初始化系统托盘图标
5. 显示主窗口

### 3.2 一键整理流程
1. 用户点击"一键整理"按钮
2. FileOrganizer 扫描桌面所有文件
3. DesktopLayoutManager 分类器识别每个文件类型
4. 生成整理预览(显示在对话框中)
5. 用户确认预览
6. 生成整理规划(分类统计)
7. 显示结果报告

### 3.3 托盘操作流程
1. 用户最小化或关闭主窗口
2. 程序隐藏到系统托盘
3. 用户右键托盘图标
4. 选择"生成整理规划"
5. 主窗口显示并执行整理流程

### 3.4 设置管理流程
1. 用户点击"设置"按钮
2. 打开设置对话框
3. 修改开机启动选项
4. 查看当前整理规则
5. 保存设置
6. 配置文件更新

---

## 四、技术特性

### 4.1 跨平台支持
- Qt 6.x 框架(当前目标 Windows)
- 标准 C++17
- 使用 Qt 文件系统 API 确保跨平台兼容性

### 4.2 编码处理
- **Windows 中文文件名乱码已修复**
- 统一使用 UTF-8 编码
- 业务逻辑层使用 `std::filesystem::path`
- UI 显示层使用 `QString::fromLocal8Bit()` 正确处理 Windows GBK 编码
- 提供 `utf8StringToPath()` 安全转换函数

### 4.3 线程安全
- 使用 `std::atomic<bool>` 实现取消标志
- 支持长时间操作的取消
- 线程安全的配置读写

### 4.4 错误处理
- 完整的错误消息机制
- 所有操作都有状态反馈
- 关键操作失败时提供详细错误信息

---

## 五、已修复的问题

### 5.1 中文文件名乱码 (2026-03-20)
- **问题**: Windows 上 std::filesystem::path 使用 UTF-16,但 .string() 转为 GBK,导致乱码
- **修复**: 统一使用 `std::wstring` 存储路径,UI 显示时使用 `QString::fromStdWString()` 正确转换
- **影响文件**: file_organizer.h/cpp, main_window.cpp

### 5.2 UTF-8 路径构造问题 (2026-03-20)
- **问题**: UTF-8 std::string 直接构造 fs::path 存在编码风险
- **修复**: 新增 `utf8StringToPath()` 安全转换函数,统一使用安全转换
- **影响文件**: file_organizer.cpp, desktop_layout_manager.cpp/h

### 5.3 DesktopLayoutManager const 限定不匹配 (2026-03-20)
- **问题**: `isExtensionInList` 函数的 extensions 参数类型不匹配
- **修复**: 将参数从 `const char* []` 改为 `const char* const []`
- **影响文件**: desktop_layout_manager.cpp

### 5.4 COM 初始化语义问题 (2026-03-21)
- **问题**: CoInitializeEx 返回 S_FALSE 时,needUninitialize 状态错误
- **修复**: S_FALSE 也要调用 CoUninitialize 配对,RPC_E_CHANGED_MODE 直接标记为失败
- **影响文件**: desktop_icon_accessor.cpp

---

## 六、当前版本状态

### 6.1 已完成功能
- ✅ 桌面文件扫描
- ✅ 智能文件分类(6大类)
- ✅ 整理规划生成
- ✅ 预览对话框
- ✅ 结果报告显示
- ✅ 系统托盘集成
- ✅ 配置管理
- ✅ 开机启动管理
- ✅ DesktopIconAccessor v1 实现(等待编译验证)
- ✅ Windows 中文文件名支持

### 6.2 已移除功能
- ❌ 虚拟分区窗口(已删除 partition_widget.h/cpp)
- ❌ 分区管理标签页(已从设置对话框移除)
- ❌ 文件实际移动(当前版本仅做分类统计)

### 6.3 待验证功能
- ⏳ DesktopIconAccessor 运行时验证(等待编译环境)

### 6.4 未来规划(v2)
- 📝 写回桌面图标位置
- 📝 获取文件完整路径
- 📝 支持布局记忆和恢复
- 📝 虚拟分区系统
- 📝 文件实际移动功能

---

## 七、项目结构

```
CCDesk/
├── src/
│   ├── main.cpp                    # 程序入口
│   ├── core/                       # 核心模块
│   │   ├── app_manager.h/cpp       # 应用管理器
│   │   ├── config_manager.h/cpp    # 配置管理器
│   │   ├── file_organizer.h/cpp    # 文件整理器
│   │   ├── desktop_layout_manager.h/cpp  # 桌面布局管理器
│   │   ├── tray_manager.h/cpp      # 托盘管理器
│   │   ├── desktop_icon_accessor.h/cpp  # 桌面图标访问器 v1
│   │   ├── logger.h/cpp            # 日志系统
│   │   └── organize_result.h       # 整理结果数据结构
│   └── ui/                         # UI模块
│       ├── main_window.h/cpp       # 主窗口
│       └── settings_dialog.h/cpp   # 设置对话框
├── CMakeLists.txt                  # CMake 构建配置
├── build.bat                       # Windows 批处理编译脚本
└── build.ps1                       # PowerShell 编译脚本
```

---

## 八、依赖环境

### 8.1 编译依赖
- CMake 3.20+
- Visual Studio 2019/2022 (MSVC 编译器)
- Qt 6.2+ (6.x)

### 8.2 运行依赖
- Qt 6 Core/Gui/Widgets 库
- Windows Shell COM 接口(桌面图标访问器)
- Windows 注册表 API(开机启动)

### 8.3 Windows 库链接
- shell32 (Shell COM)
- ole32 (COM 基础)
- oleaut32 (COM 自动化)
- shlwapi (Shell 轻量级工具)
- user32 (用户界面)
- comctl32 (通用控件)
- advapi32 (注册表操作)

---

## 九、总结

**CCDesk** 是一个**桌面文件整理规划器**,主要功能包括:

1. **智能分类**: 自动识别桌面文件类型,分为6大类(快捷方式、文件夹、文档、图片、视频、压缩包)
2. **整理规划**: 扫描桌面文件,生成分类预览和统计规划
3. **系统托盘**: 集成系统托盘,支持后台运行和快速操作
4. **配置管理**: 支持整理规则配置和开机启动管理
5. **桌面图标访问**: v1 实现读取桌面图标名称和坐标(等待编译验证)

**产品定位**: 最小可用规划器(MVP),不实际移动文件,不做虚拟分区,专注于桌面文件的智能分类分析和规划展示。

**当前状态**: 核心功能已完成实现,等待编译环境验证。
