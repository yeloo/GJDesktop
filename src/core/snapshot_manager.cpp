#include "snapshot_manager.h"
#include "desktop_snapshot_types.h"
#include "logger.h"
#include <sstream>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;
namespace ccdesk::core {

// 快照文件名
const std::string SNAPSHOT_FILENAME = "desktop_layout_snapshot.json";

//=============================================================================
// 构造函数
//=============================================================================

SnapshotManager::SnapshotManager() {
    Logger::getInstance().info("SnapshotManager: 初始化快照管理器");

    // 获取快照目录
    std::string snapshotDir = getSnapshotDirectory();

    // 构建快照文件路径
    m_snapshotFilePath = snapshotDir + "/" + SNAPSHOT_FILENAME;

    Logger::getInstance().info(
        "SnapshotManager: 快照文件路径: %s",
        m_snapshotFilePath.c_str()
    );

    // 确保快照目录存在
    if (!ensureSnapshotDirectoryExists()) {
        Logger::getInstance().error("SnapshotManager: 无法创建快照目录");
    }
}

//=============================================================================
// 公开接口实现
//=============================================================================

bool SnapshotManager::saveSnapshot(const DesktopLayoutSnapshot& snapshot) {
    Logger::getInstance().info("SnapshotManager: 保存桌面布局快照");

    // 序列化快照
    std::string json = serializeSnapshot(snapshot);
    if (json.empty()) {
        Logger::getInstance().error("SnapshotManager: 序列化快照失败");
        return false;
    }

    // 打开文件写入
    std::ofstream outFile(m_snapshotFilePath);
    if (!outFile.is_open()) {
        Logger::getInstance().error(
            "SnapshotManager: 无法打开快照文件: %s",
            m_snapshotFilePath.c_str()
        );
        return false;
    }

    // 写入 JSON
    outFile << json;
    outFile.close();

    Logger::getInstance().info(
        "SnapshotManager: 快照保存成功 - %zu 个图标，时间: %s",
        snapshot.totalCount,
        snapshot.timestamp.c_str()
    );

    return true;
}

bool SnapshotManager::loadLastSnapshot(DesktopLayoutSnapshot& snapshot) {
    Logger::getInstance().info("SnapshotManager: 加载最近一次快照");

    // 检查文件是否存在
    if (!hasSnapshot()) {
        Logger::getInstance().warning("SnapshotManager: 快照文件不存在");
        return false;
    }

    // 打开文件读取
    std::ifstream inFile(m_snapshotFilePath);
    if (!inFile.is_open()) {
        Logger::getInstance().error(
            "SnapshotManager: 无法打开快照文件: %s",
            m_snapshotFilePath.c_str()
        );
        return false;
    }

    // 读取整个文件
    std::string json((std::istreambuf_iterator<char>(inFile)),
                    std::istreambuf_iterator<char>());
    inFile.close();

    // 反序列化
    if (!deserializeSnapshot(json, snapshot)) {
        Logger::getInstance().error("SnapshotManager: 反序列化快照失败");
        return false;
    }

    Logger::getInstance().info(
        "SnapshotManager: 快照加载成功 - %zu 个图标，时间: %s",
        snapshot.totalCount,
        snapshot.timestamp.c_str()
    );

    return true;
}

bool SnapshotManager::hasSnapshot() const {
    return fs::exists(m_snapshotFilePath);
}

bool SnapshotManager::clearAllSnapshots() {
    Logger::getInstance().info("SnapshotManager: 清除所有快照");

    if (!hasSnapshot()) {
        Logger::getInstance().info("SnapshotManager: 没有快照需要清除");
        return true;
    }

    // 删除快照文件
    try {
        fs::remove(m_snapshotFilePath);
        Logger::getInstance().info("SnapshotManager: 快照文件已删除");
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error(
            "SnapshotManager: 删除快照文件失败: %s",
            e.what()
        );
        return false;
    }
}

const std::string& SnapshotManager::getSnapshotFilePath() const {
    return m_snapshotFilePath;
}

//=============================================================================
// 私有方法实现
//=============================================================================

std::string SnapshotManager::getSnapshotDirectory() const {
    // 获取可执行文件路径
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    // 获取可执行文件所在目录
    fs::path exeDir = fs::path(exePath).parent_path();

    // 创建 snapshots 子目录
    fs::path snapshotDir = exeDir / "snapshots";

    return snapshotDir.string();
}

bool SnapshotManager::ensureSnapshotDirectoryExists() const {
    std::string snapshotDir = getSnapshotDirectory();

    try {
        if (!fs::exists(snapshotDir)) {
            fs::create_directories(snapshotDir);
            Logger::getInstance().info(
                "SnapshotManager: 创建快照目录: %s",
                snapshotDir.c_str()
            );
        }
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error(
            "SnapshotManager: 创建快照目录失败: %s",
            e.what()
        );
        return false;
    }
}

std::string SnapshotManager::serializeSnapshot(const DesktopLayoutSnapshot& snapshot) const {
    std::stringstream ss;

    ss << "{\n";
    ss << "  \"timestamp\": \"" << snapshot.timestamp << "\",\n";
    ss << "  \"totalCount\": " << snapshot.totalCount << ",\n";
    ss << "  \"positions\": [\n";

    for (size_t i = 0; i < snapshot.positions.size(); i++) {
        const auto& pos = snapshot.positions[i];

        ss << "    {\n";
        ss << "      \"displayName\": \"" << pos.displayName << "\",\n";
        ss << "      \"parsingName\": \"" << pos.parsingName << "\",\n";
        ss << "      \"originalPosition\": {\n";
        ss << "        \"x\": " << pos.originalPosition.x << ",\n";
        ss << "        \"y\": " << pos.originalPosition.y << "\n";
        ss << "      },\n";
        ss << "      \"category\": \"" << pos.category << "\"\n";
        ss << "    }";

        if (i < snapshot.positions.size() - 1) {
            ss << ",";
        }
        ss << "\n";
    }

    ss << "  ]\n";
    ss << "}\n";

    return ss.str();
}

bool SnapshotManager::deserializeSnapshot(const std::string& json, DesktopLayoutSnapshot& snapshot) const {
    // 简化版：使用字符串解析（暂不引入 JSON 库）
    // 实际生产环境应该使用 nlohmann/json 或 QJsonDocument

    try {
        // 解析时间戳
        size_t timestampPos = json.find("\"timestamp\": \"");
        if (timestampPos != std::string::npos) {
            size_t timestampEnd = json.find("\"", timestampPos + 15);
            if (timestampEnd != std::string::npos) {
                snapshot.timestamp = json.substr(timestampPos + 15, timestampEnd - (timestampPos + 15));
            }
        }

        // 解析总数
        size_t totalCountPos = json.find("\"totalCount\": ");
        if (totalCountPos != std::string::npos) {
            size_t totalCountEnd = json.find(",", totalCountPos);
            if (totalCountEnd != std::string::npos) {
                std::string countStr = json.substr(totalCountPos + 14, totalCountEnd - (totalCountPos + 14));
                snapshot.totalCount = std::stoul(countStr);
            }
        }

        // 解析 positions 数组（简化版，实际应使用 JSON 解析库）
        // 这里暂时跳过详细解析，仅解析 timestamp 和 totalCount
        // 完整实现需要引入 nlohmann/json 或使用 Qt 的 QJsonDocument

        Logger::getInstance().warning(
            "SnapshotManager: 使用简化版反序列化，仅解析 timestamp 和 totalCount"
        );

        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error(
            "SnapshotManager: 反序列化失败: %s",
            e.what()
        );
        return false;
    }
}

} // namespace ccdesk::core
