@echo off
echo ========================================
echo 智能吞噬进化 - 自动构建和运行脚本
echo ========================================

cd /d "D:\Coding\Projects\ai-devour-evolve"

echo [1/4] 清理旧的构建文件...
if exist build rmdir /s /q build
mkdir build

echo [2/4] 配置CMake项目...
D:\qt\Tools\CMake_64\bin\cmake.exe -B build -S . -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo 配置失败！
    pause
    exit /b 1
)

echo [3/4] 编译项目...
D:\qt\Tools\CMake_64\bin\cmake.exe --build build --config Release
if %errorlevel% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)

echo [4/4] 部署Qt依赖...
D:\qt\6.9.1\msvc2022_64\bin\windeployqt.exe --release build\Release\ai-devour-evolve.exe

echo.
echo ========================================
echo 构建完成！正在启动程序...
echo ========================================
echo.

REM 运行程序
.\build\Release\ai-devour-evolve.exe

echo.
echo 程序已退出
pause
