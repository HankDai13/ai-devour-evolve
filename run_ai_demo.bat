@echo off
REM 设置Qt路径
set PATH=D:\qt\6.9.1\msvc2022_64\bin;%PATH%
set QT_PLUGIN_PATH=D:\qt\6.9.1\msvc2022_64\plugins

REM 运行AI演示程序
echo Starting Simple AI Demo...
.\build\Release\simple-ai-demo.exe

pause
