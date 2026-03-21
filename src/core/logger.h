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
    template<typename... Args>
    static std::string formatMessage(const char* format, Args... args) {
        // 第一步：计算所需缓冲区大小
        int size = 0;
        {
            va_list va_args;
            va_start(va_args, format);
#ifdef _MSC_VER
            size = _vscprintf(format, va_args);
#else
            size = vsnprintf(nullptr, 0, format, va_args);
#endif
            va_end(va_args);
        }

        if (size <= 0) {
            // 格式化失败，返回原始格式字符串
            return format;
        }

        // 第二步：分配足够大小的缓冲区（多分配1字节用于null终止符）
        std::vector<char> buffer(size + 1, '\0');

        // 第三步：执行实际的格式化
        {
            va_list va_args;
            va_start(va_args, format);
#ifdef _MSC_VER
            vsnprintf_s(buffer.data(), size + 1, _TRUNCATE, format, va_args);
#else
            vsnprintf(buffer.data(), size + 1, format, va_args);
#endif
            va_end(va_args);
        }

        // 第四步：转换为 std::string（buffer[size] 保证是 '\0'）
        return std::string(buffer.data());
    }

    // 成员变量
    std::string m_logPath;
    LogLevel m_logLevel;
    std::ofstream m_logFile;
};

} // namespace ccdesk::core

#endif // LOGGER_H
