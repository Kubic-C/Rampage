# PowerShell script to generate Capnproto C++ bindings
# Usage: .\generateCapnproto.ps1
# 
# Generates rampage.capnp.h and rampage.capnp.c++ from rampage.capnp

param(
    [string]$CapnpExecutable = "capnp",
    [switch]$Verbose = $false
)

# Get script directory (where rampage.capnp is located)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$CapnpFile = Join-Path $ScriptDir "rampage.capnp"

if (-not (Test-Path $CapnpFile)) {
    Write-Error "rampage.capnp not found at $CapnpFile"
    exit 1
}

if ($Verbose) {
    Write-Host "Script directory: $ScriptDir"
    Write-Host "Capnproto file: $CapnpFile"
    Write-Host "Capnp executable: $CapnpExecutable"
}

# Try to find capnp executable if not found
if (-not (Get-Command $CapnpExecutable -ErrorAction SilentlyContinue)) {
    Write-Warning "$CapnpExecutable not found in PATH, searching for it..."
    
    # Check common locations
    $searchPaths = @(
        "C:\Program Files\capnproto\bin\capnp.exe",
        "$env:ProgramFiles\Cap'n Proto\bin\capnp.exe",
        "$PSScriptRoot\..\build\deps\capnproto\bin\capnp.exe",
        "$PSScriptRoot\..\build\Debug\bin\capnp.exe",
        "$PSScriptRoot\..\build\Release\bin\capnp.exe"
    )
    
    $found = $false
    foreach ($path in $searchPaths) {
        if (Test-Path $path) {
            $CapnpExecutable = $path
            $found = $true
            if ($Verbose) {
                Write-Host "Found capnp at: $CapnpExecutable"
            }
            break
        }
    }
    
    if (-not $found) {
        Write-Error "Could not find capnp executable. Please ensure it's in PATH or specify -CapnpExecutable parameter."
        exit 1
    }
}

Write-Host "Generating Capnproto bindings..." -ForegroundColor Cyan

# Run capnp compiler
# The -o flag specifies the output language (c++ in this case)
# Running from the script directory ensures relative imports work correctly
Push-Location $ScriptDir
try {
    if ($Verbose) {
        Write-Host "Running: & '$CapnpExecutable' compile -o c++ rampage.capnp"
    }
    
    & $CapnpExecutable compile -o c++ rampage.capnp
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Successfully generated rampage.capnp.h and rampage.capnp.c++" -ForegroundColor Green
        
        # Verify files were created
        $headerFile = Join-Path $ScriptDir "rampage.capnp.h"
        $sourceFile = Join-Path $ScriptDir "rampage.capnp.c++"
        
        if ((Test-Path $headerFile) -and (Test-Path $sourceFile)) {
            Write-Host "✓ Header file: rampage.capnp.h ($(Get-Item $headerFile).Length) bytes"
            Write-Host "✓ Source file: rampage.capnp.c++ ($(Get-Item $sourceFile).Length) bytes"
        } else {
            Write-Warning "Generated files not found at expected location"
            if (-not (Test-Path $headerFile)) {
                Write-Warning "Missing: rampage.capnp.h"
            }
            if (-not (Test-Path $sourceFile)) {
                Write-Warning "Missing: rampage.capnp.c++"
            }
        }
    } else {
        Write-Error "Capnp compilation failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
} catch {
    Write-Error "Error running capnp: $_"
    exit 1
} finally {
    Pop-Location
}

exit 0
