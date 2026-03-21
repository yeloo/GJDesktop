#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>
#include "desktop_layout_manager.h"

namespace ccdesk::core {

// 分区配置
struct PartitionConfig {
    std::string id;
    std::string name;
    std::string targetPath;
    
    // 新增：分区类型（向后兼容）
    enum PartitionType {
        TYPE_LEGACY_FOLDER,   // 旧版：物理文件夹
        TYPE_DESKTOP_ZONE     // 新版：桌面区域
    };
    
    PartitionType type;
    
    // 新增：桌面区域相关（仅TYPE_DESKTOP_ZONE使用）
    FileCategory category;    // 对应的文件分类
    int x, y, width, height; // 区域位置和大小
    std::string backgroundColor; // 背景颜色（CSS格式）
    
    PartitionConfig() : type(TYPE_LEGACY_FOLDER), category(CATEGORY_OTHER), 
                       x(0), y(0), width(0), height(0) {}
};

// 整理规则配置
struct OrganizeRule {
    std::string id;
    std::string name;
    std::string extensions;   // 逗号分隔的扩展名
    std::string targetPath;
    bool enabled;
    
    // 新增：规则类型（向后兼容）
    enum RuleType {
        TYPE_LEGACY_MOVE,     // 旧版：移动文件
        TYPE_CATEGORY_CLASSIFY // 新版：分类标识
    };
    
    RuleType type;
    
    // 新增：分类相关（仅TYPE_CATEGORY_CLASSIFY使用）
    FileCategory category;    // 对应的文件分类
    
    OrganizeRule() : enabled(true), type(TYPE_LEGACY_MOVE), category(CATEGORY_OTHER) {}
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // 加载配置
    bool load();
    
    // 保存配置
    bool save();
    
    // 分区操作
    void addPartition(const PartitionConfig& partition);
    void removePartition(const std::string& id);
    const std::vector<PartitionConfig>& getPartitions() const;
    
    // 规则操作
    void addOrganizeRule(const OrganizeRule& rule);
    void removeOrganizeRule(const std::string& id);
    const std::vector<OrganizeRule>& getOrganizeRules() const;
    
    // 开机启动设置
    bool isStartupEnabled() const;
    void setStartupEnabled(bool enabled);
    
    // 新增：组织模式设置
    enum OrganizeMode {
        MODE_AUTO,            // 自动选择
        MODE_LEGACY_FOLDER,   // 旧版文件夹模式
        MODE_DESKTOP_ORGANIZE // 新版桌面收纳盒模式
    };
    
    OrganizeMode getOrganizeMode() const;
    void setOrganizeMode(OrganizeMode mode);
    
    // 新增：创建默认桌面收纳盒配置
    void createDefaultDesktopOrganizeConfig();

    // 自动整理设置
    bool isAutoArrangeEnabled() const;
    void setAutoArrangeEnabled(bool enabled);

    bool isAutoArrangeOnStartup() const;
    void setAutoArrangeOnStartup(bool enabled);

private:
    std::string m_configPath;
    std::vector<PartitionConfig> m_partitions;
    std::vector<OrganizeRule> m_rules;
    bool m_startupEnabled;
    OrganizeMode m_organizeMode;

    // 自动整理配置
    bool m_autoArrangeEnabled;
    bool m_autoArrangeOnStartup;
    
    // 内部方法
    void createDefaultConfig();
    bool parseConfig(const std::string& content);
    
    // 转换方法
    static std::string organizeModeToString(OrganizeMode mode);
    static OrganizeMode organizeModeFromString(const std::string& str);
};

} // namespace ccdesk::core

#endif // CONFIG_MANAGER_H