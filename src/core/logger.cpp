#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdarg>

namespace ccdesk::core {

// 静态单例实例（使用 Meyer's Singleton Pattern）
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_logLevel(LOG_INFO)  // 默认级别：临时调到 DEBUG 以便排查期
{
    // 初始化时自动设置日志路径
    // 注意：实际路径由 main.cpp 通过 setLogPath() 设置
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::setLogPath(const std::string& path) {
    m_logPath = path;
    
    // 如果已有打开的文件，先关闭
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    // 以追加模式打开日志文件
    m_logFile.open(m_logPath, std::ios::out | std::ios::app);
    
    if (!m_logFile.is_open()) {
        std::cerr << "Logger: Failed to open log file: " << m_logPath << std::endl;
    }
}

void Logger::setLogLevel(LogLevel level) {
    m_logLevel = level;
}

std::string Logger::getLogPath() const {
    return m_logPath;
}

void Logger::log(LogLevel level, const std::string& message) {
    // 检查日志级别
    if (level < m_logLevel) {
        return;
    }
    
    // 生成日志行
    std::string logLine = getTimestamp() + " [" + getLevelString(level) + "] " + message + "\n";
    
    // 输出到控制台
    std::cout << logLine;
    std::cout.flush();
    
    // 输出到文件（如果已打开）
    if (m_logFile.is_open()) {
        m_logFile << logLine;
        m_logFile.flush();
    }
}

std::string Logger::getTimestamp() const {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // 转换为本地时间
    std::tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    
    // 格式化为字符串
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm_now.tm_year + 1900) << "-"
        << std::setw(2) << (tm_now.tm_mon + 1) << "-"
        << std::setw(2) << tm_now.tm_mday << " "
        << std::setw(2) << tm_now.tm_hour << ":"
        << std::setw(2) << tm_now.tm_min << ":"
        << std::setw(2) << tm_now.tm_sec;
    
    return oss.str();
}

std::string Logger::getLevelString(LogLevel level) const {
    switch (level) {
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        default:          return "UNKNOWN";
    }
}

// 简单版本实现（保持向后兼容）
void Logger::debug(const std::string& message) {
    log(LOG_DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LOG_INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LOG_WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LOG_ERROR, message);
}

} // namespace ccdesk::core
