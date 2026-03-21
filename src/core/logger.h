#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <sstream>

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
    
    // 日志记录方法
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
    
    // 成员变量
    std::string m_logPath;
    LogLevel m_logLevel;
    std::ofstream m_logFile;
};

} // namespace ccdesk::core

#endif // LOGGER_H
