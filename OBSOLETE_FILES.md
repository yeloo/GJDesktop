# 废弃文件说明

**生成时间**：2026年3月20日
**说明**：以下文件已废弃，不再属于最小规划器项目

---

## 一、已废弃的UI组件

### 1.1 partition_widget.h / partition_widget.cpp

**状态**：已废弃并移除引用

**原因**：
- 接口不一致：.h文件使用`DesktopPartition`类型，但.cpp文件使用完全不同的构造函数签名
- 依赖缺失：`DesktopPartition`类型在当前项目中不存在
- 不属于最小规划器范围：虚拟分区功能未实现，也不属于规划器核心功能

**清理工作**：
- ✅ 从`src/ui/main_window.h`中删除了`PartitionWidget`前向声明和成员变量
- ✅ 从`src/ui/main_window.cpp`中删除了`#include "partition_widget.h"`
- ✅ 从`CMakeLists.txt`中删除了partition_widget相关编译条目
- ⚠️ 文件物理删除失败（文件在workspace外），但已不再被项目引用

**影响**：
- 主界面不再包含分区管理功能（该功能未实现，仅作展示）
- 减少了编译错误和接口不一致问题

---

### 1.2 settings_dialog.h / settings_dialog.cpp

**状态**：文件在workspace外，无法修改

**当前问题**：
- 文件存在于项目目录但不在当前workspace中
- 可能包含旧版"分区系统"相关UI代码
- 可能误导用户以为有分区编辑功能

**建议**：
- 如需保留设置功能，应重新实现最小版本的设置对话框
- 新版本只应包含：
  - 桌面路径设置
  - 分类规则配置（仅扩展名，不包含分区位置）
  - 关于信息
- 不应包含：
  - 虚拟分区编辑
  - 拖拽分区设置
  - 任何暗示已实现桌面分区的文案

---

## 二、已废弃的核心组件

### 2.1 app_manager.h / app_manager.cpp

**状态**：文件在workspace外，无法修改

**当前问题**：
- 文件存在于项目目录但不在当前workspace中
- `AppManager::restorePartitions()`方法会尝试恢复分区窗口
- 但分区功能已废弃，该方法会产生未定义行为

**建议**：
- 如需保留AppManager，应删除或禁用`restorePartitions()`方法
- 或者将AppManager整体移除，直接在main.cpp中初始化所需组件

---

## 三、ConfigManager中的分区配置

### 3.1 当前状态

**实际使用的字段**：
- ✅ `id` - 分区标识
- ✅ `name` - 分区名称（仅用于分类展示）
- ✅ `type` - 分区类型（区分桌面分区/传统文件夹）
- ✅ `category` - 关联的文件分类枚举

**未使用的字段（兼容保留）**：
- ❌ `x/y/width/height` - 虚拟分区位置大小（规划中，未实现）
- ❌ `backgroundColor` - 分区背景色（规划中，未实现）
- ❌ `targetPath` - 对于桌面分区类型为空（不移动文件）

### 3.2 使用场景

**仅用于**：
- 存储分类规则配置（通过partition结构保存分类信息）
- 生成规划时提供分类名称和颜色

**不用于**：
- 创建虚拟分区窗口（功能已废弃）
- 文件拖拽和放置（功能未实现）
- 实时桌面布局管理（功能未实现）

### 3.3 建议

**短期（当前版本）**：
- 保留partition配置结构作为兼容层
- 明确在代码注释中说明：partition配置仅用于分类统计，不代表已实现虚拟分区

**长期（后续版本）**：
- 如需实现真实分区功能，应重新设计数据结构
- 区分：分类规则配置 vs 虚拟分区布局配置

---

## 四、当前项目结构（最小规划器）

### 4.1 核心文件（已实现）

```
src/core/
├── logger.h/cpp              # 日志系统
├── config_manager.h/cpp      # 配置管理（含分类规则）
├── file_organizer.h/cpp      # 文件整理器（仅规划模式）
├── organize_result.h         # 整理结果结构
├── desktop_layout_manager.h/cpp  # 桌面布局管理器（仅分类）
└── tray_manager.h/cpp        # 托盘管理
```

### 4.2 UI文件（已实现）

```
src/ui/
├── main_window.h/cpp         # 主窗口（规划模式UI）
└── （settings_dialog已废弃）
```

### 4.3 已移除文件

```
src/ui/
├── partition_widget.h        # 已废弃（接口不一致）
└── partition_widget.cpp      # 已废弃（接口不一致）
```

### 4.4 需要重新实现的文件

```
src/ui/
├── settings_dialog.h         # 需重新实现（原文件在workspace外）
└── settings_dialog.cpp       # 需重新实现（原文件在workspace外）

src/core/
├── app_manager.h            # 需清理或移除（原文件在workspace外）
└── app_manager.cpp          # 需清理或移除（原文件在workspace外）
```

---

## 五、废弃文件处理建议

### 5.1 物理删除（推荐）

如果文件在workspace内：
```bash
git rm src/ui/partition_widget.h
rm src/ui/partition_widget.cpp
```

### 5.2 保留但标记（当前方案）

如果文件在workspace外（无法直接删除）：

1. 在CMakeLists.txt中移除编译引用（已做）
2. 在相关头文件中移除前向声明（已做）
3. 在实现文件中移除include（已做）
4. 创建本文档说明废弃状态（已做）

---

## 六、清理后的项目状态

### 6.1 编译状态

**修改后**：
- ✅ CMakeLists.txt不再引用废弃文件
- ✅ main_window不再依赖partition_widget
- ✅ 接口不一致问题已解决
- ✅ targetDirectory编译错误已修复

**预期结果**：
- ✅ 项目可正常编译
- ✅ 无未定义引用错误
- ✅ 无类型不匹配错误

### 6.2 功能状态

**当前版本（最小规划器）**：
- ✅ 扫描桌面文件（一级目录）
- ✅ 按固定分类规则分析文件类型
- ✅ 生成分类规划报告
- ✅ 显示分类统计和文件明细
- ✅ 托盘菜单支持快速分析
- ❌ 虚拟分区管理（已废弃）
- ❌ 文件真实移动（当前版本不支持）
- ❌ 拖拽分区调整（未实现）

---

## 七、迁移指南

### 7.1 对于开发者

**如果要从旧版本迁移**：

1. 删除所有`partition_widget`相关代码
2. 删除所有`DesktopPartition`类型引用
3. 将`setDesktopLayoutManager`调用改为直接传入FileOrganizer
4. 更新所有"移动到"文案为"建议归类到"
5. 更新所有"执行整理"文案为"生成规划"

### 7.2 对于用户

**用户可见变化**：

- 分区管理按钮已禁用（标注"未实现"）
- 所有文案明确为"规划模式"
- 明确提示"不会移动文件"
- 规划报告更简洁，只包含分类建议

---

## 八、版本历史

### 8.1 废弃版本

**版本**：桌面收纳盒整理器（旧版）
**特征**：
- 包含虚拟分区管理UI
- 包含拖拽分区功能
- 接口不一致，编译错误

### 8.2 当前版本

**版本**：桌面收纳盒规划器（最小规划器）
**特征**：
- 移除虚拟分区功能
- 专注于文件分类规划
- 接口一致，可编译运行
- 无误导性文案

---

**文档维护**：每次废弃文件时更新本文档
