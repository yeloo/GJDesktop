#ifndef ORGANIZE_RESULT_H
#define ORGANIZE_RESULT_H

#include <string>
#include <vector>

namespace ccdesk::core {

// 单个文件执行结果
struct OrganizeResult {
    std::string sourcePath;      // 源文件完整路径
    std::string fileName;        // 源文件名
    std::string targetPath;      // 目标路径（若成功）
    std::string matchedRuleName; // 匹配的规则名称
    
    enum Status {
        Success,           // 成功移动
        SkippedConflict,   // 冲突跳过（目标文件已存在）
        Failed,            // 移动失败
        NoRule             // 无匹配规则
    };
    
    Status finalStatus;   // 最终状态
    std::string message;  // 错误信息或状态说明
};

// 整理结果汇总
struct OrganizeSummary {
    int movedCount;           // 成功移动数
    int skippedConflictCount; // 冲突跳过数
    int failedCount;          // 失败数
    int noRuleCount;          // 无匹配规则数
    
    std::vector<OrganizeResult> details; // 详细结果列表
    
    OrganizeSummary() 
        : movedCount(0), skippedConflictCount(0), failedCount(0), noRuleCount(0) {}
    
    // 获取总处理文件数
    int getTotalProcessed() const {
        return movedCount + skippedConflictCount + failedCount + noRuleCount;
    }
};

} // namespace ccdesk::core

#endif // ORGANIZE_RESULT_H
