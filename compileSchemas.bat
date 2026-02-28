@echo off
REM Compile Cap'n Proto schema files
REM This script regenerates .c++ and .h files from .capnp schemas

setlocal enabledelayedexpansion

REM Navigate to schema directory
cd /d "%~dp0src\common\schema"

if not exist "rampage.capnp" (
    echo Error: rampage.capnp not found in %cd%
    exit /b 1
)

echo Compiling Cap'n Proto schemas...
echo Current directory: %cd%

REM Use capnp compiler from bin directory
set CAPNP_COMPILER=%~dp0bin\capnp.exe

if not exist "%CAPNP_COMPILER%" (
    echo Error: capnp compiler not found at %CAPNP_COMPILER%
    echo Build Cap'n Proto or ensure bin\capnp.exe exists
    exit /b 1
)

echo Using capnp: %CAPNP_COMPILER%

REM Add bin directory to PATH so capnp can find capnpc-c++ plugin
set PATH=%~dp0bin;%PATH%

REM Compile the schema
"%CAPNP_COMPILER%" compile -oc++ rampage.capnp
if errorlevel 1 (
    echo Error: Failed to compile rampage.capnp
    exit /b 1
)

echo.
echo Successfully compiled rampage.capnp
echo Generated files:
echo   - rampage.capnp.h
echo   - rampage.capnp.c++
echo.
echo Build the project to regenerate headers if needed.
pause
