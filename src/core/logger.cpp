#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace ccdesk::core {

static Logger* g_logger = nullptr;

Logger& Logger::getInstance() {
    if (!g_logger) {
        g_logger = new Logger();
    }
    return *g_logger;
}

Logger::Logger()
    : m_logLevel(INFO)
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
        case DEBUG:   return "DEBUG";
        case INFO:    return "INFO";
        case WARNING: return "WARN";
        case ERROR:   return "ERROR";
        default:      return "UNKNOWN";
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
    
    // 输出到文件
    if (m_logFile.is_open()) {
        m_logFile << logMessage << std::endl;
        m_logFile.flush();
    }
    
    // 输出到控制台
    std::cout << logMessage << std::endl;
}

void Logger::debug(const std::string& message) {
    log(DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(INFO, message);
}

void Logger::warning(const std::string& message) {
    log(WARNING, message);
}

void Logger::error(const std::string& message) {
    log(ERROR, message);
}

} // namespace ccdesk::core
