#!/bin/bash -eu
cd `dirname $0`
cd ..

mkdir -p dll_build

cp sample_plugins/twice_plugin.cpp dll_build/
cp sample_plugins/sum_plugin.cpp dll_build/
cp src/core/project_data.hpp dll_build/

pushd dll_build

clang++ -std=c++20 -O2 -g -shared twice_plugin.cpp -o debug_twice_plugin.so && \
mkdir -p ../build/tests/plugins && \
cp debug_twice_plugin.so ../build/tests/plugins/

clang++ -std=c++20 -O2 -g -shared sum_plugin.cpp -o debug_sum_plugin.so && \
mkdir -p ../build/tests/plugins && \
cp debug_sum_plugin.so ../build/tests/plugins/

popd
