#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>

namespace ccdesk::core {

// 分区配置
struct PartitionConfig {
    std::string id;
    std::string name;
    std::string targetPath;
};

// 整理规则配置
struct OrganizeRule {
    std::string id;
    std::string name;
    std::string extensions;   // 逗号分隔的扩展名
    std::string targetPath;
    bool enabled;
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

private:
    std::string m_configPath;
    std::vector<PartitionConfig> m_partitions;
    std::vector<OrganizeRule> m_rules;
    bool m_startupEnabled;
    
    // 内部方法
    void createDefaultConfig();
    bool parseConfig(const std::string& content);
};

} // namespace ccdesk::core

#endif // CONFIG_MANAGER_H
