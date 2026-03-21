#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <vector>

namespace ccdesk::core {

class Logger {
public:
    enum LogLevel {
        LOG_DEBUG = 0,
        LOG_INFO = 1,
        LOG_WARNING = 2,
        LOG_ERROR = 3
    };
    
    static Logger& getInstance();
    
    // 日志记录方法（格式化版本）
    template<typename... Args>
    void debug(const char* format, Args... args) {
        log(LOG_DEBUG, formatMessage(format, args...));
    }

    template<typename... Args>
    void info(const char* format, Args... args) {
        log(LOG_INFO, formatMessage(format, args...));
    }

    template<typename... Args>
    void warning(const char* format, Args... args) {
        log(LOG_WARNING, formatMessage(format, args...));
    }

    template<typename... Args>
    void error(const char* format, Args... args) {
        log(LOG_ERROR, formatMessage(format, args...));
    }

    // 简单版本（保持向后兼容）
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    // 设置日志输出路径
    void setLogPath(const std::string& path);
    
    // 设置日志级别
    void setLogLevel(LogLevel level);
    
    // 获取日志文件路径
    std::string getLogPath() const;

private:
    Logger();
    ~Logger();
    
    // 禁止拷贝
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 日志写入实现
    void log(LogLevel level, const std::string& message);
    
    // 获取时间戳字符串
    std::string getTimestamp() const;
    
    // 获取日志级别字符串
    std::string getLevelString(LogLevel level) const;

    // 格式化消息（可变参数模板）- 安全版本
    // 使用 vector<char> 作为临时缓冲区，避免 std::string 越界风险
    // 注意：本实现不支持 MSVC 下的 std::string 直接传递，需要使用 .c_str()
    // 变参函数不能在模板中使用，改用 std::ostringstream 实现
    template<typename... Args>
    static std::string formatMessage(const char* format, Args... args) {
        // MSVC 不支持在模板中使用 va_start/va_end
        // 改为使用 snprintf 格式化
        if constexpr (sizeof...(Args) == 0) {
            // 没有额外参数，直接返回格式字符串
            return format;
        } else {
            // 使用 snprintf 格式化
            // 计算缓冲区大小
            int size = std::snprintf(nullptr, 0, format, args...);
            if (size <= 0) {
                return format;
            }

            // 分配缓冲区
            std::vector<char> buffer(size + 1, '\0');
            std::snprintf(buffer.data(), size + 1, format, args...);

            return std::string(buffer.data());
        }
    }

    // 成员变量
    std::string m_logPath;
    LogLevel m_logLevel;
    std::ofstream m_logFile;
};

} // namespace ccdesk::core

#endif // LOGGER_H
