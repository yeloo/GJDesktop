#include "config_manager.h"
#include "logger.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <QStandardPaths>
#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace fs = std::filesystem;

namespace ccdesk::core {

ConfigManager::ConfigManager()
    : m_startupEnabled(false)
    , m_organizeMode(MODE_DESKTOP_ORGANIZE)
    , m_autoArrangeEnabled(false)
    , m_autoArrangeOnStartup(false)
{
    // Windows: QString (UTF-16) -> toStdWString() -> fs::path
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    fs::path configDir = fs::path(appDataDir.toStdWString());
    if (!fs::exists(configDir)) {
        fs::create_directories(configDir);
    }

    // P1-2: 统一使用 UTF-8 路径字符串
#ifdef _WIN32
    m_configPath = QString::fromStdWString((configDir / "ccdesk_config.json").wstring())
                       .toUtf8().constData();
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

    // P1-1: 使用 QFile 读取，避免 Windows 路径编码问题
    QString qPath = QString::fromUtf8(m_configPath.c_str());
    QFile file(qPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::getInstance().error("ConfigManager: Failed to open config file");
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    return parseConfig(data);
}

bool ConfigManager::save() {
    Logger::getInstance().info("ConfigManager: Saving configuration to " + m_configPath);

    // 确保目录存在
    fs::path dirPath = fs::path(m_configPath).parent_path();
    if (!fs::exists(dirPath)) {
        fs::create_directories(dirPath);
    }

    // P1-1: 用 QJsonDocument 序列化
    QJsonObject root;
    root["version"] = "2.0";
    root["startup_enabled"] = m_startupEnabled;
    root["organize_mode"] = QString::fromStdString(organizeModeToString(m_organizeMode));
    root["auto_arrange_enabled"] = m_autoArrangeEnabled;
    root["auto_arrange_on_startup"] = m_autoArrangeOnStartup;

    // 分区数组
    QJsonArray partitionsArray;
    for (const auto& p : m_partitions) {
        QJsonObject obj;
        obj["id"]              = QString::fromStdString(p.id);
        obj["name"]            = QString::fromStdString(p.name);
        obj["targetPath"]      = QString::fromStdString(p.targetPath);
        obj["type"]            = (p.type == PartitionConfig::TYPE_DESKTOP_ZONE)
                                     ? "desktop_zone" : "legacy_folder";
        obj["category"]        = static_cast<int>(p.category);
        obj["x"]               = p.x;
        obj["y"]               = p.y;
        obj["width"]           = p.width;
        obj["height"]          = p.height;
        obj["backgroundColor"] = QString::fromStdString(p.backgroundColor);
        partitionsArray.append(obj);
    }
    root["partitions"] = partitionsArray;

    // 规则数组
    QJsonArray rulesArray;
    for (const auto& r : m_rules) {
        QJsonObject obj;
        obj["id"]         = QString::fromStdString(r.id);
        obj["name"]       = QString::fromStdString(r.name);
        obj["extensions"] = QString::fromStdString(r.extensions);
        obj["targetPath"] = QString::fromStdString(r.targetPath);
        obj["enabled"]    = r.enabled;
        obj["type"]       = (r.type == OrganizeRule::TYPE_CATEGORY_CLASSIFY)
                                ? "category_classify" : "legacy_move";
        obj["category"]   = static_cast<int>(r.category);
        rulesArray.append(obj);
    }
    root["rules"] = rulesArray;

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    QString qPath = QString::fromUtf8(m_configPath.c_str());
    QFile file(qPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::getInstance().error("ConfigManager: Failed to open config file for writing");
        return false;
    }
    file.write(jsonData);
    file.close();

    Logger::getInstance().info("ConfigManager: Configuration saved successfully");
    return true;
}

bool ConfigManager::parseConfig(const QByteArray& data) {
    Logger::getInstance().info("ConfigManager: Parsing configuration (QJsonDocument)");

    m_partitions.clear();
    m_rules.clear();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        Logger::getInstance().error("ConfigManager: JSON parse error: " +
                                    parseError.errorString().toUtf8().toStdString());
        createDefaultConfig();
        return false;
    }

    if (!doc.isObject()) {
        Logger::getInstance().error("ConfigManager: Root is not a JSON object");
        createDefaultConfig();
        return false;
    }

    QJsonObject root = doc.object();

    // version
    QString version = root.value("version").toString("1.0");
    Logger::getInstance().info("ConfigManager: Config version: " + version.toUtf8().toStdString());

    // startup_enabled
    m_startupEnabled = root.value("startup_enabled").toBool(false);

    // organize_mode
    std::string modeStr = root.value("organize_mode").toString("auto").toUtf8().constData();
    m_organizeMode = organizeModeFromString(modeStr);

    // auto_arrange_enabled
    m_autoArrangeEnabled = root.value("auto_arrange_enabled").toBool(false);

    // auto_arrange_on_startup
    m_autoArrangeOnStartup = root.value("auto_arrange_on_startup").toBool(false);

    // partitions
    QJsonArray partitionsArray = root.value("partitions").toArray();
    for (const QJsonValue& val : partitionsArray) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        PartitionConfig pc;
        pc.id              = obj.value("id").toString().toUtf8().constData();
        pc.name            = obj.value("name").toString().toUtf8().constData();
        pc.targetPath      = obj.value("targetPath").toString().toUtf8().constData();
        pc.backgroundColor = obj.value("backgroundColor").toString().toUtf8().constData();
        pc.x               = obj.value("x").toInt(0);
        pc.y               = obj.value("y").toInt(0);
        pc.width           = obj.value("width").toInt(0);
        pc.height          = obj.value("height").toInt(0);
        pc.category        = static_cast<FileCategory>(obj.value("category").toInt(0));

        std::string typeStr = obj.value("type").toString("legacy_folder").toUtf8().constData();
        pc.type = (typeStr == "desktop_zone")
                      ? PartitionConfig::TYPE_DESKTOP_ZONE
                      : PartitionConfig::TYPE_LEGACY_FOLDER;

        m_partitions.push_back(pc);
    }

    // rules
    QJsonArray rulesArray = root.value("rules").toArray();
    for (const QJsonValue& val : rulesArray) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        OrganizeRule rule;
        rule.id         = obj.value("id").toString().toUtf8().constData();
        rule.name       = obj.value("name").toString().toUtf8().constData();
        rule.extensions = obj.value("extensions").toString().toUtf8().constData();
        rule.targetPath = obj.value("targetPath").toString().toUtf8().constData();
        rule.enabled    = obj.value("enabled").toBool(true);
        rule.category   = static_cast<FileCategory>(obj.value("category").toInt(0));

        std::string typeStr = obj.value("type").toString("legacy_move").toUtf8().constData();
        rule.type = (typeStr == "category_classify")
                        ? OrganizeRule::TYPE_CATEGORY_CLASSIFY
                        : OrganizeRule::TYPE_LEGACY_MOVE;

        m_rules.push_back(rule);
    }

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