
mkdir build
cd build

chcp 65001

cmake ..
cmake --build . --parallel --config RelWithDebInfo


rem ===== run all test executables =====
for %%f in (tests\RelWithDebInfo\*.exe) do (
    echo Running %%f
    call "%%f"
)

rem ===== run CLI =====
call cli\RelWithDebInfo\prismcascade_cli.exe
