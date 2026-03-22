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
    
    // 初始化日志系统：使用运行目录的logs子目录
    auto& logger = ccdesk::core::Logger::getInstance();

    // 获取可执行文件所在目录（运行目录）
    QString appDirPath = QCoreApplication::applicationDirPath();
    QString logDirPath = appDirPath + "/logs";
    QString logPath = logDirPath + "/ccdesk.log";

    // 确保logs目录存在
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }

    // 设置日志路径和级别
    logger.setLogPath(logPath.toUtf8().constData());
    logger.setLogLevel(ccdesk::core::Logger::LOG_DEBUG);
    logger.info(std::string("应用程序启动，日志文件: ") + logPath.toUtf8().constData());
    
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
