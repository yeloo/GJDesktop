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

        // 解析 positions 数组
        size_t positionsStart = json.find("\"positions\": [");
        if (positionsStart != std::string::npos) {
            size_t positionsEnd = json.find("]", positionsStart);
            if (positionsEnd != std::string::npos) {
                std::string positionsArray = json.substr(positionsStart + 15, positionsEnd - (positionsStart + 15));

                // 解析每个 position 对象
                size_t pos = 0;
                while (pos < positionsArray.length()) {
                    // 跳过空格和逗号
                    while (pos < positionsArray.length() && (positionsArray[pos] == ' ' || positionsArray[pos] == ',' || positionsArray[pos] == '\n')) {
                        pos++;
                    }
                    if (pos >= positionsArray.length()) break;

                    // 检查是否到达数组末尾
                    if (positionsArray[pos] == ']') break;

                    // 查找对象开始
                    if (positionsArray[pos] != '{') {
                        pos++;
                        continue;
                    }

                    // 解析 displayName
                    size_t displayNameStart = positionsArray.find("\"displayName\": \"", pos);
                    if (displayNameStart == std::string::npos) { pos++; continue; }
                    size_t displayNameEnd = positionsArray.find("\"", displayNameStart + 17);
                    if (displayNameEnd == std::string::npos) { pos++; continue; }
                    std::string displayName = positionsArray.substr(displayNameStart + 17, displayNameEnd - (displayNameStart + 17));

                    // 解析 parsingName
                    size_t parsingNameStart = positionsArray.find("\"parsingName\": \"", displayNameEnd);
                    if (parsingNameStart == std::string::npos) { pos = displayNameEnd + 1; continue; }
                    size_t parsingNameEnd = positionsArray.find("\"", parsingNameStart + 17);
                    if (parsingNameEnd == std::string::npos) { pos = parsingNameStart + 1; continue; }
                    std::string parsingName = positionsArray.substr(parsingNameStart + 17, parsingNameEnd - (parsingNameStart + 17));

                    // 解析 originalPosition.x
                    size_t xPosStart = positionsArray.find("\"x\": ", parsingNameEnd);
                    if (xPosStart == std::string::npos) { pos = parsingNameEnd + 1; continue; }
                    size_t xPosEnd = positionsArray.find(",", xPosStart);
                    if (xPosEnd == std::string::npos) { pos = xPosStart + 1; continue; }
                    std::string xStr = positionsArray.substr(xPosStart + 5, xPosEnd - (xPosStart + 5));

                    // 解析 originalPosition.y
                    size_t yPosStart = positionsArray.find("\"y\": ", xPosEnd);
                    if (yPosStart == std::string::npos) { pos = xPosEnd + 1; continue; }
                    size_t yPosEnd = positionsArray.find("}", yPosStart);
                    if (yPosEnd == std::string::npos) { pos = yPosStart + 1; continue; }
                    std::string yStr = positionsArray.substr(yPosStart + 5, yPosEnd - (yPosStart + 5));

                    // 解析 category
                    size_t categoryStart = positionsArray.find("\"category\": \"", yPosEnd);
                    if (categoryStart == std::string::npos) { pos = yPosEnd + 1; continue; }
                    size_t categoryEnd = positionsArray.find("\"", categoryStart + 14);
                    if (categoryEnd == std::string::npos) { pos = categoryStart + 1; continue; }
                    std::string category = positionsArray.substr(categoryStart + 14, categoryEnd - (categoryStart + 14));

                    // 创建 DesktopIconPositionSnapshot
                    POINT position;
                    position.x = std::stol(xStr);
                    position.y = std::stol(yStr);

                    snapshot.positions.push_back(DesktopIconPositionSnapshot(
                        displayName,
                        parsingName,
                        position,
                        category
                    ));

                    pos = categoryEnd + 1;
                }
            }
        }

        Logger::getInstance().info(
            "SnapshotManager: 反序列化完成 - 时间: %s, 图标数: %zu",
            snapshot.timestamp.c_str(),
            snapshot.totalCount
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
