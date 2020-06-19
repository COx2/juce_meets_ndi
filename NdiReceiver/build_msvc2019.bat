@echo off

rem ---Define script directory ---
set SCRIPT_DIRECTORY=%~dp0
cd %SCRIPT_DIRECTORY%

rem --- Set variables for MSVC2019 ---
set PROJECT_NAME=NdiReceiver
set EXPORTER_NAME=VisualStudio2019
set MSVC_VERSION=2019
set MSVC_OFFERING=Community
set ARCHITECTURE=x64
set BUILD_CONFIG=Release

rem --- Generate IDE project file(.sln) by Projucer ---
cd %SCRIPT_DIRECTORY%
..\Projucer\Projucer.exe --resave %PROJECT_NAME%.jucer

rem --- Get solution file name from Projucer ---
cd %SCRIPT_DIRECTORY%
for /f "usebackq delims=" %%a in (`..\Projucer\Projucer.exe --status %PROJECT_NAME%.jucer ^| find "Name:"`) do set SOLUTION_NAME=%%a
for /f "tokens=1,2 delims= " %%a in ("%SOLUTION_NAME%") do set SOLUTION_NAME=%%b

rem --- Get project version number from Projucer ---
for /f "usebackq delims=" %%a in (`..\Projucer\Projucer.exe --get-version %PROJECT_NAME%.jucer`) do set VERSION_NUMBER=%%a

rem --- Start Visual Studio 2019's Developer Command Line Tool ---
call "C:\Program Files (x86)\Microsoft Visual Studio\%MSVC_VERSION%\%MSVC_OFFERING%\Common7\Tools\VsDevCmd.bat"

rem --- Build by MSBuild ---
cd %SCRIPT_DIRECTORY%
MSBuild .\Builds\%EXPORTER_NAME%\%SOLUTION_NAME%.sln /t:clean;rebuild /p:Configuration=%BUILD_CONFIG%;Platform=%ARCHITECTURE%
if %ERRORLEVEL% neq 0 goto FAILURE

rem --- Rename to adding version number for VST3 file --
set PLUGIN_FORMAT=VST3
set SRC_FILE=.\Builds\%EXPORTER_NAME%\%ARCHITECTURE%\%BUILD_CONFIG%\%PLUGIN_FORMAT%\%SOLUTION_NAME%.vst3
if exist "%SRC_FILE%" (
    ren "%SRC_FILE%" "%SOLUTION_NAME%-%VERSION_NUMBER%.vst3"
)
if %ERRORLEVEL% neq 0 goto FAILURE

goto SUCCESS

:FAILURE
echo ErrorLevel:%ERRORLEVEL%
echo ***Build Failed***
exit 1

:SUCCESS
echo ***Build Success***
exit /B 0