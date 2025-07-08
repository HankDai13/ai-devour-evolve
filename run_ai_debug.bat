@echo off
setlocal

:: 设置Qt平台插件路径
set QT_QPA_PLATFORM_PLUGIN_PATH=D:\qt\6.9.1\msvc2022_64\plugins\platforms

:: 显示启动信息
echo ====================================
echo   AI Devour Evolve Game Launcher
echo ====================================
echo.
echo 设置Qt环境变量...
echo QT_QPA_PLATFORM_PLUGIN_PATH=%QT_QPA_PLATFORM_PLUGIN_PATH%
echo.

:: 检查程序是否存在
if not exist ".\build\Release\ai-devour-evolve.exe" (
    echo 错误：找不到游戏程序文件！
    echo 请确保项目已正确编译并且路径正确。
    echo.
    pause
    exit /b 1
)

:: 启动游戏
echo 正在启动游戏...
echo.
.\build\Release\ai-crash-debug.exe

:: 如果程序异常退出，显示信息
if %ERRORLEVEL% neq 0 (
    echo.
    echo 程序异常退出，错误代码：%ERRORLEVEL%
    echo.
)

echo.
echo 游戏已退出。
pause

endlocal
