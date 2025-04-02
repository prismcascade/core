
mkdir build
cd build

chcp 65001

cmake .. -DBUILD_TESTS=ON
cmake --build . --config RelWithDebInfo
.\tests\RelWithDebInfo\cui_render.exe
