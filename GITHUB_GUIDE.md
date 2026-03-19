# CCDesk - GitHub 上传完整操作手册

## 目录
1. [仓库目录结构说明](#1-仓库目录结构)
2. [本地 Git 初始化与首次推送](#2-本地-git-初始化)
3. [日常提交与推送](#3-日常提交)
4. [打 Tag 触发 Release 发布](#4-打-tag-触发-release)
5. [GitHub 页面查看编译结果](#5-查看编译结果)
6. [常见问题](#6-常见问题)

---

## 1. 仓库目录结构

上传到 GitHub 后，仓库结构如下：

```
CCDesk/                             ← 仓库根目录
│
├── .github/
│   └── workflows/
│       └── build.yml               ← ✅ Actions 自动编译工作流
│
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── logger.h / logger.cpp
│   │   ├── config_manager.h / config_manager.cpp
│   │   ├── file_organizer.h / file_organizer.cpp
│   │   ├── organize_result.h
│   │   ├── tray_manager.h / tray_manager.cpp
│   │   └── app_manager.h / app_manager.cpp
│   └── ui/
│       ├── main_window.h / main_window.cpp
│       ├── partition_widget.h / partition_widget.cpp
│       └── settings_dialog.h / settings_dialog.cpp
│
├── assets/                         ← 图标等资源
├── config/                         ← 配置模板目录（ccdesk_config.json 被 .gitignore 忽略）
├── CMakeLists.txt                  ← ✅ CMake 编译配置
├── .gitignore                      ← ✅ 忽略规则
├── requirements.txt
└── README.md                       ← 建议添加（可选）
```

> ⚠️ **不会被上传的文件**（已在 .gitignore 中忽略）：
> - `build/` 目录（编译产物）
> - `logs/` 目录（运行日志）
> - `config/ccdesk_config.json`（含用户路径）
> - `.vs/`、`*.exe`、`*.dll` 等

---

## 2. 本地 Git 初始化

### 第 1 步：在 GitHub 上创建仓库

1. 打开 https://github.com/new
2. 填写仓库名称，例如 `CCDesk`
3. 选择 **Private** 或 **Public**
4. **不要**勾选 "Initialize this repository with a README"
5. 点击 **Create repository**
6. 复制仓库地址，例如：
   ```
   https://github.com/你的用户名/CCDesk.git
   ```

---

### 第 2 步：本地初始化并推送

在 PowerShell 中执行以下命令（**逐步执行，勿一次粘贴全部**）：

```powershell
# ── 进入项目目录 ──────────────────────────────────────
cd e:\CCdesk

# ── 初始化 Git 仓库 ───────────────────────────────────
git init

# ── 设置默认分支为 main ───────────────────────────────
git branch -M main

# ── 配置用户信息（如果尚未配置）─────────────────────────
git config user.name  "你的GitHub用户名"
git config user.email "你的邮箱@example.com"

# ── 添加远程仓库（替换为你的实际地址）────────────────────
git remote add origin https://github.com/你的用户名/CCDesk.git

# ── 验证 .gitignore 是否正确（检查哪些文件会被追踪）──────
git status

# ── 将所有文件加入暂存区 ──────────────────────────────
git add .

# ── 再次确认暂存区内容（应看不到 build/、logs/ 等）────────
git status

# ── 提交初始版本 ──────────────────────────────────────
git commit -m "feat: initial commit - CCDesk MVP v1.0 Phase6"

# ── 推送到 GitHub ─────────────────────────────────────
git push -u origin main
```

推送成功后，终端会显示：
```
Enumerating objects: XX, done.
...
To https://github.com/你的用户名/CCDesk.git
 * [new branch]      main -> main
Branch 'main' set up to track remote branch 'main' from 'origin'.
```

---

### 第 3 步：验证 Actions 已触发

推送后，GitHub Actions 会**自动触发**编译。验证方式：
1. 打开 `https://github.com/你的用户名/CCDesk/actions`
2. 看到 **"Build CCDesk (Windows Release)"** 工作流正在运行（黄色旋转图标）

---

## 3. 日常提交

每次修改代码后，执行：

```powershell
cd e:\CCdesk

# 查看修改了哪些文件
git status

# 添加所有改动（或用 git add 指定文件）
git add .

# 提交（使用语义化提交信息）
git commit -m "fix: 修复配置文件回退问题"

# 推送，自动触发 Actions 重新编译
git push
```

### 推荐的提交信息格式

| 类型 | 含义 | 示例 |
|------|------|------|
| `feat:` | 新功能 | `feat: 添加日志轮转功能` |
| `fix:` | 修复 Bug | `fix: 修复托盘图标不显示` |
| `refactor:` | 重构 | `refactor: 整理文件组织逻辑` |
| `docs:` | 文档 | `docs: 更新 README` |
| `chore:` | 杂项 | `chore: 更新 .gitignore` |
| `ci:` | CI 配置 | `ci: 升级 Qt 版本到 6.8` |

---

## 4. 打 Tag 触发 Release 发布

打 Tag 后，Actions 会编译 + 自动创建 GitHub Release 并附上安装包：

```powershell
cd e:\CCdesk

# ── 确保代码已推送 ────────────────────────────────────
git push

# ── 创建版本 Tag（语义化版本号）──────────────────────────
git tag v1.0.0

# ── 推送 Tag 到 GitHub（触发 Release 创建）───────────────
git push origin v1.0.0
```

几分钟后，在 GitHub 仓库的 **Releases** 页面会看到：
- `CCDesk v1.0.0`
- 附件：`CCDesk-Release-windows-x64.zip`（可直接下载运行）

### 常用 Tag 命令

```powershell
# 查看所有 Tag
git tag

# 创建带说明的 Tag（推荐）
git tag -a v1.0.0 -m "Release: MVP Phase6 初始发布版本"

# 删除本地 Tag（如打错了）
git tag -d v1.0.0

# 删除远程 Tag（如推错了）
git push origin --delete v1.0.0
```

---

## 5. 查看编译结果

### 5.1 查看 Actions 运行状态

1. 打开仓库页面：`https://github.com/你的用户名/CCDesk`
2. 点击顶部 **Actions** 标签页
3. 左侧看到 **"Build CCDesk (Windows Release)"**
4. 右侧列表显示每次触发的运行记录：
   - 🟡 黄色旋转 = 正在运行
   - ✅ 绿色勾选 = 编译成功
   - ❌ 红色叉号 = 编译失败

### 5.2 下载编译产物 (Artifact)

1. 点击某次成功的运行记录
2. 滚动到页面底部 **"Artifacts"** 区域
3. 点击 **`CCDesk-Release-windows-x64`** 下载 ZIP 文件
4. 解压后直接运行 `CCDesk.exe`

> ⚠️ Artifact 默认保留 **7 天**，超期后需重新触发编译。

### 5.3 查看编译日志（排查失败原因）

1. 点击失败的运行记录（❌）
2. 点击 **"build"** 任务
3. 展开各步骤查看详细日志：
   - `Configure CMake` → 查看 Qt 路径是否正确
   - `Build (Release)` → 查看编译错误信息
   - `Deploy Qt dependencies` → 查看 windeployqt 是否成功

### 5.4 查看 Release 下载页面

打 Tag 后访问：
```
https://github.com/你的用户名/CCDesk/releases
```
每个 Release 都有：
- 版本说明（自动生成）
- 附件：`CCDesk-Release-windows-x64.zip`

---

## 6. 常见问题

### Q1: push 时要求输入用户名密码

**原因**: GitHub 已不支持密码验证，需使用 Personal Access Token (PAT)  
**解决**:
```
1. 打开 GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic)
2. 点击 "Generate new token (classic)"
3. 勾选 "repo" 权限
4. 生成 token，复制保存
5. push 时用 token 代替密码输入
```

或使用 Git Credential Manager（推荐）：
```powershell
git config --global credential.helper manager
```

---

### Q2: Actions 提示 "Qt package not found"

**原因**: Qt 版本号或 arch 字符串填写有误  
**解决**: 修改 `build.yml` 中的：
```yaml
QT_VERSION: '6.8.1'          # 检查 https://download.qt.io/online/qtsdkrepository/ 确认可用版本
QT_ARCH: win64_msvc2022_64   # 确认与 Qt_VERSION 匹配
```

---

### Q3: Actions 提示 CMake 找不到 Qt6

**原因**: `CMAKE_PREFIX_PATH` 未正确传入  
**解决**: 检查 `build.yml` 中 Configure CMake 步骤：
```yaml
-DCMAKE_PREFIX_PATH="${{ env.Qt6_DIR }}"
```
`Qt6_DIR` 由 `jurplel/install-qt-action` 自动设置，不需要手动填写路径。

---

### Q4: 编译成功但运行时报 "找不到 Qt6Core.dll"

**原因**: windeployqt 没有正确收集依赖  
**解决**: 检查 `build.yml` 中 Deploy 步骤的路径是否正确，或手动指定 Qt bin 目录。

---

### Q5: 想在 PR 时也能预览编译产物

已支持！`build.yml` 中配置了：
```yaml
on:
  pull_request:
    branches: [ main, master ]
```
提交 PR 时会触发编译，在 PR 页面下方的 Checks 中可下载 Artifact。

---

## 附录：工作流完整触发逻辑

```
代码推送到 main/master
    ↓
GitHub Actions 触发
    ↓
┌─────────────────────────────────┐
│ 1. 拉取代码 (checkout)          │
│ 2. 安装 Qt 6.8 (含缓存)        │
│ 3. 配置 MSVC 2022 环境         │
│ 4. cmake 配置 (Release)         │
│ 5. cmake --build (编译)         │
│ 6. 验证 CCDesk.exe              │
│ 7. windeployqt (收集 Qt DLL)   │
│ 8. 写入 BUILD_INFO.txt          │
│ 9. 打包 ZIP                     │
│ 10. 上传 Artifact (7天)         │
│ 11. 如是 Tag → 创建 Release     │
└─────────────────────────────────┘
    ↓
编译完成！Artifact 可下载
```

---

*最后更新: 2026-03-19 | 版本: MVP v1.0-phase6*
