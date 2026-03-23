# Lightning Futures Test Runner Script
# Uses Visual Studio 2022 compiler with vcvarsall.bat

$TestDir = $PSScriptRoot
$BuildDir = Join-Path $TestDir "build"
$SrcInclude = Join-Path $TestDir "../src/include"
$SrcShare = Join-Path $TestDir "../src/share"

# Visual Studio 2022 paths
$VsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$VcVarsAll = "$VsPath\VC\Auxiliary\Build\vcvarsall.bat"

# Check if vcvarsall.bat exists
if (!(Test-Path $VcVarsAll)) {
    Write-Host "ERROR: vcvarsall.bat not found at $VcVarsAll" -ForegroundColor Red
    exit 1
}

# Create build directory
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Write-Host "========================================"
Write-Host "   Lightning Futures Test Runner"
Write-Host "========================================"
Write-Host ""

# Create a temporary batch file to set up environment and compile
$TempBat = "$BuildDir\compile_temp.bat"
$ClOutput = "$BuildDir\cl_output.txt"

# Working test files list (tests that compile and run correctly)
$TestFiles = @(
    "test_basic_types.cpp",
    "test_basic_utils.cpp",
    "test_ringbuffer.cpp",
    "test_event_center.cpp",
    "test_crontab_scheduler.cpp",
    "test_stream_buffer.cpp"
)

$PassedCount = 0
$FailedCount = 0
$SkippedCount = 0

foreach ($TestFile in $TestFiles) {
    $TestPath = Join-Path $TestDir $TestFile
    $TestName = [System.IO.Path]::GetFileNameWithoutExtension($TestFile)
    $ExePath = "$BuildDir\$TestName.exe"
    
    if (!(Test-Path $TestPath)) {
        Write-Host "SKIP: $TestFile (not found)" -ForegroundColor Yellow
        $SkippedCount++
        continue
    }
    
    Write-Host "Compiling: $TestFile ..." -NoNewline
    
    # Create batch file to compile with proper environment
    $batContent = @"
@echo off
call "$VcVarsAll" amd64 >nul 2>&1
cl /std:c++17 /W3 /EHsc /utf-8 /I"$SrcInclude" /I"$SrcShare" "$TestPath" /Fe"$ExePath" > "$ClOutput" 2>&1
exit /b %errorlevel%
"@
    $batContent | Out-File -FilePath $TempBat -Encoding ASCII
    
    # Run the batch file
    cmd /c "$TempBat"
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "OK" -ForegroundColor Green
        
        Write-Host "Running: $TestName ..."
        Write-Host "----------------------------------------"
        
        # Run test
        & $ExePath
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "PASSED!" -ForegroundColor Green
            $PassedCount++
        } else {
            Write-Host "FAILED!" -ForegroundColor Red
            $FailedCount++
        }
        Write-Host ""
    } else {
        Write-Host "FAILED" -ForegroundColor Red
        if (Test-Path $ClOutput) {
            Get-Content $ClOutput
        }
        $FailedCount++
    }
}

# Clean up temp file
if (Test-Path $TempBat) {
    Remove-Item $TempBat -Force
}

Write-Host "========================================"
Write-Host "   Test Summary"
Write-Host "========================================"
Write-Host "   Passed: $PassedCount" -ForegroundColor Green
Write-Host "   Failed: $FailedCount" -ForegroundColor $(if ($FailedCount -eq 0) { "Green" } else { "Red" })
Write-Host "   Skipped: $SkippedCount" -ForegroundColor Yellow
Write-Host "========================================"

if ($FailedCount -gt 0) {
    exit 1
}
exit 0