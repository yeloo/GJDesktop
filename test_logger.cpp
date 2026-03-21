#include "src/core/logger.h"
#include <iostream>

int main() {
    // 测试 logger.h 的 formatMessage 实现
    std::cout << "Testing logger.h formatMessage..." << std::endl;

    // 测试1：基本格式化
    std::string result1 = ccdesk::core::Logger::formatMessage("Hello %s", "World");
    std::cout << "Test 1: " << result1 << std::endl;

    // 测试2：多参数
    std::string result2 = ccdesk::core::Logger::formatMessage("Int: %d, Float: %.2f, String: %s", 42, 3.14, "test");
    std::cout << "Test 2: " << result2 << std::endl;

    // 测试3：Logger 调用
    ccdesk::core::Logger::getInstance().debug("Debug message: %s, %d", "test", 123);
    ccdesk::core::Logger::getInstance().info("Info message: %s, %d", "test", 456);
    ccdesk::core::Logger::getInstance().warning("Warning message: %s, %d", "test", 789);
    ccdesk::core::Logger::getInstance().error("Error message: %s, %d", "test", 999);

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
