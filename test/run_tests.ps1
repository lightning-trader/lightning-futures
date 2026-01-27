# 创建临时批处理文件来设置环境并运行测试
$tempBatchFile = "$PSScriptRoot\run_tests_temp.bat"

$batchContent = @'
@echo off

REM 设置 Visual Studio 2022 环境变量
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

set "TEST_DIR=%~dp0"
set "BUILD_DIR=%TEST_DIR%build"

REM 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM 切换到测试目录
cd "%TEST_DIR%"

echo === 开始编译单元测试 ===
echo.

REM 编译所有测试文件
set "TEST_FILES=test_basic_types.cpp test_basic_utils.cpp test_memory_pool.cpp test_params.cpp test_trading_context.cpp test_trader_simulator.cpp test_fix.cpp test_position_fix.cpp test_simulator_fix.cpp"

for %%f in (%TEST_FILES%) do (
    if exist "%%f" (
        echo 编译 %%f...
        cl /std:c++17 /W3 /I../src/include /I../src/share "%%f" /Fe"%BUILD_DIR%\%%~nf.exe"
        if errorlevel 1 (
            echo 编译 %%f 失败！
            goto end
        ) else (
            echo 编译 %%f 成功
        )
    ) else (
        echo 跳过 %%f（文件不存在）
    )
)

echo.
echo === 编译完成，开始运行测试 ===
echo.

REM 运行所有测试
for %%f in (%TEST_FILES%) do (
    if exist "%BUILD_DIR%\%%~nf.exe" (
        echo 运行 %%~nf.exe...
        echo ========================
        "%BUILD_DIR%\%%~nf.exe"
        echo ========================
        echo.
    )
)

echo === 所有测试运行完成 ===

:end
cd "%TEST_DIR%"
pause
'@

# 写入临时批处理文件
$batchContent | Out-File -FilePath $tempBatchFile -Encoding ASCII -Force

# 运行临时批处理文件
& cmd.exe /c "$tempBatchFile"

# 删除临时批处理文件
Remove-Item -Path $tempBatchFile -Force -ErrorAction SilentlyContinue
