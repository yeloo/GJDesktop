#include <QApplication>
#include "src/core/app_manager.h"

int main(int argc, char* argv[]) {
    // 创建Qt应用
    QApplication app(argc, argv);
    
    // 设置应用名称（用于托盘图标和注册表）
    app.setApplicationName("CCDesk");
    app.setApplicationVersion("1.0.0");
    
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
