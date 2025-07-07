@echo off
echo ========================================
echo Building Multi-Agent GoBigger Environment
echo ========================================

REM Set build directory
set BUILD_DIR=build-multi-agent
set QT_DIR=D:\qt\6.9.1\msvc2022_64

REM Set Qt environment variables
set QT_QPA_PLATFORM_PLUGIN_PATH=%QT_DIR%\plugins\platforms
set QT_PLUGIN_PATH=%QT_DIR%\plugins
set PATH=%QT_DIR%\bin;%PATH%

echo Checking Qt installation...
if not exist "%QT_DIR%\bin\Qt6Core.dll" (
    echo ERROR: Qt not found at %QT_DIR%
    echo Please check your Qt installation path
    pause
    exit /b 1
)

if not exist "%QT_DIR%\plugins\platforms\qwindows.dll" (
    echo ERROR: Qt platform plugins not found
    echo Expected at: %QT_DIR%\plugins\platforms\qwindows.dll
    pause
    exit /b 1
)

echo Qt installation verified ✓
echo.

echo Cleaning previous build...
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
mkdir %BUILD_DIR%

echo.
echo Copying CMake configuration...
if exist CMakeLists.txt (
    copy CMakeLists.txt CMakeLists.txt.bak >nul
)
copy CMakeLists-multi-agent.txt CMakeLists.txt

cd %BUILD_DIR%

echo.
echo Configuring CMake with multi-agent support...
cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
      -DQt6_DIR="%QT_DIR%\lib\cmake\Qt6" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DBUILD_MULTI_AGENT=ON ^
      -S ..

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build . --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
cd ..

echo.
echo Restoring original CMakeLists.txt...
if exist CMakeLists.txt.bak (
    move CMakeLists.txt.bak CMakeLists.txt >nul
)

echo.
echo Copying Qt DLLs to output directory...
REM Copy necessary Qt DLLs to python directory for runtime
copy "%QT_DIR%\bin\Qt6Core.dll" python\ >nul 2>&1
copy "%QT_DIR%\bin\Qt6Gui.dll" python\ >nul 2>&1
copy "%QT_DIR%\bin\Qt6Widgets.dll" python\ >nul 2>&1

REM Create platforms directory and copy platform plugin
if not exist python\platforms mkdir python\platforms
copy "%QT_DIR%\plugins\platforms\qwindows.dll" python\platforms\ >nul 2>&1

echo Qt dependencies copied ✓

echo.
echo Testing Python import...
python -c "import sys; sys.path.append('python'); import gobigger_multi_env; print('Multi-agent module imported successfully!')"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD COMPLETE! 
    echo ========================================
    echo.
    echo The multi-agent environment is ready to use.
    echo.
    echo To run training:
    echo   python train_multi_agent.py
    echo.
    echo To test environment:
    echo   python scripts/gobigger_gym_env.py multi-agent
    echo.
    echo Qt environment is automatically configured.
) else (
    echo.
    echo Import test failed. Please check error messages above.
    echo You may need to run: setup_qt_env.bat
)

echo.
pause
