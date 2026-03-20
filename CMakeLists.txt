cmake_minimum_required(VERSION 3.20)
project(CCDesk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用异常处理（MSVC）
if(MSVC)
    add_compile_options(/EHsc)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

set(SOURCES
    src/main.cpp
    src/core/logger.h
    src/core/logger.cpp
    src/core/config_manager.h
    src/core/config_manager.cpp
    src/core/file_organizer.h
    src/core/file_organizer.cpp
    src/core/organize_result.h
    src/core/desktop_layout_manager.h
    src/core/desktop_layout_manager.cpp
    src/core/tray_manager.h
    src/core/tray_manager.cpp
    src/core/app_manager.h
    src/core/app_manager.cpp
    src/ui/main_window.h
    src/ui/main_window.cpp
    src/ui/settings_dialog.h
    src/ui/settings_dialog.cpp
)

add_executable(CCDesk ${SOURCES})

target_link_libraries(CCDesk
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(WIN32)
    # Windows特定库（用于注册表操作）
    target_link_libraries(CCDesk advapi32)
endif()

target_include_directories(CCDesk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Windows GUI 子系统设置：避免双击运行出现控制台黑框
if(WIN32)
    set_target_properties(CCDesk PROPERTIES 
        WIN32_EXECUTABLE TRUE  # 设置为 Windows GUI 应用程序
    )
endif()

set_target_properties(CCDesk PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    # 显式指定 Multi-Config (如 MSVC) 生成器的配置后缀
    # 这确保输出到 build/bin (而不是 build/bin/Release)
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 针对 Multi-Config 生成器 (MSVC)，防止自动添加配置子目录
if(CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set_target_properties(CCDesk PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin"
        )
    endforeach()
endif()
