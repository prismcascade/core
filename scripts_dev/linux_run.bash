#!/bin/bash -eu
cd `dirname $0`
cd ..

mkdir -p build
pushd build

cmake .. -DBUILD_TESTS=ON && \
cmake --build . --config RelWithDebInfo && \
./tests/cui_render

popd
