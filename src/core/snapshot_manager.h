#ifndef SNAPSHOT_MANAGER_H
#define SNAPSHOT_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "desktop_auto_arrange_service.h"

namespace ccdesk::core {

/**
 * 桌面布局快照管理器 v1
 *
 * 职责：
 *   - 管理桌面布局快照的持久化存储
 *   - 保存执行前位置快照
 *   - 读取最近一次快照
 *   - 为后续历史快照列表预留结构
 */
class SnapshotManager {
public:
    SnapshotManager();
    ~SnapshotManager() = default;

    // 禁止拷贝
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;

    /**
     * 保存快照到磁盘
     *
     * @param snapshot 快照数据
     * @return true 如果保存成功
     */
    bool saveSnapshot(const DesktopLayoutSnapshot& snapshot);

    /**
     * 加载最近一次快照
     *
     * @param snapshot 输出快照数据
     * @return true 如果加载成功，false 如果快照不存在或加载失败
     */
    bool loadLastSnapshot(DesktopLayoutSnapshot& snapshot);

    /**
     * 检查是否有可用快照
     *
     * @return true 如果存在快照文件
     */
    bool hasSnapshot() const;

    /**
     * 删除所有快照
     *
     * @return true 如果删除成功
     */
    bool clearAllSnapshots();

    /**
     * 获取快照文件路径
     *
     * @return 快照文件路径
     */
    const std::string& getSnapshotFilePath() const;

private:
    /**
     * 获取快照目录路径
     *
     * @return 快照目录路径
     */
    std::string getSnapshotDirectory() const;

    /**
     * 确保快照目录存在
     *
     * @return true 如果目录存在或创建成功
     */
    bool ensureSnapshotDirectoryExists() const;

    /**
     * 序列化快照为 JSON 字符串
     *
     * @param snapshot 快照数据
     * @return JSON 字符串
     */
    std::string serializeSnapshot(const DesktopLayoutSnapshot& snapshot) const;

    /**
     * 从 JSON 字符串反序列化快照
     *
     * @param json JSON 字符串
     * @param snapshot 输出快照数据
     * @return true 如果反序列化成功
     */
    bool deserializeSnapshot(const std::string& json, DesktopLayoutSnapshot& snapshot) const;

private:
    std::string m_snapshotFilePath;  // 快照文件路径
};

} // namespace ccdesk::core

#endif // SNAPSHOT_MANAGER_H
