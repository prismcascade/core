@echo off
cd /d %~dp0
cd ..

if not exist dll_build mkdir dll_build

copy /Y sample_plugins\twice_plugin.cpp dll_build\
copy /Y sample_plugins\sum_plugin.cpp dll_build\
copy /Y sample_plugins\count_plugin.cpp dll_build\
copy /Y src\core\project_data.hpp dll_build\

pushd dll_build

rem twice_plugin
clang++ -std=c++20 -O2 -g -shared twice_plugin.cpp -o debug_twice_plugin.dll
if errorlevel 1 (
    echo "fail: twice_plugin"
    popd
    pause
    exit /b 1
)

if not exist ..\build\tests\RelWithDebInfo\plugins mkdir ..\build\tests\RelWithDebInfo\plugins
copy /Y debug_twice_plugin.dll ..\build\tests\RelWithDebInfo\plugins\

rem sum_plugin
clang++ -std=c++20 -O2 -g -shared sum_plugin.cpp -o debug_sum_plugin.dll
if errorlevel 1 (
    echo "fail: sum_plugin"
    popd
    pause
    exit /b 1
)

copy /Y debug_sum_plugin.dll ..\build\tests\RelWithDebInfo\plugins\

rem count_plugin
clang++ -std=c++20 -O2 -g -shared count_plugin.cpp -o debug_count_plugin.dll
if errorlevel 1 (
    echo "fail: count_plugin"
    popd
    pause
    exit /b 1
)

copy /Y debug_count_plugin.dll ..\build\tests\RelWithDebInfo\plugins\

popd
