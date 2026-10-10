@echo off
rem Configures, builds and tests the Windows app from any terminal.
rem Usage: build.cmd [debug|release]   (default: debug)
rem VS Code users can instead use the CMake Tools extension with the presets.
setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=debug

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do set VSDIR=%%i
if not defined VSDIR (
  echo Visual Studio with the C++ workload was not found.
  exit /b 1
)
call "%VSDIR%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul || exit /b 1

cd /d "%~dp0"
cmake --preset windows || exit /b 1
cmake --build --preset %CONFIG% || exit /b 1
if /i "%CONFIG%"=="debug" ctest --preset debug || exit /b 1

echo.
echo Built: %USERPROFILE%\.builds\metronome-juce\build\Metronome_artefacts\%CONFIG%\Metronome.exe
