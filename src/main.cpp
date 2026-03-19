#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include "core/app_manager.h"
#include "core/logger.h"

int main(int argc, char* argv[]) {
    // 创建Qt应用
    QApplication app(argc, argv);
    
    // 设置应用名称（用于托盘图标和注册表）
    app.setApplicationName("CCDesk");
    app.setApplicationVersion("1.0.0");
    
    // 设置应用程序组织和域名（用于标准路径）
    app.setOrganizationName("CCDesk");
    app.setOrganizationDomain("ccdesk.example.com");
    
    // 配置应用程序数据目录（用户数据目录）
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 初始化日志系统：使用应用程序数据目录存储日志
    auto& logger = ccdesk::core::Logger::getInstance();
    QString logPath = appDataDir + "/logs/ccdesk.log";
    dir.mkpath(appDataDir + "/logs");  // 确保 logs 目录存在
    logger.setLogPath(logPath.toStdString());
    logger.info("应用程序启动，日志文件: " + logPath.toStdString());
    
    // 初始化AppManager
    auto& appMgr = ccdesk::core::AppManager::getInstance();
    if (!appMgr.initialize()) {
        return -1;
    }
    
    // 运行应用
    appMgr.run();
    
    // 进入事件循环
    int result = app.exec();
    
    // 清理
    appMgr.cleanup();
    
    return result;
}
