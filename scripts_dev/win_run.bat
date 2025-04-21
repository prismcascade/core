
mkdir build
cd build

chcp 65001

cmake ..
cmake --build . --parallel --config RelWithDebInfo

.\cli\RelWithDebInfo\prismcascade_cli.exe

