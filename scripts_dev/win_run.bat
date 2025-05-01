@echo off

rem === build ===
if not exist "build\" (
    mkdir "build"  || goto :error
)
cd build              || goto :error
chcp 65001            || goto :error

cmake ..              || goto :error
cmake --build . --parallel --config RelWithDebInfo || goto :error

rem === run all test executables ===
for %%f in (tests\RelWithDebInfo\*.exe) do (
    echo Running %%f
    call "%%f"       || goto :error
    @REM call "%%f" --gtest_filter="DelayAlgoProp*"       || goto :error
)

rem === install plugins ===
if not exist "cli\RelWithDebInfo\plugins" (
    mkdir "cli\RelWithDebInfo\plugins" || goto :error
)
copy /Y "sample_plugins\RelWithDebInfo\*.dll" "cli\RelWithDebInfo\plugins\" || goto :error

rem === run CLI ===
call cli\RelWithDebInfo\prismcascade_cli.exe || goto :error

echo.
echo === done ===
goto :eof

:error
echo.
echo *** build stopped: ERRORLEVEL=%errorlevel% ***
exit /b %errorlevel%
