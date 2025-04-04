@echo off
rem バッチファイルのあるディレクトリに移動
cd /d %~dp0

rem 一つ上のディレクトリに移動
cd ..

rem dll_buildディレクトリがなければ作成
if not exist dll_build mkdir dll_build

rem ファイルのコピー（/Yオプションで上書き確認を省略）
copy /Y sample_plugins\twice_plugin.cpp dll_build\
copy /Y sample_plugins\sum_plugin.cpp dll_build\
copy /Y src\core\project_data.hpp dll_build\

rem dll_buildディレクトリへ移動
pushd dll_build

rem twice_pluginのビルド
clang++ -std=c++20 -O2 -g -shared twice_plugin.cpp -o debug_twice_plugin.dll
if errorlevel 1 (
    echo twice_pluginのビルドに失敗しました。
    popd
    pause
    exit /b 1
)

rem ../build/tests/pluginsディレクトリがなければ作成
if not exist ..\build\tests\RelWithDebInfo\plugins mkdir ..\build\tests\RelWithDebInfo\plugins

rem ビルド結果のコピー
copy /Y debug_twice_plugin.dll ..\build\tests\RelWithDebInfo\plugins\

rem sum_pluginのビルド
clang++ -std=c++20 -O2 -g -shared sum_plugin.cpp -o debug_sum_plugin.dll
if errorlevel 1 (
    echo sum_pluginのビルドに失敗しました。
    popd
    pause
    exit /b 1
)

rem 再度コピー（ディレクトリは既に存在するはず）
copy /Y debug_sum_plugin.dll ..\build\tests\RelWithDebInfo\plugins\

rem 元のディレクトリに戻る
popd
