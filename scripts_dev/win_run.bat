
mkdir build
cd build

cmake .. -DBUILD_TESTS=ON
cmake --build . --config RelWithDebInfo
.\tests\RelWithDebInfo\cui_render.exe
