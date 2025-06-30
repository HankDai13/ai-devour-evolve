@echo off
echo ========================================
echo 智能吞噬进化 - 快速运行脚本
echo ========================================

cd /d "D:\Coding\Projects\ai-devour-evolve"
$env:QT_QPA_PLATFORM_PLUGIN_PATH="D:\qt\6.9.1\msvc2022_64\plugins\platforms"
if not exist "build\Release\ai-devour-evolve.exe" (
    echo 程序还没有编译，请先运行 build_and_run.bat
    pause
    exit /b 1
)

echo 正在启动程序...
.\build\Release\ai-devour-evolve.exe

echo.
echo 程序已退出
pause
