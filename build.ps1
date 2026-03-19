#!/usr/bin/env powershell
# CCDesk 编译脚本 (PowerShell 版本)
# 使用方法: .\build.ps1 -QtPath "C:\Qt\6.4\msvc2019_64"

param(
    [string]$QtPath = $env:QT_PATH,
    [int]$Jobs = [System.Environment]::ProcessorCount,
    [string]$Config = "Release",
    [switch]$Clean
)

# 颜色定义
$colors = @{
    'Reset' = "`e[0m"
    'Green' = "`e[32m"
    'Red' = "`e[31m"
    'Yellow' = "`e[33m"
    'Cyan' = "`e[36m"
    'Blue' = "`e[34m"
}

function Write-Status {
    param([string]$Message, [string]$Type = "Info")
    $timestamp = Get-Date -Format "HH:mm:ss"
    
    switch ($Type) {
        "Success" { Write-Host "[$timestamp] ✓ $Message" -ForegroundColor Green }
        "Error" { Write-Host "[$timestamp] ✗ $Message" -ForegroundColor Red }
        "Warning" { Write-Host "[$timestamp] ⚠ $Message" -ForegroundColor Yellow }
        "Info" { Write-Host "[$timestamp] * $Message" -ForegroundColor Cyan }
        "Step" { Write-Host "[$timestamp] → $Message" -ForegroundColor Blue }
    }
}

function Test-Tool {
    param([string]$Tool, [string]$TestCmd)
    
    try {
        $result = Invoke-Expression $TestCmd -ErrorAction Stop 2>&1
        Write-Status "$Tool found: $($result[0])" "Success"
        return $true
    } catch {
        Write-Status "$Tool not found" "Error"
        return $false
    }
}

# 主函数
function Build-CCDesk {
    Write-Host ""
    Write-Host "╔══════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║      CCDesk Compilation Script          ║" -ForegroundColor Cyan
    Write-Host "║      Qt Desktop Organizer (MVP v1.0)    ║" -ForegroundColor Cyan
    Write-Host "╚══════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""
    
    # 参数验证
    if ([string]::IsNullOrEmpty($QtPath)) {
        Write-Status "Qt path not provided. Use: .\build.ps1 -QtPath 'C:\Qt\6.4\msvc2019_64'" "Error"
        Write-Status "Or set environment variable: `$env:QT_PATH = 'C:\Qt\6.4\msvc2019_64'" "Warning"
        return $false
    }
    
    if (-not (Test-Path $QtPath)) {
        Write-Status "Qt path does not exist: $QtPath" "Error"
        return $false
    }
    
    Write-Status "Build Configuration" "Info"
    Write-Host "  Qt Path:          $QtPath"
    Write-Host "  Configuration:    $Config"
    Write-Host "  Parallel Jobs:    $Jobs"
    Write-Host "  Clean Build:      $Clean"
    Write-Host ""
    
    # 工具检查
    Write-Status "Checking build tools..." "Step"
    
    $toolsOk = $true
    $toolsOk = (Test-Tool "CMake" "cmake --version") -and $toolsOk
    $toolsOk = (Test-Tool "MSBuild" "msbuild --version") -and $toolsOk
    
    if (-not $toolsOk) {
        Write-Status "Some tools are missing. Please install required tools." "Error"
        Write-Status "See COMPILATION_STATUS.md for installation instructions" "Warning"
        return $false
    }
    
    Write-Host ""
    Write-Status "Preparing build environment..." "Step"
    
    # 清理旧编译
    if ($Clean -or -not (Test-Path "build")) {
        if (Test-Path "build") {
            Write-Status "Cleaning old build directory..."
            Remove-Item -Recurse -Force "build" -ErrorAction SilentlyContinue
        }
        Write-Status "Creating new build directory..."
        New-Item -ItemType Directory -Path "build" -Force | Out-Null
    }
    
    Set-Location "build"
    
    # CMake 配置
    Write-Host ""
    Write-Status "Running CMake configuration..." "Step"
    Write-Host "  Command: cmake .. -G 'Visual Studio 17 2022' -DCMAKE_PREFIX_PATH=$QtPath"
    Write-Host ""
    
    & cmake .. -G "Visual Studio 17 2022" `
        -DCMAKE_PREFIX_PATH=$QtPath `
        -DCMAKE_BUILD_TYPE=$Config
    
    if ($LASTEXITCODE -ne 0) {
        Write-Status "CMake configuration failed!" "Error"
        Set-Location ".."
        return $false
    }
    
    Write-Status "CMake configuration completed" "Success"
    
    # 编译
    Write-Host ""
    Write-Status "Starting compilation..." "Step"
    Write-Host "  Command: cmake --build . --config $Config --parallel $Jobs"
    Write-Host ""
    
    $startTime = Get-Date
    & cmake --build . --config $Config --parallel $Jobs
    $endTime = Get-Date
    $duration = ($endTime - $startTime).TotalSeconds
    
    if ($LASTEXITCODE -ne 0) {
        Write-Status "Build failed!" "Error"
        Set-Location ".."
        return $false
    }
    
    # 验证输出
    Write-Host ""
    Write-Status "Verifying build output..." "Step"
    
    $exePath = "bin\CCDesk.exe"
    if (-not (Test-Path $exePath)) {
        Write-Status "Executable not found: $exePath" "Error"
        Set-Location ".."
        return $false
    }
    
    $fileSize = (Get-Item $exePath).Length / 1MB
    $timestamp = (Get-Item $exePath).LastWriteTime
    
    Write-Host ""
    Write-Status "Build completed successfully!" "Success"
    Write-Host "  Executable:     $exePath"
    Write-Host "  File Size:      $([math]::Round($fileSize, 2)) MB"
    Write-Host "  Modified:       $timestamp"
    Write-Host "  Build Time:     $([math]::Round($duration, 1)) seconds"
    
    # 部署建议
    Write-Host ""
    Write-Status "Next Steps:" "Info"
    Write-Host "  1. Run:     .\bin\CCDesk.exe"
    Write-Host "  2. Deploy:  `$QtBinPath = '$QtPath\bin'; & `"`$QtBinPath\windeployqt.exe`" bin\CCDesk.exe --release"
    Write-Host "  3. Package: Copy bin\CCDesk.exe and required DLLs to release folder"
    
    Set-Location ".."
    return $true
}

# 执行编译
try {
    $result = Build-CCDesk
    exit ($result -eq $true ? 0 : 1)
} catch {
    Write-Status "Unexpected error: $_" "Error"
    exit 1
}
