@echo off
setlocal
pushd "%~dp0" || exit /b 1
set "MOD_DIR=build"
if not exist "%MOD_DIR%\CI5Injector.exe" if exist "build\Release\CI5Injector.exe" set "MOD_DIR=build\Release"
if not exist "%MOD_DIR%\CI5Injector.exe" goto missing
if not exist "%MOD_DIR%\CI5Mod.dll" goto missing
"%MOD_DIR%\CI5Injector.exe" "%CD%\%MOD_DIR%\CI5Mod.dll"
set "RESULT=%ERRORLEVEL%"
goto done
:missing
echo [ERROR] Build the mod first with build.bat. Both CI5Injector.exe and CI5Mod.dll are required.
set "RESULT=1"
:done
popd
if /i not "%~1"=="--no-pause" pause
exit /b %RESULT%
