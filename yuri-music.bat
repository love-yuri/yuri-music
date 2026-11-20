@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

cmake --version >nul 2>&1
if errorlevel 1 (
  echo [ERROR] CMake 4.0 or newer is required in PATH.
  goto :failed
)
ninja --version >nul 2>&1
if errorlevel 1 (
  echo [ERROR] Ninja is required in PATH.
  goto :failed
)

if /i "%VSCMD_ARG_TGT_ARCH%"=="x64" goto :configure
set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" (
  echo [ERROR] Install Visual Studio with the Desktop development with C++ workload.
  goto :failed
)
set "vs_install="
for /f "usebackq delims=" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "vs_install=%%i"
if not defined vs_install (
  echo [ERROR] No Visual Studio installation with x64 C++ tools was found.
  goto :failed
)
call "%vs_install%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 goto :failed

:configure
echo [1/2] Configuring Release...
cmake -S . -B build\release -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_CXX_COMPILER=cl ^
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=%CD%\output" ^
  -DBUILD_TESTING=OFF -DBUILD_DEMO=OFF
if errorlevel 1 goto :failed

echo [2/2] Building Release...
cmake --build build\release --config Release --target yuri-music --parallel
if errorlevel 1 goto :failed

echo [OK] Release output: "%CD%\output"
popd
exit /b 0

:failed
echo [ERROR] Release build failed. See the messages above.
popd
exit /b 1
