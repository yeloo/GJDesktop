#include "config_manager.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <QStandardPaths>
#include <QString>

namespace fs = std::filesystem;

namespace ccdesk::core {

ConfigManager::ConfigManager()
    : m_startupEnabled(false)
    , m_organizeMode(MODE_DESKTOP_ORGANIZE)  // 默认使用桌面收纳盒模式
    , m_autoArrangeEnabled(false)              // 默认禁用自动整理
    , m_autoArrangeOnStartup(false)            // 默认禁用启动时整理
{
    // 辅助函数：将 QString 安全转换为 UTF-8 std::string
    // Windows: QString (UTF-16) -> toUtf8() -> std::string (UTF-8)
    // Linux/macOS: QString (UTF-8) -> toUtf8() -> std::string (UTF-8)
    auto qstringToUtf8String = [](const QString& qstr) -> std::string {
        return qstr.toUtf8().toStdString();
    };

    // 使用 Qt 标准路径获取用户数据目录，确保配置文件位置可预测
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    
    // 安全转换：使用 toUtf8() 而不是 toStdString()
    fs::path configDir = fs::path(appDataDir.toStdWString());  // Windows 下使用 UTF-16
    if (!fs::exists(configDir)) {
        fs::create_directories(configDir);
    }
    
    // 安全转换：从 fs::path 转为 UTF-8 std::string
#ifdef _WIN32
    m_configPath = QString::fromStdWString((configDir / "ccdesk_config.json").wstring()).toUtf8().toStdString();
#else
    m_configPath = (configDir / "ccdesk_config.json").string();
#endif

    Logger::getInstance().info("ConfigManager: Constructed, config path: " + m_configPath);
    load();
}

ConfigManager::~ConfigManager() {
    Logger::getInstance().info("ConfigManager: Destroyed");
}

std::string ConfigManager::organizeModeToString(OrganizeMode mode) {
    switch (mode) {
        case MODE_LEGACY_FOLDER: return "legacy_folder";
        case MODE_DESKTOP_ORGANIZE: return "desktop_organize";
        case MODE_AUTO:
        default: return "auto";
    }
}

ConfigManager::OrganizeMode ConfigManager::organizeModeFromString(const std::string& str) {
    if (str == "legacy_folder") return MODE_LEGACY_FOLDER;
    if (str == "desktop_organize") return MODE_DESKTOP_ORGANIZE;
    return MODE_AUTO;
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
        file << "  \"version\": \"2.0\",\n";
        file << "  \"startup_enabled\": " << (m_startupEnabled ? "true" : "false") << ",\n";
        file << "  \"organize_mode\": \"" << organizeModeToString(m_organizeMode) << "\",\n";
        file << "  \"auto_arrange_enabled\": " << (m_autoArrangeEnabled ? "true" : "false") << ",\n";
        file << "  \"auto_arrange_on_startup\": " << (m_autoArrangeOnStartup ? "true" : "false") << ",\n";
        
        file << "  \"partitions\": [\n";
        for (size_t i = 0; i < m_partitions.size(); ++i) {
            const auto& p = m_partitions[i];
            file << "    {\n";
            file << "      \"id\": \"" << p.id << "\",\n";
            file << "      \"name\": \"" << p.name << "\",\n";
            file << "      \"targetPath\": \"" << p.targetPath << "\",\n";
            file << "      \"type\": \"" << (p.type == PartitionConfig::TYPE_DESKTOP_ZONE ? "desktop_zone" : "legacy_folder") << "\",\n";
            
            if (p.type == PartitionConfig::TYPE_DESKTOP_ZONE) {
                file << "      \"category\": " << static_cast<int>(p.category) << ",\n";
                file << "      \"x\": " << p.x << ",\n";
                file << "      \"y\": " << p.y << ",\n";
                file << "      \"width\": " << p.width << ",\n";
                file << "      \"height\": " << p.height << ",\n";
                file << "      \"backgroundColor\": \"" << p.backgroundColor << "\"\n";
            } else {
                file << "      \"category\": 0,\n";
                file << "      \"x\": 0,\n";
                file << "      \"y\": 0,\n";
                file << "      \"width\": 0,\n";
                file << "      \"height\": 0,\n";
                file << "      \"backgroundColor\": \"\"\n";
            }
            
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
            file << "      \"enabled\": " << (r.enabled ? "true" : "false") << ",\n";
            file << "      \"type\": \"" << (r.type == OrganizeRule::TYPE_CATEGORY_CLASSIFY ? "category_classify" : "legacy_move") << "\",\n";
            file << "      \"category\": " << static_cast<int>(r.category) << "\n";
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
    
    // 清空现有数据
    m_partitions.clear();
    m_rules.clear();
    
    // 检查版本号
    size_t versionPos = content.find("\"version\"");
    std::string version = "1.0";
    if (versionPos != std::string::npos) {
        size_t colonPos = content.find(":", versionPos);
        size_t quoteStart = content.find("\"", colonPos);
        size_t quoteEnd = content.find("\"", quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            version = content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }
    Logger::getInstance().info("ConfigManager: Config version: " + version);
    
    // 检查startup_enabled
    size_t startupPos = content.find("\"startup_enabled\"");
    if (startupPos != std::string::npos) {
        size_t colonPos = content.find(":", startupPos);
        if (colonPos != std::string::npos) {
            size_t truePos = content.find("true", colonPos);
            m_startupEnabled = (truePos != std::string::npos && truePos < colonPos + 20);
        }
    }
    
    // 检查organize_mode（新版配置）
    size_t modePos = content.find("\"organize_mode\"");
    if (modePos != std::string::npos) {
        size_t colonPos = content.find(":", modePos);
        size_t quoteStart = content.find("\"", colonPos);
        size_t quoteEnd = content.find("\"", quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            std::string modeStr = content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            m_organizeMode = organizeModeFromString(modeStr);
        }
    }

    // 检查auto_arrange_enabled
    size_t autoArrangePos = content.find("\"auto_arrange_enabled\"");
    if (autoArrangePos != std::string::npos) {
        size_t colonPos = content.find(":", autoArrangePos);
        if (colonPos != std::string::npos) {
            size_t truePos = content.find("true", colonPos);
            m_autoArrangeEnabled = (truePos != std::string::npos && truePos < colonPos + 20);
        }
    }

    // 检查auto_arrange_on_startup
    size_t autoArrangeStartupPos = content.find("\"auto_arrange_on_startup\"");
    if (autoArrangeStartupPos != std::string::npos) {
        size_t colonPos = content.find(":", autoArrangeStartupPos);
        if (colonPos != std::string::npos) {
            size_t truePos = content.find("true", colonPos);
            m_autoArrangeOnStartup = (truePos != std::string::npos && truePos < colonPos + 25);
        }
    }
    
    // 解析分区数组
    size_t partitionsStart = content.find("\"partitions\"");
    if (partitionsStart != std::string::npos) {
        size_t arrayStart = content.find("[", partitionsStart);
        size_t arrayEnd = content.find("]", arrayStart);
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string partitionsStr = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
            
            // 简单的对象提取：逐个 { } 对
            size_t objStart = 0;
            while ((objStart = partitionsStr.find("{", objStart)) != std::string::npos) {
                size_t objEnd = partitionsStr.find("}", objStart);
                if (objEnd == std::string::npos) break;
                
                std::string objStr = partitionsStr.substr(objStart, objEnd - objStart + 1);
                
                // 提取分区配置
                PartitionConfig pc;
                
                // 提取 id
                size_t idPos = objStr.find("\"id\"");
                if (idPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", idPos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        pc.id = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                // 提取 name
                size_t namePos = objStr.find("\"name\"");
                if (namePos != std::string::npos) {
                    size_t colonPos = objStr.find(":", namePos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        pc.name = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                // 提取 targetPath
                size_t pathPos = objStr.find("\"targetPath\"");
                if (pathPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", pathPos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        pc.targetPath = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                // 提取 type（新版）
                size_t typePos = objStr.find("\"type\"");
                if (typePos != std::string::npos) {
                    size_t colonPos = objStr.find(":", typePos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        std::string typeStr = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                        pc.type = (typeStr == "desktop_zone") ? PartitionConfig::TYPE_DESKTOP_ZONE 
                                                              : PartitionConfig::TYPE_LEGACY_FOLDER;
                    }
                }
                
                // 提取 category（新版）
                size_t categoryPos = objStr.find("\"category\"");
                if (categoryPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", categoryPos);
                    size_t valueStart = objStr.find_first_of("0123456789", colonPos);
                    size_t valueEnd = objStr.find_first_not_of("0123456789", valueStart);
                    if (valueStart != std::string::npos) {
                        std::string catStr = objStr.substr(valueStart, valueEnd - valueStart);
                        pc.category = static_cast<FileCategory>(std::stoi(catStr));
                    }
                }
                
                // 提取位置和大小（新版）
                size_t xPos = objStr.find("\"x\"");
                if (xPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", xPos);
                    size_t valueStart = objStr.find_first_of("0123456789-", colonPos);
                    size_t valueEnd = objStr.find_first_not_of("0123456789-", valueStart);
                    if (valueStart != std::string::npos) {
                        pc.x = std::stoi(objStr.substr(valueStart, valueEnd - valueStart));
                    }
                }
                
                size_t yPos = objStr.find("\"y\"");
                if (yPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", yPos);
                    size_t valueStart = objStr.find_first_of("0123456789-", colonPos);
                    size_t valueEnd = objStr.find_first_not_of("0123456789-", valueStart);
                    if (valueStart != std::string::npos) {
                        pc.y = std::stoi(objStr.substr(valueStart, valueEnd - valueStart));
                    }
                }
                
                size_t widthPos = objStr.find("\"width\"");
                if (widthPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", widthPos);
                    size_t valueStart = objStr.find_first_of("0123456789", colonPos);
                    size_t valueEnd = objStr.find_first_not_of("0123456789", valueStart);
                    if (valueStart != std::string::npos) {
                        pc.width = std::stoi(objStr.substr(valueStart, valueEnd - valueStart));
                    }
                }
                
                size_t heightPos = objStr.find("\"height\"");
                if (heightPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", heightPos);
                    size_t valueStart = objStr.find_first_of("0123456789", colonPos);
                    size_t valueEnd = objStr.find_first_not_of("0123456789", valueStart);
                    if (valueStart != std::string::npos) {
                        pc.height = std::stoi(objStr.substr(valueStart, valueEnd - valueStart));
                    }
                }
                
                // 提取背景颜色（新版）
                size_t colorPos = objStr.find("\"backgroundColor\"");
                if (colorPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", colorPos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        pc.backgroundColor = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                m_partitions.push_back(pc);
                objStart = objEnd + 1;
            }
        }
    }
    
    // 解析规则数组
    size_t rulesStart = content.find("\"rules\"");
    if (rulesStart != std::string::npos) {
        size_t arrayStart = content.find("[", rulesStart);
        size_t arrayEnd = content.find("]", arrayStart);
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string rulesStr = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
            
            // 简单的对象提取：逐个 { } 对
            size_t objStart = 0;
            while ((objStart = rulesStr.find("{", objStart)) != std::string::npos) {
                size_t objEnd = rulesStr.find("}", objStart);
                if (objEnd == std::string::npos) break;
                
                std::string objStr = rulesStr.substr(objStart, objEnd - objStart + 1);
                
                // 提取规则配置
                OrganizeRule rule;
                
                // 提取 id
                size_t idPos = objStr.find("\"id\"");
                if (idPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", idPos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        rule.id = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                // 提取 name
                size_t namePos = objStr.find("\"name\"");
                if (namePos != std::string::npos) {
                    size_t colonPos = objStr.find(":", namePos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        rule.name = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                // 提取 extensions
                size_t extPos = objStr.find("\"extensions\"");
                if (extPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", extPos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        rule.extensions = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                // 提取 targetPath
                size_t pathPos = objStr.find("\"targetPath\"");
                if (pathPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", pathPos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        rule.targetPath = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
                
                // 提取 enabled
                size_t enabledPos = objStr.find("\"enabled\"");
                if (enabledPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", enabledPos);
                    size_t truePos = objStr.find("true", colonPos);
                    rule.enabled = (truePos != std::string::npos && truePos < colonPos + 20);
                }
                
                // 提取 type（新版）
                size_t typePos = objStr.find("\"type\"");
                if (typePos != std::string::npos) {
                    size_t colonPos = objStr.find(":", typePos);
                    size_t quoteStart = objStr.find("\"", colonPos);
                    size_t quoteEnd = objStr.find("\"", quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        std::string typeStr = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                        rule.type = (typeStr == "category_classify") ? OrganizeRule::TYPE_CATEGORY_CLASSIFY 
                                                                     : OrganizeRule::TYPE_LEGACY_MOVE;
                    }
                }
                
                // 提取 category（新版）
                size_t categoryPos = objStr.find("\"category\"");
                if (categoryPos != std::string::npos) {
                    size_t colonPos = objStr.find(":", categoryPos);
                    size_t valueStart = objStr.find_first_of("0123456789", colonPos);
                    size_t valueEnd = objStr.find_first_not_of("0123456789", valueStart);
                    if (valueStart != std::string::npos) {
                        std::string catStr = objStr.substr(valueStart, valueEnd - valueStart);
                        rule.category = static_cast<FileCategory>(std::stoi(catStr));
                    }
                }
                
                m_rules.push_back(rule);
                objStart = objEnd + 1;
            }
        }
    }
    
    // 如果没有配置，创建默认配置
    if (m_partitions.empty() && m_rules.empty()) {
        Logger::getInstance().info("ConfigManager: Empty config, creating default");
        createDefaultConfig();
    }
    
    Logger::getInstance().info("ConfigManager: Parsed " + std::to_string(m_partitions.size()) + 
                              " partitions and " + std::to_string(m_rules.size()) + " rules");
    return true;
}

void ConfigManager::createDefaultConfig() {
    Logger::getInstance().info("ConfigManager: Creating default configuration");
    
    // 清除现有数据
    m_partitions.clear();
    m_rules.clear();
    
    // 设置默认整理模式为桌面收纳盒
    m_organizeMode = MODE_DESKTOP_ORGANIZE;
    
    // 创建默认桌面收纳盒配置
    createDefaultDesktopOrganizeConfig();
}

void ConfigManager::createDefaultDesktopOrganizeConfig() {
    Logger::getInstance().info("ConfigManager: Creating default desktop organize configuration");
    
    // 创建6个桌面区域分区
    std::vector<FileCategory> categories = DesktopLayoutManager::getFixedCategories();
    
    for (size_t i = 0; i < categories.size(); ++i) {
        PartitionConfig partition;
        partition.id = "desktop_zone_" + std::to_string(i);
        partition.name = DesktopLayoutManager::getCategoryName(categories[i]);
        partition.type = PartitionConfig::TYPE_DESKTOP_ZONE;
        partition.category = categories[i];
        
        // 默认位置和大小（将在布局时计算）
        partition.x = 0;
        partition.y = 0;
        partition.width = 0;
        partition.height = 0;
        // 修复：使用新版 getCategoryColorCode 接口
        partition.backgroundColor = DesktopLayoutManager::getCategoryColorCode(categories[i]);
        
        m_partitions.push_back(partition);
    }
    
    // 创建默认分类规则
    std::vector<std::pair<FileCategory, std::vector<std::string>>> categoryRules = {
        {CATEGORY_SHORTCUT, {".lnk", ".url"}},
        {CATEGORY_DOCUMENT, {".doc", ".docx", ".pdf", ".txt", ".ppt", ".pptx", ".xls", ".xlsx", ".md", ".rtf"}},
        {CATEGORY_IMAGE, {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".svg", ".webp", ".tiff", ".ico"}},
        {CATEGORY_VIDEO, {".mp4", ".avi", ".mov", ".wmv", ".flv", ".mkv", ".rmvb", ".m4v"}},
        {CATEGORY_ARCHIVE, {".zip", ".rar", ".7z", ".tar", ".gz", ".bz2", ".xz", ".iso"}}
    };
    
    for (size_t i = 0; i < categoryRules.size(); ++i) {
        OrganizeRule rule;
        rule.id = "category_rule_" + std::to_string(i);
        rule.name = DesktopLayoutManager::getCategoryName(categoryRules[i].first);
        rule.type = OrganizeRule::TYPE_CATEGORY_CLASSIFY;
        rule.category = categoryRules[i].first;
        rule.enabled = true;
        
        // 拼接扩展名
        std::string extensions;
        for (size_t j = 0; j < categoryRules[i].second.size(); ++j) {
            if (j > 0) extensions += ",";
            extensions += categoryRules[i].second[j];
        }
        rule.extensions = extensions;
        
        // 目标路径留空（不移动文件）
        rule.targetPath = "";
        
        m_rules.push_back(rule);
    }
    
    Logger::getInstance().info("ConfigManager: Created " + std::to_string(m_partitions.size()) + 
                              " desktop zones and " + std::to_string(m_rules.size()) + " category rules");
}

void ConfigManager::addPartition(const PartitionConfig& partition) {
    m_partitions.push_back(partition);
    Logger::getInstance().info("ConfigManager: Added partition: " + partition.name);
}

void ConfigManager::removePartition(const std::string& id) {
    auto it = std::remove_if(m_partitions.begin(), m_partitions.end(),
                           [&id](const PartitionConfig& p) { return p.id == id; });
    if (it != m_partitions.end()) {
        m_partitions.erase(it, m_partitions.end());
        Logger::getInstance().info("ConfigManager: Removed partition with id: " + id);
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
    auto it = std::remove_if(m_rules.begin(), m_rules.end(),
                           [&id](const OrganizeRule& r) { return r.id == id; });
    if (it != m_rules.end()) {
        m_rules.erase(it, m_rules.end());
        Logger::getInstance().info("ConfigManager: Removed rule with id: " + id);
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

ConfigManager::OrganizeMode ConfigManager::getOrganizeMode() const {
    return m_organizeMode;
}

void ConfigManager::setOrganizeMode(OrganizeMode mode) {
    m_organizeMode = mode;
    Logger::getInstance().info("ConfigManager: Organize mode set to: " + organizeModeToString(mode));
}

//=============================================================================
// 自动整理设置
//=============================================================================

bool ConfigManager::isAutoArrangeEnabled() const {
    return m_autoArrangeEnabled;
}

void ConfigManager::setAutoArrangeEnabled(bool enabled) {
    m_autoArrangeEnabled = enabled;
    Logger::getInstance().info("ConfigManager: Auto-arrange enabled set to: " +
                              std::string(enabled ? "true" : "false"));
}

bool ConfigManager::isAutoArrangeOnStartup() const {
    return m_autoArrangeOnStartup;
}

void ConfigManager::setAutoArrangeOnStartup(bool enabled) {
    m_autoArrangeOnStartup = enabled;
    Logger::getInstance().info("ConfigManager: Auto-arrange on startup set to: " +
                              std::string(enabled ? "true" : "false"));
}

} // namespace ccdesk::core