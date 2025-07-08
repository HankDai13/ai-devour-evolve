@echo off
echo 正在测试AI修复...
echo.

echo 1. 构建项目...
call D:\qt\Tools\CMake_64\bin\cmake.exe --build build --config Release

if %ERRORLEVEL% NEQ 0 (
    echo 构建失败！检查编译错误。
    pause
    exit /b 1
)

echo.
echo 2. 启动游戏进行测试...
echo    请点击开始游戏并观察是否仍然崩溃
echo    如果游戏能正常运行且AI工作，说明修复成功
echo.

start "" "build\bin\Release\ai-devour-evolve.exe"

echo 测试脚本完成。如果游戏正常启动，说明修复成功！
pause
