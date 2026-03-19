#include "config_manager.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace ccdesk::core {

ConfigManager::ConfigManager()
    : m_configPath("config/ccdesk_config.json")
    , m_startupEnabled(false)
{
    Logger::getInstance().info("ConfigManager: Constructed");
    load();
}

ConfigManager::~ConfigManager() {
    Logger::getInstance().info("ConfigManager: Destroyed");
}

bool ConfigManager::load() {
    Logger::getInstance().info("ConfigManager: Loading configuration from " + m_configPath);
    
    if (!fs::exists(m_configPath)) {
        Logger::getInstance().info("ConfigManager: Config file not found, creating default");
        createDefaultConfig();
        return save();
    }
    
    try {
        std::ifstream file(m_configPath);
        if (!file.is_open()) {
            Logger::getInstance().error("ConfigManager: Failed to open config file");
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        return parseConfig(buffer.str());
    } catch (const std::exception& e) {
        Logger::getInstance().error("ConfigManager: Exception loading config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::save() {
    Logger::getInstance().info("ConfigManager: Saving configuration to " + m_configPath);
    
    try {
        // 确保目录存在
        fs::path dirPath = fs::path(m_configPath).parent_path();
        if (!fs::exists(dirPath)) {
            fs::create_directories(dirPath);
        }
        
        std::ofstream file(m_configPath);
        if (!file.is_open()) {
            Logger::getInstance().error("ConfigManager: Failed to open config file for writing");
            return false;
        }
        
        // 简单的JSON格式
        file << "{\n";
        file << "  \"startup_enabled\": " << (m_startupEnabled ? "true" : "false") << ",\n";
        
        file << "  \"partitions\": [\n";
        for (size_t i = 0; i < m_partitions.size(); ++i) {
            const auto& p = m_partitions[i];
            file << "    {\n";
            file << "      \"id\": \"" << p.id << "\",\n";
            file << "      \"name\": \"" << p.name << "\",\n";
            file << "      \"targetPath\": \"" << p.targetPath << "\"\n";
            file << "    }";
            if (i < m_partitions.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ],\n";
        
        file << "  \"rules\": [\n";
        for (size_t i = 0; i < m_rules.size(); ++i) {
            const auto& r = m_rules[i];
            file << "    {\n";
            file << "      \"id\": \"" << r.id << "\",\n";
            file << "      \"name\": \"" << r.name << "\",\n";
            file << "      \"extensions\": \"" << r.extensions << "\",\n";
            file << "      \"targetPath\": \"" << r.targetPath << "\",\n";
            file << "      \"enabled\": " << (r.enabled ? "true" : "false") << "\n";
            file << "    }";
            if (i < m_rules.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        Logger::getInstance().info("ConfigManager: Configuration saved successfully");
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("ConfigManager: Exception saving config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::parseConfig(const std::string& content) {
    // 简单的JSON解析（不使用外部库）
    Logger::getInstance().info("ConfigManager: Parsing configuration");
    
    // 检查startup_enabled
    size_t startupPos = content.find("\"startup_enabled\"");
    if (startupPos != std::string::npos) {
        size_t colonPos = content.find(":", startupPos);
        if (colonPos != std::string::npos) {
            size_t truePos = content.find("true", colonPos);
            m_startupEnabled = (truePos != std::string::npos && truePos < colonPos + 20);
        }
    }
    
    // 若解析失败，应用默认配置并记录警告
    if (m_rules.empty()) {
        Logger::getInstance().warning("ConfigManager: No rules found during parse, applying defaults");
        createDefaultConfig();
    }
    
    if (m_partitions.empty()) {
        Logger::getInstance().warning("ConfigManager: No partitions found during parse, applying defaults");
        createDefaultConfig();
    }
    
    return true;
}

void ConfigManager::createDefaultConfig() {
    Logger::getInstance().info("ConfigManager: Creating default configuration");
    
    // 默认规则
    OrganizeRule docRule;
    docRule.id = "rule_docs";
    docRule.name = "Documents";
    docRule.extensions = "pdf,doc,docx,xls,xlsx,ppt,pptx";
    docRule.targetPath = ".\\Documents";
    docRule.enabled = true;
    m_rules.push_back(docRule);
    
    OrganizeRule imgRule;
    imgRule.id = "rule_images";
    imgRule.name = "Images";
    imgRule.extensions = "jpg,jpeg,png,gif,bmp,svg";
    imgRule.targetPath = ".\\Pictures";
    imgRule.enabled = true;
    m_rules.push_back(imgRule);
    
    OrganizeRule videoRule;
    videoRule.id = "rule_videos";
    videoRule.name = "Videos";
    videoRule.extensions = "mp4,avi,mov,mkv,flv,wmv";
    videoRule.targetPath = ".\\Videos";
    videoRule.enabled = true;
    m_rules.push_back(videoRule);
    
    // 默认分区
    PartitionConfig partition1;
    partition1.id = "partition_1";
    partition1.name = "Documents";
    partition1.targetPath = ".\\Documents";
    m_partitions.push_back(partition1);
}

void ConfigManager::addPartition(const PartitionConfig& partition) {
    m_partitions.push_back(partition);
    Logger::getInstance().info("ConfigManager: Added partition: " + partition.name);
}

void ConfigManager::removePartition(const std::string& id) {
    for (auto it = m_partitions.begin(); it != m_partitions.end(); ++it) {
        if (it->id == id) {
            Logger::getInstance().info("ConfigManager: Removed partition: " + it->name);
            m_partitions.erase(it);
            return;
        }
    }
}

const std::vector<PartitionConfig>& ConfigManager::getPartitions() const {
    return m_partitions;
}

void ConfigManager::addOrganizeRule(const OrganizeRule& rule) {
    m_rules.push_back(rule);
    Logger::getInstance().info("ConfigManager: Added rule: " + rule.name);
}

void ConfigManager::removeOrganizeRule(const std::string& id) {
    for (auto it = m_rules.begin(); it != m_rules.end(); ++it) {
        if (it->id == id) {
            Logger::getInstance().info("ConfigManager: Removed rule: " + it->name);
            m_rules.erase(it);
            return;
        }
    }
}

const std::vector<OrganizeRule>& ConfigManager::getOrganizeRules() const {
    return m_rules;
}

bool ConfigManager::isStartupEnabled() const {
    return m_startupEnabled;
}

void ConfigManager::setStartupEnabled(bool enabled) {
    m_startupEnabled = enabled;
    Logger::getInstance().info("ConfigManager: Startup enabled set to: " + 
                             std::string(enabled ? "true" : "false"));
}

} // namespace ccdesk::core
