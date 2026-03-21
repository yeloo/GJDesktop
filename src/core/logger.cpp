#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace ccdesk::core {

// 静态单例实例（使用 Meyer's Singleton Pattern）

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_logLevel(LOG_INFO)
{
    // 创建logs目录
    std::string logsDir = "logs";
    if (!fs::exists(logsDir)) {
        try {
            fs::create_directories(logsDir);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create logs directory: " << e.what() << std::endl;
            return;
        }
    }
    
    // 设置默认日志路径
    setLogPath(logsDir + "/ccdesk.log");
    
    // 应用启动时间戳
    info("==================================================");
    info("CCDesk application started");
    info("==================================================");
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        info("==================================================");
        info("CCDesk application closed");
        info("==================================================");
        m_logFile.close();
    }
}

void Logger::setLogPath(const std::string& path) {
    m_logPath = path;
    
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    m_logFile.open(m_logPath, std::ios::app);
    
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << m_logPath << std::endl;
    }
}

void Logger::setLogLevel(LogLevel level) {
    m_logLevel = level;
}

std::string Logger::getLogPath() const {
    return m_logPath;
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::getLevelString(LogLevel level) const {
    switch (level) {
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARN";
        case LOG_ERROR:   return "ERROR";
        default:          return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_logLevel) {
        return;
    }
    
    std::string timestamp = getTimestamp();
    std::string levelStr = getLevelString(level);
    
    std::stringstream ss;
    ss << "[" << timestamp << "] [" << levelStr << "] " << message;
    std::string logMessage = ss.str();
    
    // 输出到文件（Release 和 Debug 模式都写入文件）
    if (m_logFile.is_open()) {
        m_logFile << logMessage << std::endl;
        m_logFile.flush();
    }
    
    // 控制台输出策略：仅在 Debug 模式下输出到控制台
    // 在 Release 模式下，GUI 程序不应弹出控制台窗口
#ifdef NDEBUG
    // Release 模式：不输出到控制台，避免弹出黑框窗口
    // 日志已经写入文件，用户可以在需要时查看日志文件
#else
    // Debug 模式：输出到控制台，方便开发调试
    std::cout << logMessage << std::endl;
#endif
}

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
