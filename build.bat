@echo off
REM CCDesk 编译脚本 (批处理版本)
REM 用法: build.bat 或 build.bat --clean
REM
REM 环境需求:
REM - CMake 3.20+
REM - Visual Studio 2019/2022
REM - Qt 6.x (6.2+)
REM - 环境变量: QT_PATH 或 CMAKE_PREFIX_PATH

setlocal enabledelayedexpansion

REM 颜色定义 (仅支持 Windows 10+)
for /F %%A in ('copy /Z "%~f0" nul') do set "BS=%%A"

echo.
echo ===============================================
echo   CCDesk Compilation Script (Batch Version)
echo   Qt Desktop Organizer MVF v1.0
echo ===============================================
echo.

REM 检查环境变量
if not defined QT_PATH (
    if not defined CMAKE_PREFIX_PATH (
        echo [ERROR] Qt path not set!
        echo.
        echo Please set one of these environment variables:
        echo   setx QT_PATH "C:\Qt\6.4\msvc2019_64"
        echo   OR
        echo   setx CMAKE_PREFIX_PATH "C:\Qt\6.4\msvc2019_64"
        echo.
        pause
        exit /b 1
    ) else (
        set "QT_PATH=!CMAKE_PREFIX_PATH!"
    )
)

echo [INFO] Qt Path: !QT_PATH!

REM 检查 Qt 路径是否存在
if not exist "!QT_PATH!" (
    echo [ERROR] Qt path does not exist: !QT_PATH!
    pause
    exit /b 1
)

REM 解析参数
set CLEAN_BUILD=0
if "%1"=="--clean" set CLEAN_BUILD=1
if "%1"=="-c" set CLEAN_BUILD=1

REM 准备编译目录
echo.
echo [*] Preparing build environment...

if %CLEAN_BUILD%==1 (
    if exist build (
        echo [+] Removing old build directory...
        rmdir /s /q build 2>nul
    )
)

if not exist build (
    echo [+] Creating build directory...
    mkdir build
) else (
    echo [*] Build directory exists
)

cd build

REM CMake 配置
echo.
echo [*] Running CMake configuration...
echo     Command: cmake .. -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH=!QT_PATH!
echo.

cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH=!QT_PATH! ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM 编译
echo.
echo [*] Starting compilation (Release mode, 4 jobs)...
echo.

cmake --build . --config Release --parallel 4

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    cd ..
    pause
    exit /b 1
)

REM 验证输出
echo.
echo [*] Verifying build output...

if not exist "bin\CCDesk.exe" (
    echo [ERROR] Executable not found: bin\CCDesk.exe
    cd ..
    pause
    exit /b 1
)

REM 获取文件信息
for %%F in (bin\CCDesk.exe) do (
    set "FILE_SIZE=%%~zF"
    for /f "tokens=1-2" %%A in ('wmic datafile where name="%%~fF" get Description^,Version /format:value 2^>nul') do (
        for /f "tokens=1-2 delims==" %%X in ("%%A %%B") do (
            if "%%X"=="Description" set "FILE_DESC=%%Y"
            if "%%X"=="Version" set "FILE_VER=%%Y"
        )
    )
)

echo.
echo [SUCCESS] Build completed successfully!
echo.
echo Build Information:
echo   Executable:     bin\CCDesk.exe
echo   File Size:      !FILE_SIZE! bytes

REM 下一步提示
echo.
echo Next Steps:
echo   1. Run:     .\bin\CCDesk.exe
echo   2. Deploy:  .\bin\CCDesk.exe should be deployed with Qt DLLs
echo   3. Package: Use windeployqt.exe to package dependencies
echo.

cd ..
pause
exit /b 0
