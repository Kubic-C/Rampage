@echo off
REM Batch script to generate Capnproto C++ bindings
REM Usage: generateCapnproto.bat
REM 
REM Generates rampage.capnp.h and rampage.capnp.c++ from rampage.capnp

setlocal enabledelayedexpansion

set CAPNP_EXE=capnp
set SCRIPT_DIR=%~dp0
set CAPNP_FILE=%SCRIPT_DIR%rampage.capnp

REM Check if rampage.capnp exists
if not exist "%CAPNP_FILE%" (
    echo Error: rampage.capnp not found at %CAPNP_FILE%
    exit /b 1
)

REM Try to find capnp if not in PATH
where /q %CAPNP_EXE%
if errorlevel 1 (
    echo Searching for capnp executable...
    
    REM Check common installation locations
    if exist "C:\Program Files\capnproto\bin\capnp.exe" (
        set CAPNP_EXE=C:\Program Files\capnproto\bin\capnp.exe
    ) else if exist "%ProgramFiles%\Cap'n Proto\bin\capnp.exe" (
        set CAPNP_EXE=%ProgramFiles%\Cap'n Proto\bin\capnp.exe
    ) else if exist "%SCRIPT_DIR%..\build\deps\capnproto\bin\capnp.exe" (
        set CAPNP_EXE=%SCRIPT_DIR%..\build\deps\capnproto\bin\capnp.exe
    ) else (
        echo Error: Could not find capnp executable. Please ensure it's in PATH.
        exit /b 1
    )
)

echo Generating Capnproto bindings...
echo Running: %CAPNP_EXE% compile -o c++ rampage.capnp

REM Change to script directory and run capnp
cd /d "%SCRIPT_DIR%"
"%CAPNP_EXE%" compile -o c++ rampage.capnp

if errorlevel 1 (
    echo Error: Capnp compilation failed
    exit /b 1
)

REM Check if files were generated
if exist "%SCRIPT_DIR%rampage.capnp.h" (
    echo. 
    echo [Success] Generated rampage.capnp.h
) else (
    echo Warning: rampage.capnp.h not found
)

if exist "%SCRIPT_DIR%rampage.capnp.c++" (
    echo [Success] Generated rampage.capnp.c++
) else (
    echo Warning: rampage.capnp.c++ not found
)

echo.
echo Done.
exit /b 0
