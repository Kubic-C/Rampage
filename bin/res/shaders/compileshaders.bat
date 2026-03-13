@echo off
setlocal

REM Compile vertex shader
CALL "../../shaderc.exe" -f "shapevs.sh" -o "shapevs.bin" --type v --platform windows -p spirv16-13 -i .
IF ERRORLEVEL 1 (
	echo Failed to compile shapevs.sh
	exit /b 1
)

REM Compile fragment shader
CALL "../../shaderc.exe" -f "shapefs.sh" -o "shapefs.bin" --type f --platform windows -p spirv16-13 -i .
IF ERRORLEVEL 1 (
	echo Failed to compile shapefs.sh
	exit /b 1
)

echo Shader compilation complete.
endlocal