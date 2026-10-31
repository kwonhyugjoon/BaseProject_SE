@echo off
setlocal
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
where cl >nul 2>&1
if errorlevel 1 call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%ERRORLEVEL%"=="0" exit /b 1
set "CHECK_DIR=%TEMP%\BaseProject_SE_TravelChecks"
if not exist "%CHECK_DIR%" mkdir "%CHECK_DIR%"
copy /y "%ROOT%\x64\Release\freeglut.dll" "%CHECK_DIR%\" >nul
if not "%ERRORLEVEL%"=="0" exit /b 1
copy /y "%ROOT%\x64\Release\glew32.dll" "%CHECK_DIR%\" >nul
if not "%ERRORLEVEL%"=="0" exit /b 1
cd /d "%CHECK_DIR%"
cl /nologo /c /MP4 /O2 /MD /EHsc /std:c++14 /utf-8 /DNOMINMAX /I"%ROOT%\SimpleGame" "%ROOT%\SimpleGame\Renderer.cpp" "%ROOT%\SimpleGame\FirstLevel.cpp" "%ROOT%\SimpleGame\QuestJournal.cpp" "%ROOT%\SimpleGame\Actor.cpp" "%ROOT%\SimpleGame\SceneGraph.cpp" "%ROOT%\SimpleGame\AssetCache.cpp" "%ROOT%\SimpleGame\LevelModels.cpp" "%ROOT%\SimpleGame\CharacterSprites.cpp" "%ROOT%\SimpleGame\MonitorState.cpp" "%ROOT%\SimpleGame\MonitorServer.cpp" "%ROOT%\tests\WorldTravelTests.cpp" "%ROOT%\tests\QuestJournalTests.cpp" "%ROOT%\tests\SprintTests.cpp" "%ROOT%\tests\RegionConnectivityTests.cpp" "%ROOT%\tests\LevelExpansionTests.cpp" "%ROOT%\tests\TutorialPopulationTests.cpp"
if not "%ERRORLEVEL%"=="0" exit /b 1
set "CORE=QuestJournal.obj Renderer.obj Actor.obj SceneGraph.obj AssetCache.obj LevelModels.obj CharacterSprites.obj MonitorState.obj MonitorServer.obj"
set "LIBS=/link /LIBPATH:"%ROOT%\SimpleGame\Dependencies\x64" freeglut.lib glew32.lib opengl32.lib gdi32.lib user32.lib ws2_32.lib shell32.lib"
cl /nologo SprintTests.obj FirstLevel.obj %CORE% /Fe:SprintTests.exe %LIBS%
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo QuestJournalTests.obj FirstLevel.obj %CORE% /Fe:QuestJournalTests.exe %LIBS%
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo WorldTravelTests.obj FirstLevel.obj %CORE% /Fe:WorldTravelTests.exe %LIBS%
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo RegionConnectivityTests.obj %CORE% /Fe:RegionConnectivityTests.exe %LIBS%
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo LevelExpansionTests.obj FirstLevel.obj %CORE% /Fe:LevelExpansionTests.exe %LIBS%
if not "%ERRORLEVEL%"=="0" exit /b 1
cl /nologo TutorialPopulationTests.obj FirstLevel.obj %CORE% /Fe:TutorialPopulationTests.exe %LIBS%
if not "%ERRORLEVEL%"=="0" exit /b 1
SprintTests.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
QuestJournalTests.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
WorldTravelTests.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
RegionConnectivityTests.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
LevelExpansionTests.exe
if not "%ERRORLEVEL%"=="0" exit /b 1
TutorialPopulationTests.exe
exit /b %ERRORLEVEL%
