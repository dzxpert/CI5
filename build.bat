@echo off
setlocal
pushd "%~dp0" || exit /b 1
set "VS_VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VS_VCVARS%" (
    echo [ERROR] MSVC x86 tools not found at "%VS_VCVARS%".
    popd
    exit /b 1
)
call "%VS_VCVARS%" x86
if errorlevel 1 goto failed
if exist "build\CMakeCache.txt" (
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
) else (
    where ninja >nul 2>&1
    if errorlevel 1 (
        cmake -S . -B build -A Win32
    ) else (
        cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    )
)
if errorlevel 1 goto failed
cmake --build build --config Release
if errorlevel 1 goto failed
echo [SUCCESS] Build complete. Run inject.bat with CI5 open.
popd
exit /b 0
:failed
echo [ERROR] Build failed. If CI5Mod.dll is in use, press END in CI5 to unload it first.
popd
exit /b 1
