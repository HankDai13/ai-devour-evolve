@echo off
echo Testing simple functionality...
echo.

:: 设置Qt平台插件路径
set QT_QPA_PLATFORM_PLUGIN_PATH=D:\qt\6.9.1\msvc2022_64\plugins\platforms

echo Starting program with minimal output...
cd /d "D:\Coding\Projects\ai-devour-evolve"

:: 运行程序并捕获输出
".\build\Release\ai-devour-evolve.exe" > debug_output.txt 2>&1

echo.
echo Program exited with code: %ERRORLEVEL%

:: 显示输出文件的最后几行
if exist debug_output.txt (
    echo.
    echo Last lines of output:
    echo ========================
    tail -n 20 debug_output.txt 2>nul || (
        echo Output file content:
        type debug_output.txt
    )
) else (
    echo No output file generated
)

pause
