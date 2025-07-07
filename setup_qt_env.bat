@echo off
REM Setup Qt environment for running multi-agent environment

REM Set Qt paths
set QT_DIR=D:\qt\6.9.1\msvc2022_64
set QT_QPA_PLATFORM_PLUGIN_PATH=%QT_DIR%\plugins\platforms
set QT_PLUGIN_PATH=%QT_DIR%\plugins

REM Add Qt bin directory to PATH for DLLs
set PATH=%QT_DIR%\bin;%PATH%

echo Qt environment configured:
echo QT_DIR=%QT_DIR%
echo QT_QPA_PLATFORM_PLUGIN_PATH=%QT_QPA_PLATFORM_PLUGIN_PATH%
echo QT_PLUGIN_PATH=%QT_PLUGIN_PATH%
echo.

REM Test if Python can import the module
echo Testing multi-agent environment import...
python -c "import sys; sys.path.append('python'); import gobigger_multi_env; print('Import successful!')"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Environment setup complete! You can now run:
    echo python train_multi_agent.py
    echo.
    echo Or test with:
    echo python scripts/gobigger_gym_env.py multi-agent
) else (
    echo.
    echo Import failed. Please check the error messages above.
)
