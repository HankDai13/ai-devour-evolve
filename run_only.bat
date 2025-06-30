@echo off
setlocal

:: 设置Qt平台插件路径
set QT_QPA_PLATFORM_PLUGIN_PATH=D:\qt\6.9.1\msvc2022_64\plugins\platforms

:: 运行程序
echo 正在启动游戏...
.\build\Release\ai-devour-evolve.exe

endlocal