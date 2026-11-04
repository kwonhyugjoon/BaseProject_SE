@echo off
setlocal
for %%I in ("%~dp0..") do set "GAME_ROOT=%%~fI"
set "GAME_CMAKE=cmake"
where cmake >nul 2>&1
if errorlevel 1 (
    set "GAME_CMAKE="
    for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -find "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"`) do set "GAME_CMAKE=%%I"
)
if not defined GAME_CMAKE (
    echo CMake is required. Install the Visual Studio C++ CMake tools component.
    exit /b 1
)
set "GAME_CONFIG=%~1"
if not defined GAME_CONFIG set "GAME_CONFIG=Release"
"%GAME_CMAKE%" -S "%GAME_ROOT%" -B "%GAME_ROOT%\out\build\travel-checks" -A x64
if not "%ERRORLEVEL%"=="0" exit /b 1
"%GAME_CMAKE%" --build "%GAME_ROOT%\out\build\travel-checks" --config "%GAME_CONFIG%" --parallel 4
if not "%ERRORLEVEL%"=="0" exit /b 1
for %%I in ("%GAME_CMAKE%") do set "GAME_CTEST=%%~dpIctest.exe"
if not exist "%GAME_CTEST%" set "GAME_CTEST=ctest"
"%GAME_CTEST%" --test-dir "%GAME_ROOT%\out\build\travel-checks" -C "%GAME_CONFIG%" --output-on-failure
exit /b %ERRORLEVEL%
