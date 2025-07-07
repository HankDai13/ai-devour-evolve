:: 设置Qt平台插件路径
set QT_QPA_PLATFORM_PLUGIN_PATH=D:\qt\6.9.1\msvc2022_64\plugins\platforms
@echo off
echo Running AI Devour Evolve in Debug Mode...
echo.
echo Starting program...
cd /d "D:\Coding\Projects\ai-devour-evolve"
".\build\Release\ai-devour-evolve.exe"
echo.
echo Program exited with code: %ERRORLEVEL%
pause
